/* Optional private fixture exporter. Include the ordinary startup probe so
   construction and activation stay exactly the same as the checked path. */
#include "team_screen.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char fixture_directory[4096];
static unsigned fixture_frames;
static int logical_port0_mode;
static int focus_probe_mode;
static int dispatcher_probe_mode;
static int export_fixture(uint8_t *, size_t, nflstreet_team_screen *, const char *);
static int prepare_host(nflstreet_team_screen *);
#define DAYWATER_TEAM_STARTUP_CAPTURE export_fixture
#define DAYWATER_TEAM_STARTUP_HOST prepare_host
#define main team_screen_probe_main
#include "team_screen_probe.c"
#undef main

static int write_file(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(data, 1, size, f) == size;
    if (fclose(f)) ok = 0;
    return ok;
}

static uint32_t read_be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}

static int prepare_host(nflstreet_team_screen *s) {
    if (!logical_port0_mode) return 1;
    nflstreet_team_device_record records[] = {{0, 0}};
    uint32_t mapping[NFLSTREET_TEAM_DEVICE_MAPPING_COUNT];
    for (unsigned i = 0; i < NFLSTREET_TEAM_DEVICE_MAPPING_COUNT; ++i)
        mapping[i] = UINT32_MAX;
    mapping[0] = 0;
    return nflstreet_team_screen_devices(s, records, 1, mapping);
}

static void write_dispatch_nodes(FILE *f, const uint8_t *bytes, size_t size) {
    uint32_t count = read_be32(bytes), address = read_be32(bytes + 4);
    fprintf(f, "  \"event_dispatch_nodes\": [");
    if (address < 0x81100000u || address - 0x81100000u > size ||
        (size_t)count * 20 > size - (address - 0x81100000u)) count = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *row = bytes + (address - 0x81100000u) + i * 20;
        uint32_t root = read_be32(row);
        if (root < 0x81100000u || root - 0x81100000u > size ||
            size - (root - 0x81100000u) < 0x64) continue;
        const uint8_t *r = bytes + root - 0x81100000u;
        uint32_t g4 = read_be32(r + 4), g60 = read_be32(r + 0x60);
        if (i) fputc(',', f);
        fprintf(f, "{\"node\":%u,\"selected_byte\":%u,\"dispatch_gate_4\":%u,\"dispatch_gate_60\":%u,\"directional_gate_open\":%s}",
                i, r[7], g4, g60, g4 && g60 ? "true" : "false");
    }
    fprintf(f, "],\n  \"directional_dispatch_gate_open_nodes\": [");
    int first = 1;
    if (address >= 0x81100000u && address - 0x81100000u <= size &&
        (size_t)count * 20 <= size - (address - 0x81100000u)) {
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t *row = bytes + (address - 0x81100000u) + i * 20;
            uint32_t root = read_be32(row);
            if (root < 0x81100000u || root - 0x81100000u > size ||
                size - (root - 0x81100000u) < 0x64) continue;
            const uint8_t *r = bytes + root - 0x81100000u;
            if (read_be32(r + 4) && read_be32(r + 0x60)) {
                fprintf(f, "%s%u", first ? "" : ",", i);
                first = 0;
            }
        }
    }
    fprintf(f, "],\n");
}

static unsigned changed_bytes(const uint8_t *before, const uint8_t *after, size_t size) {
    unsigned count = 0;
    for (size_t i = 0; i < size; ++i) if (before[i] != after[i]) ++count;
    return count;
}

static void write_focus_effects(FILE *f, const nflstreet_ui_focus_effects *effects) {
    fprintf(f, "[ ");
    for (unsigned i = 0; i < effects->count; ++i) {
        const nflstreet_ui_focus_request *r = &effects->requests[i];
        fprintf(f, "%s{\"root_offset\":%u,\"script_offset\":%u,\"args\":[%u,%u]}",
                i ? "," : "", r->root_offset, r->script_offset, r->args[0], r->args[1]);
    }
    fprintf(f, " ]");
}

static int export_focus_probe(const uint8_t *prepared, size_t size,
                              const nflstreet_team_screen *initial) {
    uint8_t *source = malloc(size);
    uint8_t *before = malloc(size);
    nflstreet_team_screen *state = malloc(sizeof *state);
    char path[4096];
    int n;
    if (!source || !before || !state) { free(source); free(before); free(state); return 0; }
    memcpy(source, prepared, size); *state = *initial;
    /* These are explicit host-owned projections of the original manager route;
       neither ID is inferred from the source event route. */
    state->classes.shared.link_enabled = 1;
    state->classes.shared.linked_screen = 7;
    const unsigned nodes[3] = {21, 22, 28};
    const char *files[3] = {"native-focus21-source.bin", "native-focus22-source.bin", "native-focus28-source.bin"};
    unsigned changed[3] = {0};
    uint32_t effect_delta[3][6] = {{0}};
    nflstreet_ui_focus_effects effects[3] = {{0}};
    for (unsigned i = 0; i < 3; ++i) {
        memcpy(before, source, size);
        const uint32_t resources_before = state->classes.shared.resource_count;
        const uint32_t broadcasts_before = state->classes.shared.broadcast_count;
        const uint32_t pending_before = state->classes.shared.pending_count;
        const uint32_t audio_before = state->classes.shared.audio_count;
        const uint32_t links_before = state->classes.shared.link_count;
        const uint32_t class_effects_before = state->classes.effect_count;
        if (!nflstreet_team_screen_focus_context(source, size, 0x81100000u,
                state, nodes[i], 1, 0, 0, 7, 0x817f2380u, 0, &effects[i])) {
            free(source); free(before); free(state); return 0;
        }
        changed[i] = changed_bytes(before, source, size);
        effect_delta[i][0] = state->classes.shared.resource_count - resources_before;
        effect_delta[i][1] = state->classes.shared.broadcast_count - broadcasts_before;
        effect_delta[i][2] = state->classes.shared.pending_count - pending_before;
        effect_delta[i][3] = state->classes.shared.audio_count - audio_before;
        effect_delta[i][4] = state->classes.shared.link_count - links_before;
        effect_delta[i][5] = state->classes.effect_count - class_effects_before;
        n = snprintf(path, sizeof path, "%s/%s", fixture_directory, files[i]);
        if (n < 0 || (size_t)n >= sizeof path || !write_file(path, source, size)) {
            free(source); free(before); free(state); return 0;
        }
    }
    n = snprintf(path, sizeof path, "%s/native-focus.json", fixture_directory);
    if (n < 0 || (size_t)n >= sizeof path) { free(source); free(before); free(state); return 0; }
    FILE *f = fopen(path, "wb");
    if (!f) { free(source); free(before); free(state); return 0; }
    fprintf(f, "{\n  \"format\": \"daywater-native-team-manager-focus-v1\",\n");
    fprintf(f, "  \"boundary\": \"Native focused calls made on an independently prepared startup fixture. This is not an original-engine execution.\",\n");
    fprintf(f, "  \"starting_snapshot\": \"native-pre-input-source.bin\",\n  \"route\": {\"current_screen\":0,\"linked_screen\":7,\"stack_base\":\"0x817f2380\",\"list\":0,\"argument\":0},\n");
    fprintf(f, "  \"calls\": [\n");
    for (unsigned i = 0; i < 3; ++i) {
        fprintf(f, "    {\"node\":%u,\"selected\":1,\"returned\":true,\"snapshot\":\"%s\",\"changed_bytes_from_previous\":%u,\"effects\":",
                nodes[i], files[i], changed[i]);
        write_focus_effects(f, &effects[i]);
        fprintf(f, ",\"host_effect_delta\":{\"resource_requests\":%u,\"broadcast_requests\":%u,\"pending_focus_requests\":%u,\"audio_requests\":%u,\"linked_screen_requests\":%u,\"class_effect_requests\":%u}}%s\n",
                effect_delta[i][0], effect_delta[i][1], effect_delta[i][2],
                effect_delta[i][3], effect_delta[i][4], effect_delta[i][5],
                i == 2 ? "" : ",");
    }
    fprintf(f, "  ],\n  \"source_size\":%zu,\n  \"pending_focus_requests\":%u\n}\n",
            size, state->classes.shared.pending_count);
    int closed = fclose(f) == 0;
    free(source); free(before); free(state);
    return closed;
}

static int export_dispatcher_probe(const uint8_t *prepared, size_t size,
                                   const nflstreet_team_screen *initial) {
    const unsigned nodes[] = {21, 22};
    const uint32_t current_screen = 7, linked_screen = UINT32_MAX;
    const uint32_t stack_base = 0x817f2380u;
    char path[4096];
    int n;
    n = snprintf(path, sizeof path, "%s/dispatcher-probe.json", fixture_directory);
    if (n < 0 || (size_t)n >= sizeof path) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "{\n  \"format\": \"daywater-native-team-dispatcher-event7-v1\",\n");
    fprintf(f, "  \"boundary\": \"Native source focus and event execution, with explicit manager VM context; this is not an original engine run.\",\n");
    fprintf(f, "  \"source_size\": %zu, \"current_screen\": 7, \"linked_screen\": 4294967295, \"stack_base\": \"0x817f2380\",\n", size);
    fprintf(f, "  \"focus_nodes\": [21,22], \"focus_argument\": 0, \"event\": 7, \"port\": 0, \"float_input\": 1.0,\n");
    fprintf(f, "  \"mode_runs\": [\n");
    for (unsigned run = 0; run < 2; ++run) {
        const unsigned mode = run ? 6 : 0;
        uint8_t *source = malloc(size), *before = malloc(size);
        nflstreet_team_screen *state = malloc(sizeof *state);
        nflstreet_ui_focus_effects focus_effects[2] = {{0}};
        if (!source || !before || !state) { free(source); free(before); free(state); fclose(f); return 0; }
        memcpy(source, prepared, size);
        *state = *initial;
        state->classes.shared.link_enabled = 1;
        state->classes.shared.linked_screen = linked_screen;
        state->supplied_common_mode = mode;
        state->game.query_mode = 1;
        state->supplied_transition = mode == 6;
        state->transition_clock = 0x1a0du;
        state->transition_audio = 1;
        state->transition_context = 0;
        for (unsigned i = 0; i < 2; ++i) {
            if (!nflstreet_team_screen_focus_context(source, size, 0x81100000u,
                    state, nodes[i], 1, 0, current_screen, linked_screen,
                    stack_base, 0, &focus_effects[i])) {
                free(source); free(before); free(state); fclose(f); return 0;
            }
        }
        memcpy(before, source, size);
        unsigned pre_resources = state->classes.shared.resource_count;
        unsigned pre_broadcasts = state->classes.shared.broadcast_count;
        unsigned pre_pending = state->classes.shared.pending_count;
        unsigned pre_audio = state->classes.shared.audio_count;
        unsigned pre_links = state->classes.shared.link_count;
        unsigned pre_class_effects = state->classes.effect_count;
        unsigned pre_transitions = state->transition_count;
        uint32_t pre_rates_count = state->classes.shared.rates.count;
        uint32_t pre_rates_clock = state->classes.shared.rates.clock;
        n = snprintf(path, sizeof path, "%s/dispatcher-mode%u-pre-event7-source.bin", fixture_directory, mode);
        if (n < 0 || (size_t)n >= sizeof path || !write_file(path, source, size)) { free(source); free(before); free(state); fclose(f); return 0; }
        int input_ok = nflstreet_team_screen_input_context(source, size, 0x81100000u,
            state, 0, 7, 1.0f, current_screen, linked_screen, stack_base);
        unsigned source_delta = changed_bytes(before, source, size);
        n = snprintf(path, sizeof path, "%s/dispatcher-mode%u-post-event7-source.bin", fixture_directory, mode);
        if (n < 0 || (size_t)n >= sizeof path || !write_file(path, source, size)) { free(source); free(before); free(state); fclose(f); return 0; }
        if (run) fputs(",\n", f);
        fprintf(f, "    {\"mode\":%u,\"pre_source\":\"dispatcher-mode%u-pre-event7-source.bin\",\"post_source\":\"dispatcher-mode%u-post-event7-source.bin\",\"input_returned\":%s,\"source_delta_bytes\":%u,\"rates_before\":[%u,%u],\"rates_after\":[%u,%u],\"focus_effect_counts\":[%u,%u],\"focus_source_delta_bytes\":%u,\"effect_delta\":{\"resource\":%u,\"broadcast\":%u,\"pending\":%u,\"audio\":%u,\"link\":%u,\"class\":%u,\"transition\":%u},\"transition\":{\"supplied\":%u,\"clock\":%u,\"audio_enabled\":%u,\"broadcast_context\":%u,\"property\":\"0x80000003\",\"arg0\":0},\"callback_state\":[%u,%u]}" ,
                mode, mode, mode, input_ok ? "true" : "false", source_delta,
                pre_rates_count, pre_rates_clock, state->classes.shared.rates.count,
                state->classes.shared.rates.clock, focus_effects[0].count,
                focus_effects[1].count, changed_bytes(prepared, before, size),
                state->classes.shared.resource_count - pre_resources,
                state->classes.shared.broadcast_count - pre_broadcasts,
                state->classes.shared.pending_count - pre_pending,
                state->classes.shared.audio_count - pre_audio,
                state->classes.shared.link_count - pre_links,
                state->classes.effect_count - pre_class_effects,
                state->transition_count - pre_transitions,
                state->supplied_transition, state->transition_clock,
                state->transition_audio, state->transition_context,
                state->player_callback, state->actors.callback_registered);
        free(source); free(before); free(state);
    }
    fprintf(f, "\n  ]\n}\n");
    int ok = !ferror(f) && fclose(f) == 0;
    return ok;
}

static int export_events(FILE *f, const uint8_t *bytes, size_t size,
                         const nflstreet_team_screen *initial) {
    fprintf(f, "  \"native_event_runs\": [\n");
    for (unsigned event = 2; event <= 7; ++event) {
        uint8_t *copy = malloc(size), *before = malloc(size);
        if (!copy || !before) { free(copy); free(before); return 0; }
        memcpy(copy, bytes, size); memcpy(before, bytes, size);
        nflstreet_team_screen state = *initial;
        int ok = nflstreet_team_screen_input(copy, size, 0x81100000u, &state, 0,
                                             (uint16_t)event, 1.0f);
        if (ok) {
            char path[4096];
            int n = snprintf(path, sizeof path, "%s/native-event-%02u-source.bin", fixture_directory, event);
            if (n < 0 || (size_t)n >= sizeof path || !write_file(path, copy, size)) {
                free(copy); free(before); return 0;
            }
        }
        if (event > 2) fprintf(f, ",\n");
        fprintf(f, "    {\"event\":%u,\"native_input_returned\":%s,\"snapshot\":%s,\"changed_source_bytes\":%u,\"team_ids\":[%u,%u],\"selected_counts\":[%u,%u],\"effect_trace_delta\":{\"resource_requests\":%u,\"broadcast_requests\":%u,\"pending_focus_requests\":%u,\"audio_requests\":%u,\"transition_requests\":%u,\"player_spawn_requests\":%u,\"player_resource_requests\":%u}}",
                event, ok ? "true" : "false", ok ? "true" : "false",
                changed_bytes(before, copy, size), state.game.startup.selected_id[0],
                state.game.startup.selected_id[1], state.activation.sides[0].selected_count,
                state.activation.sides[1].selected_count,
                state.classes.shared.resource_count - initial->classes.shared.resource_count,
                state.classes.shared.broadcast_count - initial->classes.shared.broadcast_count,
                state.classes.shared.pending_count - initial->classes.shared.pending_count,
                state.classes.shared.audio_count - initial->classes.shared.audio_count,
                state.transition_count - initial->transition_count,
                state.player_spawn_count - initial->player_spawn_count,
                state.player_resource_count - initial->player_resource_count);
        free(copy); free(before);
    }
    fprintf(f, "\n  ],\n");
    return !ferror(f);
}

static int export_fixture(uint8_t *bytes, size_t size,
                          nflstreet_team_screen *s, const char *unpacked) {
    (void)unpacked;
    char path[4096];
    int n = snprintf(path, sizeof path, "%s/native-pre-input-source.bin", fixture_directory);
    if (n < 0 || (size_t)n >= sizeof path || !write_file(path, bytes, size)) return 0;
    n = snprintf(path, sizeof path, "%s/fixture.json", fixture_directory);
    if (n < 0 || (size_t)n >= sizeof path) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "{\n  \"format\": \"daywater-native-team-startup-fixture-v1\",\n");
    fprintf(f, "  \"stage\": \"native-prepared-controlled-stage-before-input\",\n");
    fprintf(f, "  \"device_fixture_mode\": \"%s\",\n", logical_port0_mode ? "logical-port0-record-side0-mapping0" : "empty-device-projection");
    fprintf(f, "  \"boundary\": \"Relocated source, native construction/activation, and %u passive updates. This is not a full original-engine startup or an original RAM capture.\",\n", fixture_frames);
    fprintf(f, "  \"source_file\": \"native-pre-input-source.bin\",\n  \"source_size\": %zu,\n  \"source_base\": \"0x81100000\",\n", size);
    fprintf(f, "  \"state_before_input\": {\"selected_team_ids\": [%u, %u], \"prior_profile\": [%u, %u], \"lineup_sizes\": [%u, %u], \"selected_ids\": [[",
            s->game.startup.selected_id[0], s->game.startup.selected_id[1],
            s->game.startup.prior_profile[0], s->game.startup.prior_profile[1],
            s->activation.sides[0].selected_count, s->activation.sides[1].selected_count);
    for (unsigned side = 0; side < 2; ++side) {
        if (side) fputs("],[", f);
        for (unsigned i = 0; i < s->activation.sides[side].selected_count; ++i)
            fprintf(f, "%s%u", i ? "," : "", s->activation.sides[side].selected_ids[i]);
    }
    fprintf(f, "]], \"roster_rows\": [[");
    for (unsigned side = 0; side < 2; ++side) {
        if (side) fputs("],[", f);
        const nflstreet_team_activation_side *a = &s->activation.sides[side];
        for (unsigned i = 0; i < a->count; ++i)
            fprintf(f, "%s[%u,%d,%u]", i ? "," : "", a->roster[i].id,
                    a->roster[i].selected, a->roster[i].auxiliary);
    }
    fprintf(f, "]], \"supplied_port_mapping_mask\": %u, \"port_mapping\": [", s->supplied_port_mapping_mask);
    for (unsigned i = 0; i < NFLSTREET_TEAM_DEVICE_MAPPING_COUNT; ++i)
        fprintf(f, "%s%u", i ? "," : "", s->port_mapping[i]);
    fprintf(f, "], \"supplied_controller_sides\": %s, \"controller_sides\": [%u,%u,%u,%u], \"player_spawn_requests\": %u, \"player_resource_requests\": %u, \"transition_requests\": %u, \"ui_effect_requests\": %u, \"line_plan_count\": %u},\n",
            s->supplied_controller_sides ? "true" : "false",
            s->controller_sides[0], s->controller_sides[1], s->controller_sides[2], s->controller_sides[3],
            s->player_spawn_count, s->player_resource_count, s->transition_count,
            s->classes.effect_count, s->objects.line_plan_count);
    fprintf(f, "  \"native_font_copies\": [\n");
    for (unsigned i = 0; i < NFLSTREET_TEAM_CLASSES_FONT_COUNT; ++i) {
        const nflstreet_font *font = &s->objects.fonts[i];
        n = snprintf(path, sizeof path, "%s/font-%02u.bin", fixture_directory, i);
        if (n < 0 || (size_t)n >= sizeof path || !write_file(path, font->bytes, font->size)) { fclose(f); return 0; }
        unsigned at = 0x01740000u + i * 0x10000u;
        uint32_t record = 0x817f3000u + i * 0x20u;
        uint32_t asset = 0x817f3100u + i * 0x20u;
        fprintf(f, "    {\"file\": \"font-%02u.bin\", \"size\": %zu, \"line_height\": %u, \"glyphs_offset\": %zu, \"kerning_offset\": %zu, \"atlas_offset\": %zu, \"record_address\": \"0x%08x\", \"record_words\": [\"0x00000000\",\"0x%08x\",\"0x%08x\",\"0x%08x\",\"0x%08x\"], \"asset_record_address\": \"0x%08x\", \"asset_record_word\": \"0x%08x\"}%s\n",
                i, font->size, font->line_height, font->glyphs, font->kerning, font->atlas,
                record, 0x80000000u + at, 0x80000000u + at + (uint32_t)font->glyphs,
                0x80000000u + at + (uint32_t)font->kerning,
                0x80000000u + at + (uint32_t)font->atlas, asset, record,
                i + 1 == NFLSTREET_TEAM_CLASSES_FONT_COUNT ? "" : ",");
    }
    fprintf(f, "  ],\n  \"line_plans\": [\n");
    for (unsigned i = 0; i < s->objects.line_plan_count; ++i) {
        const nflstreet_team_classes_line_plan *p = &s->objects.line_plans[i];
        fprintf(f, "    {\"ordinal\": %u, \"target_object\": \"0x%08x\", \"native_logical_base\": \"0x%08x\", \"checker_allocator_address\": \"0x%08x\", \"line_count\": %u, \"ranges\": [",
                i, p->target, p->line_base, 0x81610000u + i * 1024u, p->plan.count);
        for (unsigned j = 0; j < p->plan.count; ++j)
            fprintf(f, "%s[%u,%u]", j ? "," : "", p->plan.lines[j].begin, p->plan.lines[j].end);
        fprintf(f, "]}%s\n", i + 1 == s->objects.line_plan_count ? "" : ",");
    }
    fprintf(f, "  ],\n  \"line_address_note\": \"checker_allocator_address is the deterministic return used by the bounded original manager checker (0x81610000 + ordinal*1024); it is a materialization plan, not a claimed live-original heap address. Native source pointers still use native_logical_base.\",\n");
    write_dispatch_nodes(f, bytes, size);
    if (dispatcher_probe_mode) {
        fprintf(f, "  \"dispatcher_probe\": \"See dispatcher-probe.json and its per-mode before/after source dumps. This opt-in path does not modify default fixture outputs.\"\n}\n");
    } else if (focus_probe_mode) {
        fprintf(f, "  \"focus_probe\": \"See native-focus.json for the opt-in manager-route native focus calls; the default and logical-port0 source snapshots are not modified.\"\n}\n");
    } else if (logical_port0_mode) {
        if (!export_events(f, bytes, size, s)) { fclose(f); return 0; }
        fprintf(f, "  \"event_results_note\": \"native_event_runs are actual independent native input executions from copies of this pre-input state. Queue counts are retained per lane and do not assert a cross-lane total order; snapshots exist only when native input returned success.\"\n}\n");
    } else {
        fprintf(f, "  \"event_expectations\": {\"basis\": \"Previously exercised native source events from a fresh equivalent passive-startup state; physical controller meanings are not inferred.\", \"events_2_3\": \"same bounded artwork/text-state mutations; team IDs unchanged\", \"events_4_5\": \"no-op in the tested state\", \"event_6\": \"roster removal/player-window query/portrait selection; adds two preview spawn requests; retains team IDs\", \"event_7\": \"standalone callback13 transition path: clears player callback and class3 gate; supplied common mode6 creates deadline+500 and one transition request; delayed completion emits broadcast and linked-screen request\", \"effect_capture\": \"This default export does not contain per-event effect traces.\"}\n}\n");
    }
    int ok = !ferror(f) && fclose(f) == 0;
    return ok && (!focus_probe_mode || export_focus_probe(bytes, size, s)) &&
           (!dispatcher_probe_mode || export_dispatcher_probe(bytes, size, s));
}

int main(int argc, char **argv) {
    const char *source, *unpacked, *dol, *frames;
    if (argc == 7 && strcmp(argv[1], "--out") == 0) {
        logical_port0_mode = 0;
        if (strlen(argv[2]) >= sizeof fixture_directory) return 2;
        strcpy(fixture_directory, argv[2]);
        source = argv[3]; unpacked = argv[4]; dol = argv[5]; frames = argv[6];
    } else if (argc == 9 && strcmp(argv[1], "--out") == 0 &&
               strcmp(argv[3], "--mode") == 0 && strcmp(argv[4], "logical-port0") == 0) {
        logical_port0_mode = 1;
        if (strlen(argv[2]) >= sizeof fixture_directory) return 2;
        strcpy(fixture_directory, argv[2]);
        source = argv[5]; unpacked = argv[6]; dol = argv[7]; frames = argv[8];
    } else if (argc == 8 && strcmp(argv[1], "--out") == 0 &&
               strcmp(argv[3], "--focus-probe") == 0) {
        logical_port0_mode = 1;
        focus_probe_mode = 1;
        if (strlen(argv[2]) >= sizeof fixture_directory) return 2;
        strcpy(fixture_directory, argv[2]);
        source = argv[4]; unpacked = argv[5]; dol = argv[6]; frames = argv[7];
    } else if (argc == 8 && strcmp(argv[1], "--out") == 0 &&
               strcmp(argv[3], "--dispatcher-probe") == 0) {
        logical_port0_mode = 1;
        dispatcher_probe_mode = 1;
        if (strlen(argv[2]) >= sizeof fixture_directory) return 2;
        strcpy(fixture_directory, argv[2]);
        source = argv[4]; unpacked = argv[5]; dol = argv[6]; frames = argv[7];
    } else return 2;
    fixture_frames = (unsigned)strtoul(frames, NULL, 10);
    if (!fixture_frames || fixture_frames > 10000) return 2;
    char *probe_argv[] = {argv[0], (char *)source, (char *)unpacked, (char *)dol, (char *)frames};
    return team_screen_probe_main(5, probe_argv);
}
