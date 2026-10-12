/* Bounded private scanner for source node focus paths followed by numeric
   source events. It is an exploration harness, not a controller map. */
#include "team_screen.h"
#include "screen.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCAN_BASE 0x81100000u
#define SCAN_STACK 0x817f2380u
#define SCAN_NODES 1024u
#define SCAN_PATH 1024u

static char scan_directory[4096];
static unsigned scan_frames;
static int scanner_capture(uint8_t *, size_t, nflstreet_team_screen *, const char *);
static int scanner_host(nflstreet_team_screen *);
#define DAYWATER_TEAM_STARTUP_CAPTURE scanner_capture
#define DAYWATER_TEAM_STARTUP_HOST scanner_host
#define main team_screen_probe_main
#include "team_screen_probe.c"
#undef main

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}

static uint16_t be16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] << 8 | p[1]); }

static int range(size_t n, uint32_t base, uint32_t address, size_t length, size_t *offset) {
    if (address < base || (size_t)(address - base) > n || length > n - (size_t)(address - base)) return 0;
    *offset = (size_t)(address - base);
    return 1;
}

static int write_bytes(const char *path, const void *bytes, size_t length) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(bytes, 1, length, f) == length;
    if (fclose(f)) ok = 0;
    return ok;
}

static unsigned changed_source(const uint8_t *a, const uint8_t *b, size_t n) {
    unsigned count = 0;
    for (size_t i = 0; i < n; ++i) if (a[i] != b[i]) ++count;
    return count;
}

static int scanner_host(nflstreet_team_screen *s) {
    nflstreet_team_device_record record = {0, 0};
    uint32_t mapping[NFLSTREET_TEAM_DEVICE_MAPPING_COUNT];
    if (!s) return 0;
    for (unsigned i = 0; i < NFLSTREET_TEAM_DEVICE_MAPPING_COUNT; ++i) mapping[i] = UINT32_MAX;
    mapping[0] = 0;
    if (!nflstreet_team_screen_devices(s, &record, 1, mapping)) return 0;
    /* Explicit test inputs for the supplied 500-tick transition effect path. */
    s->supplied_transition = 1;
    s->transition_clock = 6669;
    s->transition_audio = 1;
    s->transition_context = 0;
    return 1;
}

typedef struct {
    uint32_t count, address;
    uint32_t parent[SCAN_NODES];
    uint8_t ambiguous[SCAN_NODES];
} graph;

static int graph_build(const uint8_t *b, size_t n, graph *g) {
    size_t rows_at;
    if (!b || !g || n < 32) return 0;
    memset(g, 0, sizeof *g);
    g->count = be32(b);
    g->address = be32(b + 4);
    if (!g->count || g->count > SCAN_NODES || !range(n, SCAN_BASE, g->address, (size_t)g->count * 20, &rows_at)) return 0;
    for (uint32_t i = 0; i < g->count; ++i) g->parent[i] = UINT32_MAX;
    const uint8_t *rows = b + rows_at;
    for (uint32_t p = 0; p < g->count; ++p) {
        const uint8_t *r = rows + p * 20;
        size_t list_at;
        uint32_t member_count = be32(r + 4), list_address = be32(r + 8);
        if (member_count > 4096 || !range(n, SCAN_BASE, list_address, (size_t)member_count * 4, &list_at)) return 0;
        for (uint32_t j = 0; j < member_count; ++j) {
            uint32_t object_address = be32(b + list_at + j * 4);
            size_t object_at, links_at;
            if (!range(n, SCAN_BASE, object_address, 12, &object_at)) return 0;
            const uint8_t *obj = b + object_at;
            uint32_t links_count = be32(obj + 4), links_address = be32(obj + 8);
            if (links_count > 8192 || !range(n, SCAN_BASE, links_address, (size_t)links_count * 8, &links_at)) return 0;
            for (uint32_t k = 0; k < links_count; ++k) {
                const uint8_t *link = b + links_at + k * 8;
                if (be16(link) != 0xffff) continue;
                uint32_t child = be32(link + 4);
                if (child >= g->count) return 0;
                if (g->parent[child] == UINT32_MAX) g->parent[child] = p;
                else if (g->parent[child] != p) g->ambiguous[child] = 1;
            }
        }
    }
    return 1;
}

static int focus_path(const graph *g, unsigned node, uint32_t out[SCAN_PATH], unsigned *count) {
    uint32_t reverse[SCAN_PATH];
    unsigned used = 0;
    uint32_t current = node;
    uint8_t seen[SCAN_NODES] = {0};
    if (!g || !out || !count || node >= g->count) return 0;
    while (current != 0) {
        if (current >= g->count || g->ambiguous[current] || seen[current] || used >= SCAN_PATH) return 0;
        seen[current] = 1;
        reverse[used++] = current;
        uint32_t parent = g->parent[current];
        if (parent == UINT32_MAX) return 0;
        current = parent;
    }
    for (unsigned i = 0; i < used; ++i) out[i] = reverse[used - 1 - i];
    *count = used;
    return 1;
}

static void team_ids(FILE *f, const nflstreet_team_screen *s) {
    fprintf(f, "[%u,%u]", s->game.startup.selected_id[0], s->game.startup.selected_id[1]);
}

static void windows(FILE *f, const nflstreet_team_screen *s) {
    fprintf(f, "[[%u,%u,%u],[%u,%u,%u]]",
        s->game.startup.carousel.windows[0][0], s->game.startup.carousel.windows[0][1], s->game.startup.carousel.windows[0][2],
        s->game.startup.carousel.windows[1][0], s->game.startup.carousel.windows[1][1], s->game.startup.carousel.windows[1][2]);
}

static int run_case(FILE *out, const uint8_t *prepared, size_t n,
                    const nflstreet_team_screen *initial, const graph *g,
                    unsigned node, unsigned event, unsigned ordinal) {
    uint8_t *source = malloc(n), *before = malloc(n);
    nflstreet_team_screen *state = malloc(sizeof *state);
    uint8_t *dol_copy = NULL;
    nflstreet_team_bootstrap_static constants_copy = {0};
    nflstreet_team_bootstrap bootstrap_copy = {0};
    if (!source || !before || !state || !initial->bootstrap || !initial->bootstrap->constants) goto bad;
    memcpy(source, prepared, n); memcpy(before, prepared, n); *state = *initial;
    const nflstreet_team_bootstrap *original_bootstrap = initial->bootstrap;
    constants_copy = *original_bootstrap->constants;
    if (!constants_copy.mapped_dol || !constants_copy.mapped_size) goto bad;
    dol_copy = malloc(constants_copy.mapped_size);
    if (!dol_copy) goto bad;
    memcpy(dol_copy, constants_copy.mapped_dol, constants_copy.mapped_size);
    constants_copy.mapped_dol = dol_copy;
    bootstrap_copy = *original_bootstrap;
    bootstrap_copy.constants = &constants_copy;
    state->bootstrap = &bootstrap_copy;
    state->classes.shared.link_enabled = 1;
    state->classes.shared.linked_screen = UINT32_MAX;
    const uint32_t teams_initial[2] = {state->game.startup.selected_id[0], state->game.startup.selected_id[1]};
    const uint32_t windows_initial[2][3] = {
        {state->game.startup.carousel.windows[0][0],state->game.startup.carousel.windows[0][1],state->game.startup.carousel.windows[0][2]},
        {state->game.startup.carousel.windows[1][0],state->game.startup.carousel.windows[1][1],state->game.startup.carousel.windows[1][2]}};
    uint32_t pending_initial = state->classes.shared.pending_count;
    uint32_t resources_initial = state->classes.shared.resource_count;
    uint32_t transitions_initial = state->transition_count;

    uint32_t path[SCAN_PATH]; unsigned path_count = 0, applied = 0, focus_scripts = 0;
    int path_valid = focus_path(g, node, path, &path_count);
    int focus_ok = path_valid;
    for (unsigned i = 0; focus_ok && i < path_count; ++i) {
        nflstreet_ui_focus_effects effects = {0};
        if (!nflstreet_team_screen_focus_context(source, n, SCAN_BASE, state,
                path[i], 1, 0, 7, UINT32_MAX, SCAN_STACK, 0, &effects)) focus_ok = 0;
        else { ++applied; focus_scripts += effects.count; }
    }
    unsigned focus_changed = changed_source(prepared, source, n);
    uint32_t teams_after_focus[2] = {state->game.startup.selected_id[0], state->game.startup.selected_id[1]};
    uint32_t windows_after_focus[2][3] = {
        {state->game.startup.carousel.windows[0][0],state->game.startup.carousel.windows[0][1],state->game.startup.carousel.windows[0][2]},
        {state->game.startup.carousel.windows[1][0],state->game.startup.carousel.windows[1][1],state->game.startup.carousel.windows[1][2]}};
    uint32_t pending_after_focus = state->classes.shared.pending_count;
    uint32_t resources_after_focus = state->classes.shared.resource_count;
    uint32_t transitions_after_focus = state->transition_count;
    memcpy(before, source, n);
    const uint32_t teams_before_event[2] = {state->game.startup.selected_id[0], state->game.startup.selected_id[1]};
    const uint32_t windows_before_event[2][3] = {
        {state->game.startup.carousel.windows[0][0],state->game.startup.carousel.windows[0][1],state->game.startup.carousel.windows[0][2]},
        {state->game.startup.carousel.windows[1][0],state->game.startup.carousel.windows[1][1],state->game.startup.carousel.windows[1][2]}};
    uint32_t pending_before_event = state->classes.shared.pending_count;
    uint32_t resources_before_event = state->classes.shared.resource_count;
    uint32_t transitions_before_event = state->transition_count;
    int input_ok = focus_ok && nflstreet_team_screen_input_context(source, n, SCAN_BASE,
        state, 0, (uint16_t)event, 1.0f, 7, UINT32_MAX, SCAN_STACK);
    unsigned changed = changed_source(before, source, n);
    uint32_t teams_after_event[2] = {state->game.startup.selected_id[0], state->game.startup.selected_id[1]};
    uint32_t windows_after_event[2][3] = {
        {state->game.startup.carousel.windows[0][0],state->game.startup.carousel.windows[0][1],state->game.startup.carousel.windows[0][2]},
        {state->game.startup.carousel.windows[1][0],state->game.startup.carousel.windows[1][1],state->game.startup.carousel.windows[1][2]}};
    uint32_t pending_after_event = state->classes.shared.pending_count;
    uint32_t resources_after_event = state->classes.shared.resource_count;
    uint32_t transitions_after_event = state->transition_count;
    char snapshot[256] = "", update_snapshot[256] = "";
    if (input_ok) {
        snprintf(snapshot, sizeof snapshot, "case-%03u-node-%03u-event-%u.bin", ordinal, node, event);
        char pathbuf[4096];
        int z = snprintf(pathbuf, sizeof pathbuf, "%s/%s", scan_directory, snapshot);
        if (z < 0 || (size_t)z >= sizeof pathbuf || !write_bytes(pathbuf, source, n)) input_ok = 0;
    }
    /* The event snapshot above preserves the explicit manager-equivalent VM context.
       Only the separate legacy native advance checkpoint needs this host override. */
    if (input_ok) state->classes.shared.linked_screen = 5;
    unsigned updates_done = 0;
    int updates_ok = input_ok;
    memcpy(before, source, n);
    while (updates_ok && updates_done < 60) {
        if (!nflstreet_team_screen_advance(source, n, SCAN_BASE, state, 16)) { updates_ok = 0; break; }
        ++updates_done;
    }
    unsigned update_changed = changed_source(before, source, n);
    if (updates_done) {
        snprintf(update_snapshot, sizeof update_snapshot, "case-%03u-node-%03u-event-%u-after-update-%u.bin", ordinal, node, event, updates_done);
        char pathbuf[4096];
        int z = snprintf(pathbuf, sizeof pathbuf, "%s/%s", scan_directory, update_snapshot);
        if (z < 0 || (size_t)z >= sizeof pathbuf || !write_bytes(pathbuf, source, n)) updates_ok = 0;
    }
    fprintf(out, "    {\"node\":%u,\"event\":%u,\"result\":\"%s\",\"path_valid\":%s,\"focus_ok\":%s,\"input_ok\":%s,\"focus_path\":[",
        node, event, input_ok ? "accepted" : "rejected", path_valid ? "true" : "false", focus_ok ? "true" : "false", input_ok ? "true" : "false");
    for (unsigned i = 0; i < path_count; ++i) fprintf(out, "%s%u", i ? "," : "", path[i]);
    fprintf(out, "],\"focus_path_applied\":%u,\"focus_script_requests\":%u,\"focus_changed_source_bytes\":%u,\"selected_team_ids_before_focus\":[%u,%u],\"selected_team_ids_after_focus\":[%u,%u],\"carousel_windows_before_focus\":[[%u,%u,%u],[%u,%u,%u]],\"carousel_windows_after_focus\":[[%u,%u,%u],[%u,%u,%u]],\"focus_effect_deltas\":{\"pending_focus_requests\":%u,\"resource_requests\":%u,\"transition_requests\":%u},\"selected_team_ids_before_event\":[%u,%u]",
        applied, focus_scripts, focus_changed,
        teams_initial[0], teams_initial[1], teams_after_focus[0], teams_after_focus[1],
        windows_initial[0][0], windows_initial[0][1], windows_initial[0][2],
        windows_initial[1][0], windows_initial[1][1], windows_initial[1][2],
        windows_after_focus[0][0], windows_after_focus[0][1], windows_after_focus[0][2],
        windows_after_focus[1][0], windows_after_focus[1][1], windows_after_focus[1][2],
        pending_after_focus - pending_initial, resources_after_focus - resources_initial,
        transitions_after_focus - transitions_initial, teams_before_event[0], teams_before_event[1]);
    fprintf(out, ",\"selected_team_ids_after_event\":[%u,%u],\"carousel_windows_before_event\":[[%u,%u,%u],[%u,%u,%u]],\"carousel_windows_after_event\":[[%u,%u,%u],[%u,%u,%u]],\"event_changed_source_bytes\":%u,\"event_effect_deltas\":{\"pending_focus_requests\":%u,\"resource_requests\":%u,\"transition_requests\":%u},\"updates_60_ok\":%s,\"updates_completed\":%u,\"source_changed_bytes_during_updates\":%u,\"selected_team_ids_after_updates\":",
        teams_after_event[0], teams_after_event[1],
        windows_before_event[0][0], windows_before_event[0][1], windows_before_event[0][2],
        windows_before_event[1][0], windows_before_event[1][1], windows_before_event[1][2],
        windows_after_event[0][0], windows_after_event[0][1], windows_after_event[0][2],
        windows_after_event[1][0], windows_after_event[1][1], windows_after_event[1][2],
        changed, pending_after_event - pending_before_event,
        resources_after_event - resources_before_event, transitions_after_event - transitions_before_event,
        updates_ok ? "true" : "false", updates_done, update_changed);
    team_ids(out, state);
    fprintf(out, ",\"carousel_windows_after_updates\":");
    windows(out, state);
    fprintf(out, ",\"updates_host_linked_screen_override\":%s,\"updates_result\":\"%s\",\"updates_effect_deltas\":{\"pending_focus_requests\":%u,\"resource_requests\":%u,\"transition_requests\":%u},\"event_snapshot\":\"%s\",\"updates_snapshot\":\"%s\"}\n",
        input_ok ? "5" : "null",
        updates_ok ? "completed_60" : "rejected_fail_closed",
        state->classes.shared.pending_count - pending_after_event,
        state->classes.shared.resource_count - resources_after_event,
        state->transition_count - transitions_after_event,
        snapshot, update_snapshot);
    free(dol_copy); free(state); free(before); free(source);
    return 1;
bad:
    free(dol_copy); free(state); free(before); free(source);
    return 0;
}

static int scanner_capture(uint8_t *bytes, size_t size, nflstreet_team_screen *initial, const char *unpacked) {
    (void)unpacked;
    graph g;
    if (!graph_build(bytes, size, &g)) return 0;
    char path[4096];
    int z = snprintf(path, sizeof path, "%s/navigation-scan.json", scan_directory);
    if (z < 0 || (size_t)z >= sizeof path) return 0;
    FILE *out = fopen(path, "wb"); if (!out) return 0;
    fprintf(out, "{\n  \"format\":\"daywater-native-source-navigation-scan-v1\",\n");
    fprintf(out, "  \"boundary\":\"Fresh native-prepared startup copies; supplied logical port0 record/mapping, supplied transition clock6669/audio1/context0; no physical key mapping is inferred. Each case clones mutable bootstrap state and treats DB views as readonly.\",\n");
    fprintf(out, "  \"updates_checkpoint_note\":\"The event/focus checkpoint uses supplied VM context7/FFFFFFFF/0x817f2380. Immediately before the separate legacy native advance checkpoint, the harness supplies shared.linked_screen=5 because the existing advance implementation runs tick context7/5/0x817c0000. This is an update-only harness compatibility input, not an original-engine semantic claim; it does not acknowledge or clear queued requests. An advance false return is recorded as fail-closed.\",\n");
    fprintf(out, "  \"vm_context\":{\"current_screen\":7,\"linked_screen\":4294967295,\"stack_base\":\"0x817f2380\"},\n  \"source_base\":\"0x81100000\",\"source_size\":%zu,\"node_count\":%u,\n  \"candidate_cases\":[\n", size, g.count);
    unsigned candidate_count = 0;
    for (unsigned node = 0; node < g.count; ++node) {
        for (unsigned event = 2; event <= 7; ++event) {
            uint32_t pc; unsigned found;
            if (!nflstreet_ui_event(bytes, size, SCAN_BASE, node, 0, (uint16_t)event, 0, &pc, &found)) { fclose(out); return 0; }
            if (!found) continue;
            if (candidate_count && fputs(",\n", out) == EOF) { fclose(out); return 0; }
            if (!run_case(out, bytes, size, initial, &g, node, event, candidate_count)) { fclose(out); return 0; }
            ++candidate_count;
        }
    }
    fprintf(out, "  ],\n  \"candidate_count\":%u,\n  \"productive_case_repeats\":[\n", candidate_count);
    const unsigned repeat_node[4] = {0, 0, 21, 77};
    const unsigned repeat_event[4] = {6, 7, 7, 7};
    unsigned repeat_count = 0;
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t pc; unsigned found;
        if (!nflstreet_ui_event(bytes, size, SCAN_BASE, repeat_node[i], 0,
                (uint16_t)repeat_event[i], 0, &pc, &found) || !found) continue;
        if (repeat_count && fputs(",\n", out) == EOF) { fclose(out); return 0; }
        if (!run_case(out, bytes, size, initial, &g, repeat_node[i], repeat_event[i], candidate_count + repeat_count)) { fclose(out); return 0; }
        ++repeat_count;
    }
    fprintf(out, "  ],\n  \"repeat_count\":%u,\n  \"path_rule\":\"Edges are original sentinel-flag child links. Each candidate requires a unique acyclic parent chain to root node0; ambiguous, cyclic, or disconnected paths fail closed. The root is assumed already selected by the prepared startup stage; no flags are forced.\"\n}\n", repeat_count);
    int ok = !ferror(out) && fclose(out) == 0;
    return ok;
}

int main(int argc, char **argv) {
    const char *source, *unpacked, *dol, *frames;
    if (argc != 7 || strcmp(argv[1], "--out") != 0) return 2;
    static const char prefix[] = "W:\\Daywater\\";
    size_t output_length = strlen(argv[2]);
    if (output_length >= sizeof scan_directory || strncmp(argv[2], prefix, sizeof prefix - 1) != 0) return 2;
    for (size_t i = sizeof prefix - 1; i < output_length;) {
        while (i < output_length && (argv[2][i] == '\\' || argv[2][i] == '/')) ++i;
        size_t start = i;
        while (i < output_length && argv[2][i] != '\\' && argv[2][i] != '/') ++i;
        size_t component_length = i - start;
        if ((component_length == 2 && argv[2][start] == '.' && argv[2][start + 1] == '.') ||
            (component_length == 1 && argv[2][start] == '.')) return 2;
    }
    strcpy(scan_directory, argv[2]);
    source = argv[3]; unpacked = argv[4]; dol = argv[5]; frames = argv[6];
    errno = 0;
    char *end = NULL;
    unsigned long frame_value = strtoul(frames, &end, 10);
    if (errno || end == frames || !end || *end || !frame_value || frame_value > 10000) return 2;
    scan_frames = (unsigned)frame_value;
    char *probe_argv[] = {argv[0], (char *)source, (char *)unpacked, (char *)dol, (char *)frames};
    return team_screen_probe_main(5, probe_argv);
}
