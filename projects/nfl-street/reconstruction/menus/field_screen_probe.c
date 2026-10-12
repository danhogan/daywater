#include "field_catalog.h"
#include "field_bootstrap.h"
#include "field_availability.h"
#include "field_stat_text.h"
#include "construct.h"
#include "events.h"
#include "font.h"
#include "image_construct.h"
#include "main_menu.h"
#include "screen.h"
#include "text_construct.h"
#include "text.h"
#include "team_classes.h"
#include "field_screen_preview.h"
#include "field_screen_runtime.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_BASE 0x81100000u
#define VM_STACK_BASE 0x817c0000u
#define SCREEN_MAX (8u * 1024u * 1024u)

typedef nflstreet_field_resource_effect resource_effect;
typedef nflstreet_field_screen_state probe;
typedef nflstreet_field_screen_runtime field_runtime;

static uint32_t be32(const uint8_t *p);
static int service(void *, uint8_t *, size_t, uint32_t,
                   const nflstreet_ui_service_request *, uint32_t *);
static int memory_service(void *, uint8_t *, size_t, uint32_t,
                          const nflstreet_ui_service_request *, uint32_t *,
                          uint32_t, uint32_t[256]);
int nflstreet_field_screen_runtime_event(void *, unsigned);
int nflstreet_field_screen_runtime_tick(void *, uint32_t);

static int ack_prefix(void *queue, size_t element_size, unsigned *queued,
                      unsigned capacity, const void *expected, unsigned count) {
    if (!queue || !queued || *queued > capacity || count > *queued ||
        (count && !expected)) return 0;
    if (count && memcmp(queue, expected, (size_t)count * element_size) != 0)
        return 0;
    unsigned remain = *queued - count;
    memmove(queue, (uint8_t *)queue + (size_t)count * element_size,
            (size_t)remain * element_size);
    memset((uint8_t *)queue + (size_t)remain * element_size, 0,
           (size_t)count * element_size);
    *queued = remain;
    return 1;
}

int nflstreet_field_screen_runtime_ack_resource_effects(
        nflstreet_field_screen_state *s,
        const nflstreet_field_resource_effect *expected, unsigned count) {
    return s && ack_prefix(s->resource_effects, sizeof s->resource_effects[0],
        &s->resource_effect_count, 128, expected, count);
}

int nflstreet_field_screen_runtime_ack_button_resources(
        nflstreet_field_screen_state *s,
        const nflstreet_ui_button_effect *expected, unsigned count) {
    return s && ack_prefix(s->main_menu.resources,
        sizeof s->main_menu.resources[0], &s->main_menu.resource_count,
        128, expected, count);
}

int nflstreet_field_screen_runtime_ack_audio(
        nflstreet_field_screen_state *s, const uint32_t *expected, unsigned count) {
    return s && ack_prefix(s->main_menu.audio, sizeof s->main_menu.audio[0],
        &s->main_menu.audio_count, 128, expected, count);
}

int nflstreet_field_screen_runtime_ack_broadcasts(
        nflstreet_field_screen_state *s,
        const nflstreet_ui_service_request *expected, unsigned count) {
    return s && ack_prefix(s->main_menu.broadcasts, sizeof s->main_menu.broadcasts[0],
        &s->main_menu.broadcast_count, 128, expected, count);
}

int nflstreet_field_screen_runtime_ack_links(
        nflstreet_field_screen_state *s, const uint32_t *expected, unsigned count) {
    return s && ack_prefix(s->main_menu.links, sizeof s->main_menu.links[0],
        &s->main_menu.link_count, 16, expected, count);
}

int nflstreet_field_screen_runtime_ack_focus_effects(
        nflstreet_field_screen_state *s,
        const nflstreet_ui_focus_effects *expected, unsigned count) {
    return s && ack_prefix(s->focus_effects, sizeof s->focus_effects[0],
        &s->focus_effect_count, 128, expected, count);
}

static int drain_field_focus(uint8_t *bytes, size_t size, field_runtime *runtime,
                             unsigned first) {
    probe *p = runtime ? runtime->state : NULL;
    if (!bytes || !p || first > p->main_menu.pending_count ||
        p->main_menu.pending_count > 128 || p->focus_effect_count > 128) return 0;
    unsigned count = be32(bytes);
    uint32_t rows_address = be32(bytes + 4);
    if (!count || count > 1024 || rows_address < SCREEN_BASE ||
        rows_address - SCREEN_BASE > size || (size_t)count * 20 > size - (rows_address - SCREEN_BASE)) return 0;
    const uint8_t *rows = bytes + (rows_address - SCREEN_BASE);
    /* Process the complete ordered pending prefix. A successful drain consumes
       it so repeated navigation does not fill the one-shot request queue. */
    (void)first;
    for (unsigned i = 0; i < p->main_menu.pending_count; ++i) {
        nflstreet_ui_service_request q = p->main_menu.pending[i];
        unsigned target = count;
        for (unsigned j = 0; j < count; ++j)
            if (be32(rows + j * 20) == q.target) { target = j; break; }
        if (target == count || p->focus_effect_count >= 128) return 0;
        nflstreet_ui_focus_effects effects = {0};
        if (!nflstreet_ui_focus_indexed_execute_context(bytes, size, SCREEN_BASE,
                target, 1, (uint32_t)(int32_t)(int16_t)q.property, &effects,
                &p->main_menu.entry_flags, runtime->current_screen,
                runtime->linked_screen, runtime->stack_base, service, p, sizeof *p,
                q.args[0], memory_service)) return 0;
        p->focus_effects[p->focus_effect_count++] = effects;
    }
    memset(p->main_menu.pending, 0, sizeof p->main_menu.pending);
    p->main_menu.pending_count = 0;
    return 1;
}

int nflstreet_field_screen_runtime_event(void *context, unsigned event) {
    field_runtime *runtime = context;
    if (!runtime || !runtime->source || !runtime->state || event < 2 || event > 7 ||
        runtime->source_size < 32 || runtime->source_size > SCREEN_MAX ||
        runtime->state->main_menu.pending_count > 128) return 0;
    uint8_t *copy = malloc(runtime->source_size);
    probe *next = malloc(sizeof *next);
    if (!copy || !next) { free(copy); free(next); return 0; }
    probe *persistent = runtime->state;
    memcpy(copy, runtime->source, runtime->source_size);
    memcpy(next, persistent, sizeof *next);
    unsigned first = next->main_menu.pending_count, executed = 0;
    uint32_t one = 0x3f800000u;
    int ok = nflstreet_ui_events_memory_context(copy, runtime->source_size,
        SCREEN_BASE, runtime->current_screen, runtime->linked_screen,
        runtime->stack_base, &next->main_menu.entry_flags, 0, (uint16_t)event,
        0, 1, &one, service, next, sizeof *next, &executed, NULL, memory_service);
    field_runtime private_runtime = *runtime;
    private_runtime.state = next;
    if (ok) ok = drain_field_focus(copy, runtime->source_size, &private_runtime, first);
    if (ok) {
        memcpy(runtime->source, copy, runtime->source_size);
        ++next->runtime_event_count;
        next->last_runtime_event = event;
        memcpy(persistent, next, sizeof *persistent);
    }
    free(copy);
    free(next);
    return ok;
}

static int field_tick_script(void *context, nflstreet_ui_rates *rates,
        uint8_t *bytes, size_t size, uint32_t base, uint32_t screen,
        uint32_t root, uint32_t pc, unsigned argc, const uint32_t *args,
        float *result) {
    field_runtime *runtime = context;
    if (!runtime || !runtime->state || screen != runtime->current_screen ||
        argc > 16 || !result) return 0;
    probe *p = runtime->state;
    p->main_menu.rates = *rates;
    nflstreet_ui_script vm = {0};
    vm.current_screen = runtime->current_screen;
    vm.linked_screen = runtime->linked_screen;
    vm.root = root;
    vm.pc = pc;
    vm.flags = p->main_menu.entry_flags;
    memcpy(vm.words + vm.sp++, result, sizeof *result);
    for (unsigned i = 0; i < argc; ++i) vm.words[vm.sp++] = args[i];
    vm.words[vm.sp++] = root;
    vm.words[vm.sp++] = 0;
    if (!nflstreet_ui_script_services_memory(bytes, size, base,
            runtime->stack_base, &vm, 100000, service, p, sizeof *p,
            NULL, memory_service)) return 0;
    p->main_menu.entry_flags = vm.flags;
    *rates = p->main_menu.rates;
    memcpy(result, vm.words, sizeof *result);
    return 1;
}

int nflstreet_field_screen_runtime_tick(void *context, uint32_t elapsed) {
    field_runtime *runtime = context;
    if (!runtime || !runtime->source || !runtime->state || !elapsed ||
        runtime->source_size < 32 || runtime->source_size > SCREEN_MAX ||
        runtime->state->main_menu.pending_count > 128) return 0;
    uint8_t *copy = malloc(runtime->source_size);
    probe *next = malloc(sizeof *next);
    if (!copy || !next) { free(copy); free(next); return 0; }
    memcpy(copy, runtime->source, runtime->source_size);
    memcpy(next, runtime->state, sizeof *next);
    unsigned first = next->main_menu.pending_count;
    field_runtime private_runtime = *runtime;
    private_runtime.state = next;
    int ok = nflstreet_ui_rate_update(&next->main_menu.rates, copy,
        runtime->source_size, SCREEN_BASE, runtime->current_screen, elapsed,
        0, field_tick_script, &private_runtime);
    if (ok) ok = drain_field_focus(copy, runtime->source_size, &private_runtime, first);
    if (ok && next->supplied_transition) {
        nflstreet_team_transition_completion completion = {0};
        next->transition_clock += elapsed;
        ok = nflstreet_team_transition_advance(next->transition_object,
            sizeof next->transition_object, next->transition_clock,
            next->transition_audio, &completion);
        if (ok && completion.completed) {
            if (next->transition_completion_count >= 16) ok = 0;
            else next->transition_completions[next->transition_completion_count++] = completion;
        }
    }
    if (ok) {
        memcpy(runtime->source, copy, runtime->source_size);
        ++next->runtime_tick_count;
        memcpy(runtime->state, next, sizeof *next);
    }
    free(next);
    free(copy);
    return ok;
}

int nflstreet_field_screen_runtime_finish(nflstreet_field_screen_runtime *runtime) {
    if (!runtime || !runtime->source || !runtime->state ||
        runtime->source_size < 32 || runtime->source_size > SCREEN_MAX ||
        runtime->state->runtime_finish_completed ||
        !runtime->state->transition_completion_count ||
        !runtime->state->main_menu.link_enabled ||
        !runtime->state->linked_route_observed ||
        runtime->state->linked_route_target != runtime->linked_screen ||
        runtime->linked_screen == UINT32_MAX ||
        runtime->state->field_cache_release_requests ||
        runtime->state->field_commit_request_count) return 0;
    uint8_t *copy = malloc(runtime->source_size);
    probe *next = malloc(sizeof *next);
    if (!copy || !next) { free(copy); free(next); return 0; }
    memcpy(copy, runtime->source, runtime->source_size);
    memcpy(next, runtime->state, sizeof *next);
    next->runtime_finish_in_progress = 1;
    unsigned executed = 0, first = next->main_menu.pending_count;
    int ok = nflstreet_ui_events_memory_context(copy, runtime->source_size,
        SCREEN_BASE, runtime->current_screen, runtime->linked_screen,
        runtime->stack_base, &next->main_menu.entry_flags, 0, 0xfffd,
        0, 0, NULL, service, next, sizeof *next, &executed, NULL, memory_service);
    if (ok) {
        field_runtime private_runtime = *runtime;
        private_runtime.state = next;
        ok = drain_field_focus(copy, runtime->source_size, &private_runtime, first);
    }
    if (ok && (next->field_cache_release_requests != 1 ||
        (next->supplied_finish_owner_value == 1 &&
         next->field_commit_request_count != 1) ||
        (next->supplied_finish_owner_value != 1 &&
         next->field_commit_request_count != 0))) ok = 0;
    if (ok) {
        next->runtime_finish_in_progress = 0;
        next->runtime_finish_completed = 1;
        memcpy(runtime->source, copy, runtime->source_size);
        memcpy(runtime->state, next, sizeof *next);
    }
    free(next);
    free(copy);
    return ok;
}

int nflstreet_field_screen_runtime_start(uint8_t *bytes, size_t size, probe *state,
        uint32_t current, uint32_t linked, uint32_t stack, uint32_t load_argument,
        unsigned argc, const uint32_t *args) {
    if (!bytes || !state || argc > 16 || (argc && !args) || size < 32) return 0;
    uint8_t *copy = malloc(size);
    probe *next = malloc(sizeof *next);
    if (!copy || !next) { free(copy); free(next); return 0; }
    memcpy(copy, bytes, size);
    memcpy(next, state, sizeof *next);
    /* Route ownership is a supplied host context. Capture source link requests
       for this route without executing the external screen transition. */
    next->main_menu.link_enabled = linked != UINT32_MAX;
    next->main_menu.linked_screen = linked;
    /* This field owner passes its supplied scheduler clock and current-screen
       identity into the bounded 80137B84 model. Audio remains explicitly off. */
    next->supplied_transition = 1;
    if (!next->transition_clock) next->transition_clock = next->main_menu.rates.clock;
    next->transition_audio = 0;
    next->transition_context = current;
    unsigned executed = 0;
    int ok = nflstreet_ui_events_memory_context(copy, size, SCREEN_BASE,
        current, linked, stack, &next->main_menu.entry_flags, 0, 0xfffe,
        load_argument, argc, args, service, next, sizeof *next, &executed,
        NULL, memory_service);
    uint32_t rows_address = be32(copy + 4);
    uint32_t root = 0;
    if (ok && rows_address >= SCREEN_BASE && rows_address - SCREEN_BASE <= size &&
        size - (rows_address - SCREEN_BASE) >= 20)
        root = be32(copy + rows_address - SCREEN_BASE);
    else ok = 0;
    if (ok && (root < SCREEN_BASE || root - SCREEN_BASE > size ||
               size - (root - SCREEN_BASE) < 8)) ok = 0;
    if (ok) {
        memset(copy + root - SCREEN_BASE + 4, 0, 4);
        copy[root - SCREEN_BASE + 7] = 1;
        ok = nflstreet_ui_events_memory_context(copy, size, SCREEN_BASE,
            current, linked, stack, &next->main_menu.entry_flags, 0, 0xfffc,
            0, 0, NULL, service, next, sizeof *next, &executed, NULL,
            memory_service);
    }
    if (ok) {
        field_runtime private_runtime = {copy, size, next, current, linked, stack};
        ok = drain_field_focus(copy, size, &private_runtime, 0);
    }
    if (ok) {
        next->startup_fffe_completed = 1;
        next->startup_fffc_completed = 1;
        memcpy(bytes, copy, size);
        memcpy(state, next, sizeof *next);
    }
    free(next);
    free(copy);
    return ok;
}

int nflstreet_field_screen_runtime_key_for_slot(
        const nflstreet_field_screen_state *state, uint32_t ui_slot,
        uint32_t *raw_key) {
    if (!state || !raw_key || ui_slot >= 8) return 0;
    *raw_key = state->field_bootstrap.keys[ui_slot];
    return 1;
}

static int owned_word(uint8_t *bytes, size_t size, uint32_t base,
                      uint32_t stack_base, uint32_t words[256],
                      uint32_t address, uint32_t *value, int write) {
    if (address >= base && address - base <= size && size - (address - base) >= 4) {
        uint8_t *p = bytes + address - base;
        if (write) { p[0] = (uint8_t)(*value >> 24); p[1] = (uint8_t)(*value >> 16);
                     p[2] = (uint8_t)(*value >> 8); p[3] = (uint8_t)*value; }
        else *value = be32(p);
        return 1;
    }
    if (address >= stack_base && address - stack_base <= 1020 &&
        !((address - stack_base) & 3)) {
        unsigned index = (address - stack_base) / 4;
        if (write) words[index] = *value; else *value = words[index];
        return 1;
    }
    return 0;
}

static int owned_byte(uint8_t *bytes, size_t size, uint32_t base,
                      uint32_t stack_base, uint32_t words[256],
                      uint32_t address, uint8_t *value, int write) {
    if (address >= base && address - base < size) {
        if (write) bytes[address - base] = *value;
        else *value = bytes[address - base];
        return 1;
    }
    if (address >= stack_base && address - stack_base < 1024) {
        unsigned offset = address - stack_base;
        unsigned index = offset / 4, shift = 24 - (offset & 3u) * 8;
        uint32_t mask = 0xffu << shift;
        if (write) words[index] = (words[index] & ~mask) | ((uint32_t)*value << shift);
        else *value = (uint8_t)(words[index] >> shift);
        return 1;
    }
    return 0;
}

static int bounded_output(uint8_t *bytes, size_t size, uint32_t base,
                          uint32_t stack_base, uint32_t words[256],
                          uint32_t descriptor, const char *text) {
    uint32_t capacity = 0, destination = 0;
    if (descriptor > UINT32_MAX - 8 ||
        !owned_word(bytes, size, base, stack_base, words, descriptor + 4,
                    &capacity, 0) || descriptor > UINT32_MAX - 8 ||
        !owned_word(bytes, size, base, stack_base, words, descriptor + 8,
                    &destination, 0) || capacity >= 1024 ||
        destination > UINT32_MAX - capacity) return 0;
    uint32_t remaining = capacity + 1;
    if (!remaining) return 1;
    while (remaining) {
        --remaining;
        if (!remaining) {
            uint8_t nul = 0;
            return owned_byte(bytes, size, base, stack_base, words,
                              destination, &nul, 1);
        }
        uint8_t ch = (uint8_t)*text++;
        if (!owned_byte(bytes, size, base, stack_base, words,
                        destination++, &ch, 1)) return 0;
        if (ch == 0) return 1;
    }
    return 1;
}

static int memory_service(void *context, uint8_t *bytes, size_t size, uint32_t base,
                          const nflstreet_ui_service_request *q, uint32_t *result,
                          uint32_t stack_base, uint32_t words[256]) {
    probe *p = context;
    if (!p || !q || !result || !words) return 0;
    if (q->kind == 1 && q->property == 0x0au) {
        uint32_t port;
        if (q->argc != 1 || p->supplied_port_mapping_mask > 1023 ||
            !owned_word(bytes, size, base, stack_base, words,
                        q->args[0], &port, 0) || port >= 10 ||
            !(p->supplied_port_mapping_mask & (1u << port))) return 0;
        uint32_t mapped = p->port_mapping[port];
        return owned_word(bytes, size, base, stack_base, words,
                          q->args[0], &mapped, 1);
    }
    if (q->kind == 1 && q->property == 0x80000004u && q->argc == 6) {
        if (q->args[0] >= 8) return 0;
        nflstreet_field_cache_output row;
        memset(&row, 0, sizeof row);
        if (!nflstreet_field_cache_read(p->field_bootstrap.records,
                                         sizeof p->field_bootstrap.records, 8,
                                         q->args[0], &row)) return 0;
        if (!bounded_output(bytes, size, base, stack_base, words, q->args[1], row.text0) ||
            !bounded_output(bytes, size, base, stack_base, words, q->args[2], row.text21) ||
            !bounded_output(bytes, size, base, stack_base, words, q->args[3], row.text42)) return 0;
        uint32_t raw_c9 = row.value_c9;
        if (!owned_word(bytes, size, base, stack_base, words, q->args[4], &raw_c9, 1) ||
            !bounded_output(bytes, size, base, stack_base, words, q->args[5], row.text99)) return 0;
        p->supplied_selected_index = q->args[0];
        ++p->cache_read_requests;
        return 1;
    }
    if (q->kind == 1 && q->property == 0x0f && q->argc == 1) {
        uint32_t dimensions = 0;
        if (!owned_word(bytes, size, base, stack_base, words, q->args[0],
                        &dimensions, 0) || dimensions > 8 ||
            q->args[0] > UINT32_MAX - (dimensions + 5u) * 4u) return 0;
        uint32_t address = q->args[0] + (dimensions + 1u) * 4u;
        for (unsigned i = 0; i < 4; ++i) {
            uint32_t value = p->controller_sides[i];
            if (!owned_word(bytes, size, base, stack_base, words,
                            address + i * 4, &value, 0)) return 0;
        }
        for (unsigned i = 0; i < 4; ++i) {
            uint32_t value = p->controller_sides[i];
            if (!owned_word(bytes, size, base, stack_base, words,
                            address + i * 4, &value, 1)) return 0;
        }
        ++p->controller_projection_calls;
        return 1;
    }
    return service(context, bytes, size, base, q, result);
}

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}

static int parse_u32(const char *text, uint32_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno || !end || *end || value > UINT32_MAX) return 0;
    *out = (uint32_t)value;
    return 1;
}

static int parse_i32(const char *text, int32_t *out) {
    char *end = NULL;
    errno = 0;
    long long value = strtoll(text, &end, 0);
    if (errno || !end || *end || value < INT32_MIN || value > INT32_MAX) return 0;
    *out = (int32_t)value;
    return 1;
}

static uint8_t *load_file(const char *path, size_t *size, size_t maximum) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return NULL; }
    long length = ftell(f);
    if (length < 1 || (unsigned long)length > maximum || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return NULL;
    }
    uint8_t *bytes = malloc((size_t)length);
    if (!bytes || fread(bytes, 1, (size_t)length, f) != (size_t)length) {
        free(bytes);
        fclose(f);
        return NULL;
    }
    if (fclose(f)) { free(bytes); return NULL; }
    *size = (size_t)length;
    return bytes;
}

static int write_file(const char *path, const uint8_t *bytes, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(bytes, 1, size, f) == size;
    if (fclose(f)) ok = 0;
    return ok;
}

static int load_font(probe *p, const char *directory, unsigned index) {
    char path[4096];
    int written = snprintf(path, sizeof path, "%s\\%05u.bin", directory, index);
    if (written < 0 || (size_t)written >= sizeof path) return 0;
    size_t size = 0;
    uint8_t *bytes = load_file(path, &size, 16u * 1024u * 1024u);
    if (!bytes) return 0;
    if (!nflstreet_font_open(bytes, size, &p->fonts[index])) {
        free(bytes);
        return 0;
    }
    return 1;
}

static int service(void *context, uint8_t *bytes, size_t size, uint32_t base,
                   const nflstreet_ui_service_request *q, uint32_t *result) {
    probe *p = context;
    if (!p || !q || !result) return 0;
    if (q->kind == 0 && q->property == UINT32_MAX) {
        return 0;
    }
    if (q->kind == 0 && q->class_id == 1 && q->property == 0 && q->argc == 1 &&
        q->target != 0)
        return nflstreet_ui_main_service(&p->main_menu, bytes, size, base, q, result);
    if (q->kind == 2 || q->kind == 7 || q->kind == 8) {
        int ok = nflstreet_ui_main_service(&p->main_menu, bytes, size, base, q, result);
        if (ok && q->kind == 8 && q->property == 3 && q->argc == 0 &&
            p->main_menu.link_enabled && q->target == p->main_menu.linked_screen) {
            p->linked_route_observed = 1;
            p->linked_route_target = q->target;
        }
        return ok;
    }
    if (q->kind == 0 && q->class_id == 8 && q->property == 0xfffffffcU &&
        q->argc == 1) return 1;
    if (q->kind == 0 && q->class_id == 0 && q->property == 0xfffffffcU &&
        q->argc == 1) {
        unsigned requested = 0;
        uint32_t identity = 0;
        return nflstreet_ui_image_construct(bytes, size, base, q->target,
                                             &requested, &identity);
    }
    if (q->kind == 0 && q->class_id == 0 && q->property == 0 && q->argc == 4) {
        if (q->target < base || q->target - base > size ||
            size - (q->target - base) < 76 || q->args[0] >= 4) return 0;
        uint8_t *object = bytes + q->target - base;
        unsigned index = q->args[0];
        for (unsigned axis = 1; axis < 4; ++axis) {
            float value;
            memcpy(&value, &q->args[axis], sizeof value);
            float scaled = value * 16.0f;
            if (!isfinite(scaled) || scaled < -2147483648.0f ||
                scaled >= 2147483648.0f) return 0;
            int32_t integer = (int32_t)scaled;
            unsigned offset = 52 + index * 6 + (axis - 1) * 2;
            object[offset] = (uint8_t)((uint32_t)integer >> 8);
            object[offset + 1] = (uint8_t)integer;
        }
        unsigned flags = ((unsigned)object[0] << 8 | object[1]) | 0x4000u;
        object[0] = (uint8_t)(flags >> 8);
        object[1] = (uint8_t)flags;
        return 1;
    }
    if (q->kind == 0 && q->class_id == 0 && q->property == 2 && q->argc == 4 &&
        q->args[0] == UINT32_MAX) {
        /* Exact NaN/sentinel branch of 801D60DC property2: copy the low bytes
           of args1..3 into four 3-byte rows at +0x14, +0x18, +0x1c, +0x20. */
        if (q->target < base || q->target - base > size ||
            size - (q->target - base) < 0x24) return 0;
        uint8_t *object = bytes + q->target - base;
        for (unsigned row = 0; row < 4; ++row) {
            unsigned offset = 0x14 + row * 4;
            object[offset] = (uint8_t)q->args[1];
            object[offset + 1] = (uint8_t)q->args[2];
            object[offset + 2] = (uint8_t)q->args[3];
        }
        unsigned flags = ((unsigned)object[0] << 8 | object[1]) | 0x4000u;
        object[0] = (uint8_t)(flags >> 8);
        object[1] = (uint8_t)flags;
        return 1;
    }
    if (q->kind == 0 && q->class_id == 0 && q->property == 3 && q->argc == 2) {
        if (q->target < base || q->target - base > size ||
            size - (q->target - base) < 36 ||
            (q->args[0] >= 4 && q->args[0] != UINT32_MAX)) return 0;
        uint8_t *object = bytes + q->target - base;
        unsigned first = q->args[0] == UINT32_MAX ? 0 : q->args[0];
        unsigned last = q->args[0] == UINT32_MAX ? 4 : first + 1;
        for (unsigned i = first; i < last; ++i) object[23 + i * 4] = (uint8_t)q->args[1];
        object[0] |= 0x40;
        return 1;
    }
    if (q->kind == 0 && q->class_id == 0 && q->property == 5 && q->argc == 1) {
        if (q->target < base || q->target - base > size ||
            size - (q->target - base) < 36) return 0;
        uint8_t *object = bytes + q->target - base;
        unsigned archive = q->args[0] >> 16, entry = q->args[0] & 0xffffu;
        unsigned old_archive = (unsigned)object[6] << 8 | object[7];
        unsigned old_entry = (unsigned)object[4] << 8 | object[5];
        if (archive == 0 || archive == 0xffffu) archive = old_archive;
        int archive_unavailable = archive != 0xffffu &&
            (archive >= 32 || !(p->supplied_static_archives & (1u << archive)));
        int old_archive_unavailable = old_archive != 0xffffu &&
            (old_archive >= 32 || !(p->supplied_static_archives & (1u << old_archive)));
        if (archive_unavailable || old_archive_unavailable) return 0;
        if (archive != old_archive || entry != old_entry) {
            unsigned additions = (old_archive != 0xffffu && old_entry != 0xffffu) +
                                 (archive != 0xffffu && entry != 0xffffu);
            if (additions > 128u - p->resource_effect_count) return 0;
            if (old_archive != 0xffffu && old_entry != 0xffffu)
                p->resource_effects[p->resource_effect_count++] =
                    (resource_effect){0, (uint16_t)old_archive, (uint16_t)old_entry};
            if (archive != 0xffffu && entry != 0xffffu)
                p->resource_effects[p->resource_effect_count++] =
                    (resource_effect){1, (uint16_t)archive, (uint16_t)entry};
            object[4] = (uint8_t)(entry >> 8); object[5] = (uint8_t)entry;
            object[6] = (uint8_t)(archive >> 8); object[7] = (uint8_t)archive;
        }
        object[12] = object[13] = object[14] = object[15] = 0xff;
        object[0] |= 0x40;
        return 1;
    }
    if (q->kind == 0 && q->class_id == 8 && q->property == 3 && q->argc == 1) {
        if (q->target < base || q->target - base > size ||
            size - (q->target - base) < 40 || q->args[0] < base ||
            q->args[0] - base > size || size - (q->args[0] - base) < 12) return 0;
        uint8_t *object = bytes + q->target - base;
        unsigned font = object[2];
        if (object[3] != 2 || font >= 4 || !p->fonts[font].bytes) return 0;
        uint32_t descriptor_address = be32(object + 32);
        if (descriptor_address < base || descriptor_address - base > size ||
            size - (descriptor_address - base) < 12) return 0;
        uint8_t *source_descriptor = bytes + q->args[0] - base;
        uint8_t *destination_descriptor = bytes + descriptor_address - base;
        uint32_t source = be32(source_descriptor + 8);
        uint32_t destination = be32(destination_descriptor + 8);
        uint32_t capacity = be32(destination_descriptor + 4);
        if (source < base || source - base >= size || destination < base ||
            destination - base > size || capacity >= size - (destination - base)) return 0;
        uint8_t *end = memchr(bytes + source - base, 0, size - (source - base));
        if (!end) return 0;
        size_t length = (size_t)(end - (bytes + source - base));
        if (length > capacity) return 0;
        memmove(bytes + destination - base, bytes + source - base, length + 1);
        if (!nflstreet_team_classes_prepare_text(&p->classes, bytes, size, base,
                                                  q->target)) return 0;
        ++p->text_updates;
        return 1;
    }
    if (q->kind == 0 && q->class_id == 8 && q->property == 5 && q->argc == 1) {
        /* The shared main-menu helper also handles this state-only property,
           but its entry guard assumes team font3. Field class8 objects use
           other valid font indices, so preserve the source class/type check
           and apply the same low status-bit update without the team-font test. */
        if (q->target >= base && q->target - base <= size &&
            size - (q->target - base) >= 40) {
            uint8_t *object = bytes + q->target - base;
            if (object[3] == 2) {
                unsigned flags = ((unsigned)object[0] << 8 | object[1]) & 0xfffcu;
                if ((int32_t)q->args[0] >= 0)
                    flags |= q->args[0] == 0 ? 1u : 2u;
                object[0] = (uint8_t)(flags >> 8);
                object[1] = (uint8_t)flags;
                return 1;
            }
        }
        return nflstreet_ui_main_service(&p->main_menu, bytes, size, base, q, result);
    }
    if (q->kind == 0 && q->class_id == 8 && q->property == 12 && q->argc == 1 &&
        q->target >= base && q->target - base <= size &&
        size - (q->target - base) >= 40) {
        uint8_t *object = bytes + q->target - base;
        unsigned font_index = object[2];
        uint32_t descriptor = be32(object + 32);
        if (object[3] != 2 || font_index >= 4 || !p->fonts[font_index].bytes ||
            descriptor < base || descriptor - base > size ||
            size - (descriptor - base) < 12) return 0;
        uint8_t *desc = bytes + descriptor - base;
        uint32_t text_address = be32(desc + 8);
        if (text_address < base || text_address - base >= size) return 0;
        uint8_t *text = bytes + text_address - base;
        uint8_t *end = memchr(text, 0, size - (text_address - base));
        if (!end || object[15] != 1 ||
            (((unsigned)object[36] << 8) | object[37]) != 0) return 0;
        unsigned length = (unsigned)(end - text), index = q->args[0];
        if (index && !length) return 0;
        if (index && index >= length) index = length - 1;
        unsigned glyph_count = ((unsigned)object[38] << 8) | object[39];
        int32_t width = 0;
        for (unsigned i = 0; i < index && i < glyph_count; ++i) {
            const uint8_t *glyph = nflstreet_font_glyph(&p->fonts[font_index], text[i]);
            if (glyph) width += (int8_t)glyph[8];
        }
        float value = (float)(int16_t)width;
        memcpy(result, &value, sizeof value);
        return 1;
    }
    if (q->kind == 1 && q->property == 0x80000001u && q->argc == 0) {
        /* The callback branch 8011F4AC property1 builds/clears the eight-row
           cache. This probe's records are already built by field_catalog from
           RVNE; only this bounded cache-ready boundary is represented here. */
        ++p->cache_build_requests;
        *result = 0;
        return 1;
    }
    if (q->kind == 6) return nflstreet_ui_class_activation(bytes, size, base, q,
                                                              service, context);
    if (q->kind == 1 && q->property == 0x80000003u && q->argc == 0) {
        /* The callback returns the caller-supplied raw selected index. */
        ++p->selected_index_reads;
        *result = p->supplied_selected_index;
        return 1;
    }
    if (q->kind == 1 && q->property == 0x80000002u && q->argc == 0 &&
        p->runtime_finish_in_progress) {
        /* 8011F170 always releases the caller-owned field cache. Its mode==1
           branch additionally requests selected DIVE field setup; do not call
           801EA480 or claim match construction here. */
        if (p->field_cache_release_requests == UINT_MAX) return 0;
        if (p->supplied_finish_owner_value == 1) {
            if (p->field_commit_request_count >= 4 ||
                p->supplied_selected_index >= 8) return 0;
            p->field_commit_requests[p->field_commit_request_count++] =
                (nflstreet_field_commit_request){
                    p->supplied_selected_index,
                    p->field_bootstrap.keys[p->supplied_selected_index],
                    p->supplied_finish_owner_value
                };
        }
        ++p->field_cache_release_requests;
        return 1;
    }
    if (q->kind == 1 && q->property == 1 && q->argc == 0) {
        /* Original 8010C3BC property1 reads 8010BE48, whose checked body
           returns float2. Keep the callback result in its original float form. */
        float value = 2.0f;
        memcpy(result, &value, sizeof value);
        return 1;
    }
    if (q->kind == 1 && q->property == 0x80000006u && q->argc == 1) {
        /* The source's 0x67 branch invokes 80137B84(500). Use only the
           explicitly supplied, owned transition state; effects remain deferred. */
        if (q->args[0] == 0x67u) {
            if (p->supplied_transition != 1 || p->transition_request_count >= 16 ||
                !nflstreet_team_transition_apply(p->transition_object,
                    sizeof p->transition_object, 500, p->transition_clock,
                    p->transition_audio, p->transition_context,
                    &p->transition_requests[p->transition_request_count])) return 0;
            ++p->transition_request_count;
        }
        return 1;
    }
    if (q->kind == 1 && q->property == 0x80000005u && q->argc == 1) {
        /* 8011F4AC delegates this property to the original constant-return
           function 8011F4A4, which returns 1 without touching caller memory. */
        *result = 1;
        return 1;
    }
    if (q->kind == 3 || q->kind == 4 || q->kind == 5)
        return nflstreet_ui_rate_request(&p->main_menu.rates, bytes, size, base,
                                         8, q);
    if (q->kind == 1 && q->argc == 0 &&
        (q->property == 0xcb || q->property == 0x104 || q->property == 0x105)) {
        /* Diagnostic host stubs copied from the already-supplied team test;
           these are not claimed as original field-query results. */
        *result = 0;
        return 1;
    }
    /* The team source shares this accepted no-result callback branch. */
    if (q->kind == 1 && q->property == 0x135 && q->argc == 0) return 1;
    ++p->unsupported_calls;
    fprintf(stderr, "unsupported service kind=%u property=0x%08x class=%u argc=%u target=0x%08x",
            q->kind, q->property, q->class_id, q->argc, q->target);
    for (unsigned i = 0; i < q->argc; ++i) fprintf(stderr, " arg%u=0x%08x", i, q->args[i]);
    fputc('\n', stderr);
    return 0;
}

static void clear_probe(probe *p) {
    for (unsigned i = 0; i < 4; ++i) free((void *)p->fonts[i].bytes);
}

int nflstreet_field_screen_runtime_prepare(uint8_t *source, size_t source_size,
        const uint8_t *template_bytes, size_t template_size,
        const uint8_t *dol_bytes, size_t dol_size, const char *font_directory,
        nflstreet_field_screen_state *state, int32_t stat42, uint32_t c9,
        int no_profile, uint32_t selected_index, uint32_t static_archive_mask,
        uint32_t initial_rate_clock) {
    if (!source || !template_bytes || !dol_bytes || !font_directory || !state ||
        source_size < 32 || source_size > SCREEN_MAX || template_size == 0 ||
        dol_size == 0 || c9 > 255 || selected_index >= 8 ||
        (no_profile != 0 && no_profile != 1)) return 0;
    for (unsigned i = 0; i < 4; ++i) if (state->fonts[i].bytes) return 0;
    char text42[8][33];
    uint8_t c9_values[8];
    char formatted[33];
    if (!nflstreet_field_stat_text(stat42, formatted)) return 0;
    for (unsigned i = 0; i < 8; ++i) {
        memcpy(text42[i], formatted, sizeof formatted);
        c9_values[i] = (uint8_t)c9;
        if (no_profile) {
            uint8_t ifids[2];
            if (!nflstreet_field_availability_from_ivid(template_bytes,
                    template_size, (int8_t)i, &c9_values[i], ifids)) return 0;
        }
    }
    probe next;
    memcpy(&next, state, sizeof next);
    next.supplied_stat42 = stat42;
    memcpy(next.supplied_text42, formatted, sizeof formatted);
    next.supplied_c9 = (uint8_t)c9;
    next.c9_from_no_profile = (uint8_t)no_profile;
    next.supplied_selected_index = selected_index;
    next.supplied_static_archives = static_archive_mask;
    next.supplied_rate_clock = initial_rate_clock;
    next.transition_clock = initial_rate_clock;
    if (!nflstreet_field_catalog_from_template(template_bytes, template_size,
            text42, c9_values, next.fields) ||
        !nflstreet_field_bootstrap_from_dol(dol_bytes, dol_size, next.fields,
                                             &next.field_bootstrap)) return 0;
    for (unsigned i = 0; i < 4; ++i) if (!load_font(&next, font_directory, i)) {
        clear_probe(&next);
        return 0;
    }
    memcpy(next.classes.fonts, next.fonts, sizeof next.fonts);
    next.main_menu.font = next.fonts[3];
    next.main_menu.rates.clock = initial_rate_clock;
    uint8_t *copy = malloc(source_size);
    if (!copy) { clear_probe(&next); return 0; }
    memcpy(copy, source, source_size);
    unsigned relocated = 0;
    int ok = nflstreet_ui_relocate(copy, source_size, SCREEN_BASE, &relocated) &&
             relocated == 1 &&
             nflstreet_team_classes_construct(copy, source_size, SCREEN_BASE,
                                                &next.classes);
    if (ok) {
        next.image_objects = next.classes.class_requests[0];
        next.text_objects = next.classes.class_requests[8];
        next.image_requests = next.classes.image_requests;
        for (unsigned i = 0; i < next.classes.request_count; ++i) {
            const nflstreet_ui_service_request *request = &next.classes.requests[i];
            if (request->class_id != 0 || request->target < SCREEN_BASE ||
                request->target - SCREEN_BASE > source_size ||
                source_size - (request->target - SCREEN_BASE) < 8) continue;
            const uint8_t *object = copy + request->target - SCREEN_BASE;
            if (!(object[1] & 2)) continue;
            uint32_t identity = ((uint32_t)object[6] << 24) |
                ((uint32_t)object[7] << 16) | ((uint32_t)object[4] << 8) | object[5];
            if (next.image_id_count < 256)
                next.image_ids[next.image_id_count++] = identity;
        }
        memcpy(source, copy, source_size);
        memcpy(state, &next, sizeof next);
    } else clear_probe(&next);
    free(copy);
    return ok;
}

static int write_report(const char *path, size_t source_size, unsigned node_count,
                        uint32_t selected,
                        uint32_t event, uint32_t current, uint32_t linked,
                        uint32_t argument, unsigned argc, const uint32_t *args,
                        unsigned relocated, int constructed, int event_ok,
                        unsigned event_scripts, unsigned root_event_found,
                        unsigned root_event_pc, int preview_rendered,
                        const char *capture_path, const probe *p) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f,
        "format=nflstreet-field-screen-probe-v1\n"
        "boundary=private source entry8; native relocation and class0/class8 construction; bounded event service\n"
        "screen_base=0x%08x\nsource_size=%zu\nsource_nodes=%u\n"
        "relocation_result=%u\nconstructor_walk_completed=%u\nfull_constructor_claimed=0\n"
        "catalog=RVNE private template; records reordered by original DOL UI-slot table\n"
        "text42_raw_stat_input=%d\ntext42_input=original integer formatter result: %s\n"
        "C9_mode=%s\n",
        SCREEN_BASE, source_size, node_count, relocated, constructed,
        p->supplied_stat42, p->supplied_text42,
        p->c9_from_no_profile ? "actual IVID no-profile predicate" : "explicit caller override");
    fprintf(f, "C9_by_DIVE_key=");
    for (unsigned i = 0; i < 8; ++i)
        fprintf(f, "%s%u:0x%02x", i ? "," : "", i, p->fields[i].record[0xc9]);
    fprintf(f, "\n");
    fprintf(f,
        "selected_index_input=0x%08x\n"
        "field_slot_to_DIVE_keys=%u,%u,%u,%u,%u,%u,%u,%u\n"
        "static_archive_availability_mask=0x%08x (explicit caller input)\n"
        "initial_rate_clock=%u (explicit host input, not an original constant)\n"
        "controller_side_words=0x%08x,0x%08x,0x%08x,0x%08x (explicit caller projection)\n"
        "common_query_stubs=CB/104/105 return0;135 returns success without result; provisional diagnostic only\n"
        "event=0x%04x\nevent_execution=ordered child/self source walk with explicit VM context\n"
        "current_screen=0x%08x\nlinked_screen=0x%08x\n"
        "event_argument=0x%08x\nevent_argc=%u\n",
        selected,
        p->field_bootstrap.keys[0], p->field_bootstrap.keys[1],
        p->field_bootstrap.keys[2], p->field_bootstrap.keys[3],
        p->field_bootstrap.keys[4], p->field_bootstrap.keys[5],
        p->field_bootstrap.keys[6], p->field_bootstrap.keys[7],
        p->supplied_static_archives,
        p->supplied_rate_clock,
        p->controller_sides[0], p->controller_sides[1], p->controller_sides[2], p->controller_sides[3],
        event, current, linked, argument, argc);
    for (unsigned i = 0; i < argc; ++i) fprintf(f, "event_arg_%u=0x%08x\n", i, args[i]);
    fprintf(f,
        "root_event_found=%u\nroot_event_pc=0x%08x\nevent_completed=%u\nevent_scripts_executed=%u\n"
        "diagnostic_preview_rendered=%u\ndiagnostic_preview_path=%s\n"
        "class0_objects=%u\nclass8_text_objects_constructed=%u\n"
        "class8_line_plan_count=%u\nclass8_text_updates=%u\n"
        "image_acquisition_requests=%u\n"
        "image_request_ids=",
        root_event_found, root_event_pc, event_ok, event_scripts,
        preview_rendered, capture_path ? capture_path : "not-requested",
        p->image_objects, p->text_objects,
        p->classes.line_plan_count, p->text_updates, p->image_requests);
    for (unsigned i = 0; i < p->image_id_count; ++i)
        fprintf(f, "%s0x%08x", i ? "," : "", p->image_ids[i]);
    fprintf(f, "\npending_resource_effect_count=%u\nresource_effects_consumed=0\n",
            p->resource_effect_count);
    for (unsigned i = 0; i < p->resource_effect_count; ++i)
        fprintf(f, "resource_effect_%u=%s archive=%u entry=%u\n", i,
                p->resource_effects[i].acquire ? "acquire" : "release",
                p->resource_effects[i].archive, p->resource_effects[i].entry);
    fprintf(f,
        "\nstartup_fffe_completed=%u\nstartup_fffc_completed=%u\n"
        "runtime_event_count=%u\nlast_runtime_event=0x%08x\n"
        "runtime_tick_count=%u\nrate_record_count=%u\nrate_clock=%u\n"
        "transition_effect_count=%u\ntransition_clock_current=%u\ntransition_audio_input=%u\ntransition_context_input=0x%08x\n"
        "transition_completion_count=%u\ntransition_completion_external_effects_dispatched=0\n"
        "transition_external_effects_dispatched=0\nlinked_route_capture_enabled=%u\nlinked_route_request_count=%u\n"
        "pending_focus_count=%u\nfocus_effect_count=%u\n"
        "field_cache_build_callback_count=%u\nselected_index_callback_count=%u\n"
        "field_cache_read_callback_count=%u\n"
        "controller_projection_callback_count=%u\n"
        "unsupported_service_count=see-probe-stderr; rejected script/service state is rolled back atomically\n"
        "limitations=Field data and status arguments are bounded caller inputs; profile unlock state, resource I/O, selected-field coordinator commit, and original live rendering are not claimed.\n",
        p->startup_fffe_completed, p->startup_fffc_completed,
        p->runtime_event_count, p->last_runtime_event, p->runtime_tick_count,
        p->main_menu.rates.count, p->main_menu.rates.clock,
        p->transition_request_count, p->transition_clock, p->transition_audio,
        p->transition_context, p->transition_completion_count,
        p->main_menu.link_enabled, p->main_menu.link_count,
        p->main_menu.pending_count, p->focus_effect_count,
        p->cache_build_requests, p->selected_index_reads,
        p->cache_read_requests, p->controller_projection_calls);
    return fclose(f) == 0;
}

int main(int argc, char **argv) {
    if (argc < 18 || argc > 36) {
        fprintf(stderr, "usage: %s source8.bin template00002.bin main.dol font-dir output-dir stat42 C9|NO_PROFILE selected-index event current linked argument argc [event args...] side0 side1 side2 side3 [static-archive-mask [initial-rate-clock]]\n", argv[0]);
        return 2;
    }
    uint32_t selected, event, current, linked, argument, arg_count;
    uint32_t c9 = 0;
    int no_profile = strcmp(argv[7], "NO_PROFILE") == 0;
    if ((!no_profile && (!parse_u32(argv[7], &c9) || c9 > 255)) ||
        !parse_u32(argv[8], &selected) || selected >= 8 ||
        !parse_u32(argv[9], &event) || event > 65535 ||
        !parse_u32(argv[10], &current) || !parse_u32(argv[11], &linked) ||
        !parse_u32(argv[12], &argument) || !parse_u32(argv[13], &arg_count) ||
        arg_count > 16 || (argc != 18 + (int)arg_count &&
        argc != 19 + (int)arg_count && argc != 20 + (int)arg_count)) return 2;

    probe p;
    memset(&p, 0, sizeof p);
    if (!parse_i32(argv[6], &p.supplied_stat42) ||
        !nflstreet_field_stat_text(p.supplied_stat42, p.supplied_text42)) return 2;
    p.c9_from_no_profile = (uint8_t)no_profile;
    p.supplied_c9 = (uint8_t)c9;
    uint32_t args[16] = {0};
    for (unsigned i = 0; i < arg_count; ++i) if (!parse_u32(argv[14 + i], &args[i])) return 2;
    for (unsigned i = 0; i < 4; ++i)
        if (!parse_u32(argv[14 + arg_count + i], &p.controller_sides[i])) return 2;
    if ((argc == 19 + (int)arg_count || argc == 20 + (int)arg_count) &&
        !parse_u32(argv[18 + arg_count], &p.supplied_static_archives)) return 2;
    if (argc == 20 + (int)arg_count &&
        !parse_u32(argv[19 + arg_count], &p.supplied_rate_clock)) return 2;

    size_t source_size = 0, template_size = 0, dol_size = 0;
    uint8_t *source = load_file(argv[1], &source_size, SCREEN_MAX);
    uint8_t *template_bytes = load_file(argv[2], &template_size, 128u * 1024u * 1024u);
    uint8_t *dol_bytes = load_file(argv[3], &dol_size, 64u * 1024u * 1024u);
    if (!source || !template_bytes || !dol_bytes || source_size < 32 || be32(source) > 1024) {
        free(source); free(template_bytes); free(dol_bytes); return 3;
    }
    p.supplied_selected_index = selected;
    char text42[8][33];
    uint8_t c9_values[8];
    for (unsigned i = 0; i < 8; ++i) {
        memcpy(text42[i], p.supplied_text42, sizeof text42[i]);
        c9_values[i] = (uint8_t)c9;
        if (no_profile) {
            uint8_t ifids[2];
            if (!nflstreet_field_availability_from_ivid(template_bytes, template_size,
                    (int8_t)i, &c9_values[i], ifids)) {
                fprintf(stderr, "no-profile IVID predicate rejected DIVE key %u\n", i);
                free(source); free(template_bytes); free(dol_bytes); return 4;
            }
        }
    }
    if (!nflstreet_field_catalog_from_template(template_bytes, template_size,
                                                text42, c9_values, p.fields)) {
        fprintf(stderr, "RVNE catalog initialization rejected\n");
        free(source); free(template_bytes); free(dol_bytes); return 4;
    }
    free(template_bytes);
    if (!nflstreet_field_bootstrap_from_dol(dol_bytes, dol_size, p.fields,
                                             &p.field_bootstrap)) {
        fprintf(stderr, "DOL field-slot bootstrap rejected\n");
        free(source); free(dol_bytes); return 4;
    }
    free(dol_bytes);
    for (unsigned i = 0; i < 4; ++i) if (!load_font(&p, argv[4], i)) {
        fprintf(stderr, "font%u could not be opened\n", i);
        clear_probe(&p); free(source); return 5;
    }

    unsigned relocated = 0;
    if (!nflstreet_ui_relocate(source, source_size, SCREEN_BASE, &relocated) ||
        relocated != 1) {
        fprintf(stderr, "source relocation failed or source was already relocated\n");
        clear_probe(&p); free(source); return 6;
    }
    memcpy(p.classes.fonts, p.fonts, sizeof p.fonts);
    p.main_menu.font = p.fonts[3];
    p.main_menu.rates.clock = p.supplied_rate_clock;
    uint8_t *construction_copy = malloc(source_size);
    nflstreet_team_classes classes_next = p.classes;
    int constructed = 0;
    if (construction_copy) {
        memcpy(construction_copy, source, source_size);
        constructed = nflstreet_team_classes_construct(construction_copy, source_size,
                                                        SCREEN_BASE, &classes_next);
        if (constructed) {
            memcpy(source, construction_copy, source_size);
            p.classes = classes_next;
            p.image_objects = p.classes.class_requests[0];
            p.text_objects = p.classes.class_requests[8];
            p.image_requests = p.classes.image_requests;
            for (unsigned i = 0; i < p.classes.request_count; ++i) {
                const nflstreet_ui_service_request *request = &p.classes.requests[i];
                if (request->class_id != 0 || request->target < SCREEN_BASE ||
                    request->target - SCREEN_BASE > source_size ||
                    source_size - (request->target - SCREEN_BASE) < 8) continue;
                const uint8_t *object = source + request->target - SCREEN_BASE;
                if (!(object[1] & 2)) continue;
                uint32_t identity = ((uint32_t)object[6] << 24) |
                    ((uint32_t)object[7] << 16) | ((uint32_t)object[4] << 8) | object[5];
                if (p.image_id_count < sizeof p.image_ids / sizeof p.image_ids[0])
                    p.image_ids[p.image_id_count++] = identity;
            }
        }
        free(construction_copy);
    }
    if (!constructed) fprintf(stderr, "bounded class0/class8 construction rejected\n");

    char path[4096];
    int n = snprintf(path, sizeof path, "%s\\field-screen-constructed.bin", argv[5]);
    if (n < 0 || (size_t)n >= sizeof path || !write_file(path, source, source_size)) {
        clear_probe(&p); free(source); return 7;
    }

    unsigned scripts = 0;
    unsigned root_event_pc = 0, root_event_found = 0;
    int root_lookup_ok = nflstreet_ui_event(source, source_size, SCREEN_BASE,
        0, 0, (uint16_t)event, 0, &root_event_pc, &root_event_found);
    field_runtime runtime = {source, source_size, &p, current, linked, VM_STACK_BASE};
    int event_ok = 0;
    if (constructed && event == 0xfffe)
        event_ok = nflstreet_field_screen_runtime_start(source, source_size, &p, current, linked,
            VM_STACK_BASE, argument, (unsigned)arg_count, args);
    else if (constructed && event >= 2 && event <= 7)
        event_ok = nflstreet_field_screen_runtime_event(&runtime, event);
    if (constructed && !event_ok) {
        uint8_t *diag_source = malloc(source_size);
        probe *diag_state = malloc(sizeof *diag_state);
        if (diag_source && diag_state) {
            unsigned count = be32(source);
            if (count > 64) count = 64;
            for (unsigned i = 0; i < count; ++i) {
                memcpy(diag_source, source, source_size);
                memcpy(diag_state, &p, sizeof *diag_state);
                uint32_t diag_flags = 0;
                unsigned diag_scripts = 0;
                int ok = nflstreet_ui_events_memory_context(diag_source, source_size,
                    SCREEN_BASE, current, linked, VM_STACK_BASE, &diag_flags, i,
                    (uint16_t)event, argument, (unsigned)arg_count, args, service,
                    diag_state, sizeof *diag_state, &diag_scripts, NULL, memory_service);
                fprintf(stderr, "field event node probe index=%u completed=%u scripts=%u\n",
                        i, ok, diag_scripts);
                unsigned pc = 0, found = 0;
                if (nflstreet_ui_event(diag_source, source_size, SCREEN_BASE, 0, i,
                                       (uint16_t)event, 0, &pc, &found) && found) {
                    uint32_t rows = be32(diag_source + 4);
                    uint32_t root = be32(diag_source + rows - SCREEN_BASE + i * 20u);
                    nflstreet_ui_script vm = {0};
                    vm.current_screen = current; vm.linked_screen = linked;
                    vm.root = root; vm.pc = SCREEN_BASE + pc;
                    vm.words[vm.sp++] = 0; vm.words[vm.sp++] = argument;
                    for (unsigned a = 0; a < arg_count; ++a) vm.words[vm.sp++] = args[a];
                    vm.words[vm.sp++] = root; vm.words[vm.sp++] = 0;
                    int direct = nflstreet_ui_script_services_memory(diag_source,
                        source_size, SCREEN_BASE, VM_STACK_BASE, &vm, 100000,
                        service, diag_state, sizeof *diag_state, NULL, memory_service);
                    fprintf(stderr, "field event direct probe index=%u found=1 pc=0x%x completed=%u\n",
                            i, pc, direct);
                }
            }
        }
        free(diag_source);
        free(diag_state);
    }
    if (event_ok) {
        n = snprintf(path, sizeof path, "%s\\field-screen-after-event.bin", argv[5]);
        if (n < 0 || (size_t)n >= sizeof path || !write_file(path, source, source_size)) {
            clear_probe(&p); free(source); return 8;
        }
    }
    unsigned preview_rendered = 0;
    char capture_path[4096] = {0};
    const char *capture_prefix = getenv("DAYWATER_FIELD_CAPTURE_PREFIX");
    if (constructed && capture_prefix &&
        strncmp(capture_prefix, "W:\\Daywater\\", 12) == 0) {
        int z = snprintf(capture_path, sizeof capture_path,
                         "%s\\field-screen-initial.bmp", capture_prefix);
        char asset_root[4096];
        size_t root_len = strlen(argv[4]);
        if (root_len < sizeof asset_root) {
            memcpy(asset_root, argv[4], root_len + 1);
            char *slash = strrchr(asset_root, '\\');
            char *forward = strrchr(asset_root, '/');
            if (!slash || (forward && forward > slash)) slash = forward;
            if (slash) *slash = 0;
            if (z > 0 && (size_t)z < sizeof capture_path && slash &&
                nflstreet_field_screen_preview_render(source, source_size,
                asset_root, &p.classes, capture_path)) preview_rendered = 1;
        }
    }
    if (constructed && event_ok && capture_prefix &&
        strncmp(capture_prefix, "W:\\Daywater\\", 12) == 0 &&
        getenv("DAYWATER_FIELD_LIVE_PREVIEW")) {
        char asset_root[4096];
        size_t root_len = strlen(argv[4]);
        if (root_len < sizeof asset_root) {
            memcpy(asset_root, argv[4], root_len + 1);
            char *slash = strrchr(asset_root, '\\');
            char *forward = strrchr(asset_root, '/');
            if (!slash || (forward && forward > slash)) slash = forward;
            if (slash) *slash = 0;
            if (slash) (void)nflstreet_field_screen_preview_live(source, source_size,
                asset_root, &p.classes, nflstreet_field_screen_runtime_event,
                nflstreet_field_screen_runtime_tick, &runtime);
        }
    }
    n = snprintf(path, sizeof path, "%s\\field-screen-probe.txt", argv[5]);
    if (n < 0 || (size_t)n >= sizeof path ||
        !write_report(path, source_size, be32(source),
                      selected, event, current, linked, argument, (unsigned)arg_count,
                      args, relocated, constructed, event_ok, scripts,
                      root_lookup_ok ? root_event_found : 0,
                      root_lookup_ok ? root_event_pc : 0, preview_rendered,
                      capture_path[0] ? capture_path : NULL, &p)) {
        clear_probe(&p); free(source); return 9;
    }
    printf("Field source probe: relocate=%u constructorWalk=%s fullConstruction=unclaimed event=0x%04x %s scripts=%u imageRequests=%u cacheBuild=%u selectedReads=%u rejection=see-stderr\n",
        relocated, constructed ? "ok" : "rejected", event,
        event_ok ? "completed" : "bounded/rejected", scripts, p.image_requests,
        p.cache_build_requests, p.selected_index_reads);
    clear_probe(&p);
    free(source);
    return constructed && event_ok ? 0 : 10;
}
