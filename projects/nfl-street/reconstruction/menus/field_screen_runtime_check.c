#include "field_screen_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_BASE 0x81100000u
#define STACK_BASE 0x817c0000u
#define MAX_SOURCE (8u * 1024u * 1024u)

typedef struct {
    uint8_t *source;
    size_t source_size;
    nflstreet_field_screen_state *state;
    nflstreet_field_screen_runtime runtime;
} fixture;

static int read_file(const char *path, uint8_t **bytes, size_t *size, size_t max_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long length = ftell(f);
    if (length <= 0 || (size_t)length > max_size || fseek(f, 0, SEEK_SET)) {
        fclose(f); return 0;
    }
    uint8_t *data = malloc((size_t)length);
    if (!data) { fclose(f); return 0; }
    int ok = fread(data, 1, (size_t)length, f) == (size_t)length;
    fclose(f);
    if (!ok) { free(data); return 0; }
    *bytes = data; *size = (size_t)length;
    return 1;
}

static int write_file(const char *path, const void *bytes, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(bytes, 1, size, f) == size;
    return fclose(f) == 0 && ok;
}

static void close_fixture(fixture *f) {
    if (!f) return;
    if (f->state) {
        for (unsigned i = 0; i < 4; ++i) free((void *)f->state->fonts[i].bytes);
        free(f->state);
    }
    free(f->source);
    memset(f, 0, sizeof *f);
}

static int open_fixture(fixture *f, const char *source_path,
        const uint8_t *template_bytes, size_t template_size,
        const uint8_t *dol_bytes, size_t dol_size, const char *font_dir) {
    memset(f, 0, sizeof *f);
    if (!read_file(source_path, &f->source, &f->source_size, MAX_SOURCE)) return 0;
    f->state = calloc(1, sizeof *f->state);
    if (!f->state) return 0;
    for (unsigned i = 0; i < 10; ++i) f->state->port_mapping[i] = UINT32_MAX;
    f->state->port_mapping[0] = 0;
    f->state->supplied_port_mapping_mask = 0x3ffu;
    if (!nflstreet_field_screen_runtime_prepare(f->source, f->source_size,
            template_bytes, template_size, dol_bytes, dol_size, font_dir,
            f->state, 0, 0, 1, 0, 0x802u, 16)) return 0;
    const uint32_t load_args[2] = {1, 0x69};
    if (!nflstreet_field_screen_runtime_start(f->source, f->source_size,
            f->state, 8, 5, STACK_BASE, UINT32_MAX, 2, load_args)) return 0;
    f->runtime = (nflstreet_field_screen_runtime){
        f->source, f->source_size, f->state, 8, 5, STACK_BASE
    };
    for (unsigned i = 0; i < 180; ++i)
        if (!nflstreet_field_screen_runtime_tick(&f->runtime, 16)) return 0;
    return f->state->startup_fffe_completed && f->state->startup_fffc_completed;
}

static int advance_event(fixture *f, unsigned event) {
    if (!nflstreet_field_screen_runtime_event(&f->runtime, event)) return 0;
    for (unsigned i = 0; i < 60; ++i)
        if (!nflstreet_field_screen_runtime_tick(&f->runtime, 16)) return 0;
    return 1;
}

static int finish_rejects_atomically(fixture *f) {
    if (!f || !f->source || !f->state) return 0;
    uint8_t *source_before = malloc(f->source_size);
    nflstreet_field_screen_state *state_before = malloc(sizeof *state_before);
    if (!source_before || !state_before) {
        free(source_before); free(state_before); return 0;
    }
    memcpy(source_before, f->source, f->source_size);
    memcpy(state_before, f->state, sizeof *state_before);
    int rejected = !nflstreet_field_screen_runtime_finish(&f->runtime);
    int unchanged = memcmp(source_before, f->source, f->source_size) == 0 &&
                    memcmp(state_before, f->state, sizeof *state_before) == 0;
    free(source_before); free(state_before);
    return rejected && unchanged;
}

/* This harness is a synthetic queue consumer. It verifies the host acknowledgment
   contract; it does not load archives, play audio, or execute route transitions. */
static int acknowledge_captured_queues(nflstreet_field_screen_state *s,
                                       unsigned *resource_total) {
    unsigned n = s->resource_effect_count;
    nflstreet_field_resource_effect snapshot[128];
    if (n > 128) return 0;
    memcpy(snapshot, s->resource_effects, (size_t)n * sizeof snapshot[0]);
    if (n) {
        nflstreet_field_resource_effect bad[128];
        memcpy(bad, snapshot, (size_t)n * sizeof bad[0]);
        bad[0].entry ^= 1u;
        nflstreet_field_resource_effect before[128];
        memcpy(before, s->resource_effects, (size_t)n * sizeof before[0]);
    if (nflstreet_field_screen_runtime_ack_resource_effects(s, bad, 1) ||
            s->resource_effect_count != n ||
            memcmp(before, s->resource_effects, (size_t)n * sizeof before[0])) return 0;
        if (n >= 128) return 0;
        nflstreet_field_resource_effect appended = {1, 31, 0x1234};
        s->resource_effects[n] = appended;
        s->resource_effect_count = n + 1;
        if (!nflstreet_field_screen_runtime_ack_resource_effects(s, snapshot, n) ||
            s->resource_effect_count != 1 ||
            s->resource_effects[0].acquire != appended.acquire ||
            s->resource_effects[0].archive != appended.archive ||
            s->resource_effects[0].entry != appended.entry) return 0;
        if (!nflstreet_field_screen_runtime_ack_resource_effects(s, &appended, 1) ||
            s->resource_effect_count != 0) return 0;
        *resource_total += n + 1;
    }
    n = s->main_menu.resource_count;
    nflstreet_ui_button_effect buttons[128];
    if (n > 128) return 0;
    memcpy(buttons, s->main_menu.resources, (size_t)n * sizeof buttons[0]);
    if (!nflstreet_field_screen_runtime_ack_button_resources(s, buttons, n) ||
        s->main_menu.resource_count != 0) return 0;
    n = s->main_menu.audio_count;
    uint32_t audio[128];
    if (n > 128) return 0;
    memcpy(audio, s->main_menu.audio, (size_t)n * sizeof audio[0]);
    if (!nflstreet_field_screen_runtime_ack_audio(s, audio, n) ||
        s->main_menu.audio_count != 0) return 0;
    n = s->main_menu.broadcast_count;
    nflstreet_ui_service_request broadcasts[128];
    if (n > 128) return 0;
    memcpy(broadcasts, s->main_menu.broadcasts, (size_t)n * sizeof broadcasts[0]);
    if (!nflstreet_field_screen_runtime_ack_broadcasts(s, broadcasts, n) ||
        s->main_menu.broadcast_count != 0) return 0;
    n = s->main_menu.link_count;
    uint32_t links[16];
    if (n > 16) return 0;
    memcpy(links, s->main_menu.links, (size_t)n * sizeof links[0]);
    if (!nflstreet_field_screen_runtime_ack_links(s, links, n) ||
        s->main_menu.link_count != 0) return 0;
    n = s->focus_effect_count;
    nflstreet_ui_focus_effects focus[128];
    if (n > 128) return 0;
    memcpy(focus, s->focus_effects, (size_t)n * sizeof focus[0]);
    if (!nflstreet_field_screen_runtime_ack_focus_effects(s, focus, n) ||
        s->focus_effect_count != 0) return 0;
    return s->main_menu.pending_count == 0;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "usage: field-screen-runtime-check.exe source8.bin template00002.bin main.dol font-dir output-dir\n");
        return 2;
    }
    if (strncmp(argv[5], "W:\\Daywater\\", 12) != 0) return 2;
    uint8_t *template_bytes = NULL, *dol_bytes = NULL;
    size_t template_size = 0, dol_size = 0;
    if (!read_file(argv[2], &template_bytes, &template_size, 32u * 1024u * 1024u) ||
        !read_file(argv[3], &dol_bytes, &dol_size, 16u * 1024u * 1024u)) {
        free(template_bytes); free(dol_bytes); return 3;
    }
    fixture nav = {0}, confirm = {0}, owner_other = {0}, browse = {0};
    int ok = open_fixture(&nav, argv[1], template_bytes, template_size,
                          dol_bytes, dol_size, argv[4]);
    if (!ok) { fprintf(stderr, "navigation fixture startup failed\n"); goto done; }
    unsigned resource_total = 0;
    if (!acknowledge_captured_queues(nav.state, &resource_total)) {
        fprintf(stderr, "initial queue acknowledgment failed\n"); ok = 0; goto done;
    }
    for (unsigned slot = 1; slot < 8; ++slot) {
        if (!advance_event(&nav, 5) || nav.state->supplied_selected_index != slot ||
            !acknowledge_captured_queues(nav.state, &resource_total)) {
            fprintf(stderr, "forward selection failed at slot%u\n", slot); ok = 0; goto done;
        }
    }
    if (!advance_event(&nav, 5) || nav.state->supplied_selected_index != 7 ||
        !acknowledge_captured_queues(nav.state, &resource_total)) {
        fprintf(stderr, "forward boundary at slot7 failed\n"); ok = 0; goto done;
    }
    if (!advance_event(&nav, 4) || nav.state->supplied_selected_index != 6 ||
        !acknowledge_captured_queues(nav.state, &resource_total)) {
        fprintf(stderr, "reverse movement from slot7 failed\n"); ok = 0; goto done;
    }
    unsigned nav_ticks = nav.state->runtime_tick_count;
    unsigned navigation_resource_total = resource_total;
    char output_path[4096];
    int written = snprintf(output_path, sizeof output_path,
        "%s\\field-screen-navigation-final.bin", argv[5]);
    if (written < 0 || (size_t)written >= sizeof output_path ||
        !write_file(output_path, nav.source, nav.source_size)) {
        fprintf(stderr, "navigation source export failed\n"); ok = 0; goto done;
    }
    close_fixture(&nav);

    ok = open_fixture(&confirm, argv[1], template_bytes, template_size,
                      dol_bytes, dol_size, argv[4]);
    if (!ok) { fprintf(stderr, "confirmation fixture startup failed\n"); goto done; }
    resource_total = 0;
    if (!acknowledge_captured_queues(confirm.state, &resource_total) ||
        !advance_event(&confirm, 4) || !acknowledge_captured_queues(confirm.state, &resource_total) ||
        !advance_event(&confirm, 4) || !acknowledge_captured_queues(confirm.state, &resource_total)) {
        fprintf(stderr, "confirmation setup events failed\n"); ok = 0; goto done;
    }
    if (!finish_rejects_atomically(&confirm)) {
        fprintf(stderr, "pre-transition finish rejection was not atomic\n"); ok = 0; goto done;
    }
    confirm.state->supplied_finish_owner_value = 1;
    if (!advance_event(&confirm, 7)) { fprintf(stderr, "event7 failed\n"); ok = 0; goto done; }
    if (!confirm.state->transition_request_count || !confirm.state->transition_completion_count ||
        !confirm.state->main_menu.link_count || !confirm.state->linked_route_observed ||
        confirm.state->linked_route_target != 5) {
        fprintf(stderr, "event7 evidence failed requests=%u completions=%u links=%u observed=%u target=%u\n",
            confirm.state->transition_request_count, confirm.state->transition_completion_count,
            confirm.state->main_menu.link_count, confirm.state->linked_route_observed,
            confirm.state->linked_route_target); ok = 0; goto done;
    }
    if (!acknowledge_captured_queues(confirm.state, &resource_total) ||
        confirm.state->main_menu.link_count != 0) {
        fprintf(stderr, "link queue acknowledgment failed count=%u\n",
            confirm.state->main_menu.link_count); ok = 0; goto done;
    }
    if (!nflstreet_field_screen_runtime_finish(&confirm.runtime)) {
        fprintf(stderr, "FFFD callback failed release=%u commit=%u pending=%u\n",
            confirm.state->field_cache_release_requests, confirm.state->field_commit_request_count,
            confirm.state->main_menu.pending_count); ok = 0; goto done;
    }
    if (confirm.state->field_commit_request_count != 1 ||
        confirm.state->field_commit_requests[0].ui_slot != 0 ||
        confirm.state->field_commit_requests[0].raw_key != confirm.state->field_bootstrap.keys[0] ||
        confirm.state->field_commit_requests[0].supplied_owner_value != 1 ||
        confirm.state->field_cache_release_requests != 1 ||
        !acknowledge_captured_queues(confirm.state, &resource_total)) {
        fprintf(stderr, "FFFD selected-field assertion failed slot=%u key=%u expected=%u owner=%u release=%u\n",
            confirm.state->field_commit_request_count ? confirm.state->field_commit_requests[0].ui_slot : UINT32_MAX,
            confirm.state->field_commit_request_count ? confirm.state->field_commit_requests[0].raw_key : UINT32_MAX,
            confirm.state->field_bootstrap.keys[0],
            confirm.state->field_commit_request_count ? confirm.state->field_commit_requests[0].supplied_owner_value : UINT32_MAX,
        confirm.state->field_cache_release_requests); ok = 0; goto done;
    }
    if (!finish_rejects_atomically(&confirm)) {
        fprintf(stderr, "duplicate finish rejection was not atomic\n"); ok = 0; goto done;
    }
    uint32_t nav_commit_key = confirm.state->field_commit_requests[0].raw_key;
    unsigned confirmation_resource_total = resource_total;
    written = snprintf(output_path, sizeof output_path,
        "%s\\field-screen-finished.bin", argv[5]);
    if (written < 0 || (size_t)written >= sizeof output_path ||
        !write_file(output_path, confirm.source, confirm.source_size)) {
        fprintf(stderr, "finished source export failed\n"); ok = 0; goto done;
    }
    close_fixture(&confirm);

    ok = open_fixture(&owner_other, argv[1], template_bytes, template_size,
                      dol_bytes, dol_size, argv[4]);
    if (!ok) { fprintf(stderr, "non-one owner fixture startup failed\n"); goto done; }
    resource_total = 0;
    if (!acknowledge_captured_queues(owner_other.state, &resource_total) ||
        !advance_event(&owner_other, 4) || !acknowledge_captured_queues(owner_other.state, &resource_total) ||
        !advance_event(&owner_other, 4) || !acknowledge_captured_queues(owner_other.state, &resource_total) ||
        !advance_event(&owner_other, 7) ||
        !owner_other.state->transition_completion_count ||
        !owner_other.state->linked_route_observed) {
        fprintf(stderr, "non-one owner confirmation setup failed\n"); ok = 0; goto done;
    }
    if (!acknowledge_captured_queues(owner_other.state, &resource_total) ||
        !nflstreet_field_screen_runtime_finish(&owner_other.runtime) ||
        !owner_other.state->runtime_finish_completed ||
        owner_other.state->field_cache_release_requests != 1 ||
        owner_other.state->field_commit_request_count != 0 ||
        !acknowledge_captured_queues(owner_other.state, &resource_total)) {
        fprintf(stderr, "non-one owner release/no-commit branch failed\n"); ok = 0; goto done;
    }
    unsigned owner_other_resource_total = resource_total;
    close_fixture(&owner_other);

    ok = open_fixture(&browse, argv[1], template_bytes, template_size,
                      dol_bytes, dol_size, argv[4]);
    if (!ok) { fprintf(stderr, "browse fixture startup failed\n"); goto done; }
    resource_total = 0;
    if (!acknowledge_captured_queues(browse.state, &resource_total)) {
        fprintf(stderr, "browse initial queue acknowledgment failed\n"); ok = 0; goto done;
    }
    for (unsigned i = 0; i < 128; ++i) {
        unsigned event = (i & 1u) ? 4u : 5u;
        unsigned expected = (i & 1u) ? 0u : 1u;
        if (!advance_event(&browse, event) ||
            browse.state->supplied_selected_index != expected ||
            !acknowledge_captured_queues(browse.state, &resource_total) ||
            browse.state->resource_effect_count > 128 ||
            browse.state->main_menu.resource_count > 128 ||
            browse.state->main_menu.audio_count > 128 ||
            browse.state->main_menu.broadcast_count > 128 ||
            browse.state->main_menu.link_count > 16) {
            fprintf(stderr, "long browse failed at step%u\n", i); ok = 0; goto done;
        }
    }
    written = snprintf(output_path, sizeof output_path,
        "%s\\field-screen-runtime-report.txt", argv[5]);
    if (written < 0 || (size_t)written >= sizeof output_path) { ok = 0; goto done; }
    FILE *report = fopen(output_path, "wb");
    if (!report) { ok = 0; goto done; }
    fprintf(report,
        "status=passed\n"
        "boundary=controlled entry8 source runtime; supplied no-profile/database/port/rate/archive inputs; synthetic deferred-effect consumer\n"
        "startup_fffe_fffc=passed; load_args=1,0x69; current=8; linked=5; stack=0x817c0000\n"
        "fade_ticks=180; elapsed_per_tick=16\n"
        "forward_cache_indices=1,2,3,4,5,6,7\n"
        "forward_boundary_at_7=passed; reverse_to_6=passed\n"
        "event7_transition=completed; linked_route_captured=5; link_notification_acknowledged_before_finish=1; FFFD_slot=0; FFFD_raw_key=%u; match_creation=not_executed\n"
        "runtime_finish_pretransition_rejection_atomic=passed; duplicate_finish_rejection_atomic=passed; owner_not_one_release_without_commit=passed\n"
        "long_browse_steps=128; final_cache_index=%u; browse_resource_effects_acknowledged=%u; navigation_resource_effects_acknowledged=%u; confirmation_resource_effects_acknowledged=%u; owner_not_one_resource_effects_acknowledged=%u\n"
        "queue_ack_mismatch_atomicity=passed; appended_prefix_preservation=passed\n"
        "navigation_ticks=%u\n"
        "limitations=The harness acknowledges captured records as a synthetic host. It does not execute archive I/O, audio, route dispatch, rendering, or field-to-match commit.\n",
        nav_commit_key,
        browse.state->supplied_selected_index, resource_total, navigation_resource_total,
        confirmation_resource_total, owner_other_resource_total, nav_ticks);
    int report_ok = fclose(report) == 0;
    if (!report_ok) { ok = 0; goto done; }
    printf("field navigation: slots1-7, forward boundary and reverse step passed; long browse 128 steps passed; output %s\n",
           output_path);

done:
    close_fixture(&nav); close_fixture(&confirm); close_fixture(&owner_other);
    close_fixture(&browse);
    free(template_bytes); free(dol_bytes);
    return ok ? 0 : 10;
}
