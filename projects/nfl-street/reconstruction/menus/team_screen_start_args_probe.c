#include "team_input_probe.c"

static int start_loaded_args(uint8_t *, size_t, uint32_t,
                             nflstreet_team_screen *);
static int startup_args_controls(uint8_t *, size_t, nflstreet_team_screen *);
#define DAYWATER_TEAM_STARTUP_HOST daywater_team_startup_host
#define DAYWATER_TEAM_STARTUP_CONTROLS startup_args_controls
#define nflstreet_team_screen_start(bytes,size,base,state) \
    start_loaded_args((bytes),(size),(base),(state))
#include "team_screen_probe.c"
#undef nflstreet_team_screen_start

#define START_ARGS_OUT "W:\\Daywater\\research-temp\\nflstreet-team-start-args-20261011\\"

static int dump(const char *name, const uint8_t *bytes, size_t size) {
    char path[512];
    int count = snprintf(path, sizeof path, "%s%s", START_ARGS_OUT, name);
    if (count < 0 || (size_t)count >= sizeof path) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(bytes, 1, size, f) == size;
    if (fclose(f) != 0) ok = 0;
    return ok;
}

static unsigned probe_read_be(const uint8_t *p) {
    return (unsigned)p[0] << 24 | (unsigned)p[1] << 16 | (unsigned)p[2] << 8 | p[3];
}

static int write_state(const char *name, const uint8_t *bytes, size_t size,
                       const nflstreet_team_screen *state) {
    char path[512];
    int count = snprintf(path, sizeof path, "%s%s", START_ARGS_OUT, name);
    if (count < 0 || (size_t)count >= sizeof path) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    uint32_t root = UINT32_MAX;
    if (size >= 8) {
        uint32_t rows = probe_read_be(bytes + 4);
        if (rows >= 0x81100000u && rows - 0x81100000u <= size - 4)
            root = probe_read_be(bytes + rows - 0x81100000u);
    }
    fprintf(f,
        "source_size=%zu\nsource_base=0x81100000\nroot=0x%08x\n"
        "root_selected=%u\nentry_flags=0x%08x\nselected_team_ids=%u,%u\n"
        "lineup_counts=%u,%u\ncontroller_mapping_mask=0x%x\n"
        "controller_mapping_0=%u\ncontroller_sides_supplied=%u\n"
        "pending_focus=%u\nresource_requests=%u\ntransition_requests=%u\n",
        size, root,
        root >= 0x81100000u && root - 0x81100000u + 8 <= size
            ? bytes[root - 0x81100000u + 7] : UINT32_MAX,
        state->classes.shared.entry_flags,
        state->game.startup.selected_id[0], state->game.startup.selected_id[1],
        state->activation.sides[0].selected_count, state->activation.sides[1].selected_count,
        state->supplied_port_mapping_mask, state->port_mapping[0],
        state->supplied_controller_sides, state->classes.shared.pending_count,
        state->classes.shared.resource_count, state->transition_count);
    return fclose(f) == 0;
}

static void report_state(const char *label, unsigned event,
                         const nflstreet_team_screen *state) {
    printf("%s event%u teams=%u/%u carousel=[%u,%u,%u]/[%u,%u,%u] dirty/rebuild=%u/%u,%u/%u pending=%u resources=%u transitions=%u\n",
        label, event, state->game.startup.selected_id[0], state->game.startup.selected_id[1],
        state->game.startup.carousel.windows[0][0], state->game.startup.carousel.windows[0][1], state->game.startup.carousel.windows[0][2],
        state->game.startup.carousel.windows[1][0], state->game.startup.carousel.windows[1][1], state->game.startup.carousel.windows[1][2],
        state->activation.sides[0].dirty, state->activation.sides[0].rebuild,
        state->activation.sides[1].dirty, state->activation.sides[1].rebuild,
        state->classes.shared.pending_count, state->classes.shared.resource_count,
        state->transition_count);
}

static int startup_args_controls(uint8_t *bytes, size_t size,
                                 nflstreet_team_screen *state) {
    if (!bytes || !state || size < 32) return 0;
    /* The source route waits 1000 legacy native updates before input. The
       included probe already completed update one. */
    for (unsigned tick = 1; tick < 1000; ++tick)
        if (!nflstreet_team_screen_advance(bytes, size, 0x81100000u, state, 16)) return 0;
    if (!dump("native-pre-input.bin", bytes, size) ||
        !write_state("pre-input-state.txt", bytes, size, state)) return 0;
    for (unsigned i = 0; i < 4; ++i) {
        char name[64];
        int count = snprintf(name, sizeof name, "font-%u.bin", i);
        if (count < 0 || (size_t)count >= sizeof name ||
            !state->objects.fonts[i].bytes || !state->objects.fonts[i].size ||
            !dump(name, state->objects.fonts[i].bytes, state->objects.fonts[i].size)) return 0;
    }
    if (state->bootstrap && state->bootstrap->constants &&
        state->bootstrap->constants->mapped_dol && state->bootstrap->constants->mapped_size &&
        !dump("mapped-dol.bin", state->bootstrap->constants->mapped_dol,
              state->bootstrap->constants->mapped_size)) return 0;
    FILE *manifest = fopen(START_ARGS_OUT "manifest.txt", "wb");
    if (!manifest) return 0;
    unsigned node_count = probe_read_be(bytes);
    fprintf(manifest,
        "format=nflstreet-native-loaded-entry-start-v1\n"
        "boundary=controlled native source; proprietary bytes stay in private W research storage\n"
        "prestart=after-relocation-and-construction-before-FFFE/FFFC\n"
        "post_start=immediately-after-FFFE/FFFC-before-advance-or-input\n"
        "pre_input=after-1000-legacy-native-advance-calls-before-input replay\n"
        "source_base=0x81100000\nsource_size=%zu\nnode_count=%u\n"
        "load_event=0xFFFE\nload_argument=0xFFFFFFFF\nload_argc=2\nload_args=0x00000001,0x00000069\n"
        "reactivation_event=0xFFFC\nreactivation_argument=0\nreactivation_argc=0\n"
        "startup_vm_context=7,5,0x817c0000\n"
        "input_event_context=7,5,0x817c0000 via legacy input wrapper; no forced focus\n"
        "device_projection=record(handle0,side0);mapping[0]=0;others=FFFFFFFF\n"
        "font_count=4\nsource_files=native-prestart.bin,native-post-start.bin,native-pre-input.bin\n"
        "state_files=prestart-state.txt,post-start-state.txt,pre-input-state.txt\n"
        "fonts=font-0.bin,font-1.bin,font-2.bin,font-3.bin\n"
        "mapped_dol_file=mapped-dol.bin\nmapped_dol_address_base=0x80000000\n",
        size, node_count);
    int manifest_ok = fclose(manifest) == 0;
    if (!manifest_ok) return 0;

    unsigned started = 0;
    for (unsigned event = 2; event <= 7; ++event) {
        uint8_t *trial = malloc(size);
        nflstreet_team_screen *trial_state = malloc(sizeof *trial_state);
        if (!trial || !trial_state) { free(trial); free(trial_state); return 0; }
        memcpy(trial, bytes, size);
        *trial_state = *state;
        unsigned pending_before = trial_state->classes.shared.pending_count;
        unsigned resource_before = trial_state->classes.shared.resource_count;
        unsigned transition_before = trial_state->transition_count;
        int ok = nflstreet_team_screen_input(trial, size, 0x81100000u,
            trial_state, 0, (uint16_t)event, 1.0f);
        unsigned changed = 0;
        if (ok) for (size_t i = 0; i < size; ++i) changed += trial[i] != bytes[i];
        printf("Loaded-args native source event%u %s: changed-bytes=%u teams=%u/%u carousel=[%u,%u,%u]/[%u,%u,%u] effects=pending:%u resources:%u transitions:%u\n",
            event, ok ? "accepted" : "unsupported", changed,
            ok ? trial_state->game.startup.selected_id[0] : state->game.startup.selected_id[0],
            ok ? trial_state->game.startup.selected_id[1] : state->game.startup.selected_id[1],
            ok ? trial_state->game.startup.carousel.windows[0][0] : state->game.startup.carousel.windows[0][0],
            ok ? trial_state->game.startup.carousel.windows[0][1] : state->game.startup.carousel.windows[0][1],
            ok ? trial_state->game.startup.carousel.windows[0][2] : state->game.startup.carousel.windows[0][2],
            ok ? trial_state->game.startup.carousel.windows[1][0] : state->game.startup.carousel.windows[1][0],
            ok ? trial_state->game.startup.carousel.windows[1][1] : state->game.startup.carousel.windows[1][1],
            ok ? trial_state->game.startup.carousel.windows[1][2] : state->game.startup.carousel.windows[1][2],
            ok ? trial_state->classes.shared.pending_count - pending_before : 0,
            ok ? trial_state->classes.shared.resource_count - resource_before : 0,
            ok ? trial_state->transition_count - transition_before : 0);
        if (!ok) ++started;
        free(trial); free(trial_state);
    }
    if (started) return 0;

    uint8_t *sequence_bytes = malloc(size);
    nflstreet_team_screen *sequence_state = malloc(sizeof *sequence_state);
    if (!sequence_bytes || !sequence_state) {
        free(sequence_bytes); free(sequence_state); return 0;
    }
    memcpy(sequence_bytes, bytes, size);
    *sequence_state = *state;
    const unsigned sequence[] = {4, 4, 7};
    FILE *sequence_log = fopen(START_ARGS_OUT "sequence-4-4-7.txt", "wb");
    if (!sequence_log) { free(sequence_bytes); free(sequence_state); return 0; }
    fprintf(sequence_log, "case=sequence-4-4-7\nsource=native-pre-input.bin after 1000 legacy advances\ninput_context=legacy 7/5/817c0000; port0,float1; no focus override\ninitial_ids=%u,%u\ninitial_windows=%u,%u,%u;%u,%u,%u\n",
        sequence_state->game.startup.selected_id[0], sequence_state->game.startup.selected_id[1],
        sequence_state->game.startup.carousel.windows[0][0], sequence_state->game.startup.carousel.windows[0][1], sequence_state->game.startup.carousel.windows[0][2],
        sequence_state->game.startup.carousel.windows[1][0], sequence_state->game.startup.carousel.windows[1][1], sequence_state->game.startup.carousel.windows[1][2]);
    report_state("sequence-4-4-7-start", 0, sequence_state);
    for (unsigned i = 0; i < sizeof sequence / sizeof sequence[0]; ++i) {
        unsigned event = sequence[i];
        unsigned resource_before = sequence_state->classes.shared.resource_count;
        unsigned transition_before = sequence_state->transition_count;
        if (!nflstreet_team_screen_input(sequence_bytes, size, 0x81100000u,
                sequence_state, 0, (uint16_t)event, 1.0f)) {
            fclose(sequence_log); free(sequence_bytes); free(sequence_state); return 0;
        }
        report_state("sequence-immediate", event, sequence_state);
        fprintf(sequence_log, "after_event_%u ids=%u,%u windows=%u,%u,%u;%u,%u,%u lineup=%u,%u dirty_rebuild=%u,%u;%u,%u resources_delta=%u transition_delta=%u\n",
            event, sequence_state->game.startup.selected_id[0], sequence_state->game.startup.selected_id[1],
            sequence_state->game.startup.carousel.windows[0][0], sequence_state->game.startup.carousel.windows[0][1], sequence_state->game.startup.carousel.windows[0][2],
            sequence_state->game.startup.carousel.windows[1][0], sequence_state->game.startup.carousel.windows[1][1], sequence_state->game.startup.carousel.windows[1][2],
            sequence_state->activation.sides[0].selected_count, sequence_state->activation.sides[1].selected_count,
            sequence_state->activation.sides[0].dirty, sequence_state->activation.sides[0].rebuild,
            sequence_state->activation.sides[1].dirty, sequence_state->activation.sides[1].rebuild,
            sequence_state->classes.shared.resource_count - resource_before,
            sequence_state->transition_count - transition_before);
        if (i == 2 && !dump("sequence-4-4-7-post-event7.bin", sequence_bytes, size)) {
            fclose(sequence_log); free(sequence_bytes); free(sequence_state); return 0;
        }
        for (unsigned tick = 0; tick < 60; ++tick) {
            if (!nflstreet_team_screen_advance(sequence_bytes, size,
                    0x81100000u, sequence_state, 16)) {
                fclose(sequence_log); free(sequence_bytes); free(sequence_state); return 0;
            }
        }
        report_state("sequence-after-60-updates", event, sequence_state);
        fprintf(sequence_log, "after_event_%u_plus_60_updates ids=%u,%u windows=%u,%u,%u;%u,%u,%u lineup=%u,%u dirty_rebuild=%u,%u;%u,%u resources=%u transitions=%u\n",
            event, sequence_state->game.startup.selected_id[0], sequence_state->game.startup.selected_id[1],
            sequence_state->game.startup.carousel.windows[0][0], sequence_state->game.startup.carousel.windows[0][1], sequence_state->game.startup.carousel.windows[0][2],
            sequence_state->game.startup.carousel.windows[1][0], sequence_state->game.startup.carousel.windows[1][1], sequence_state->game.startup.carousel.windows[1][2],
            sequence_state->activation.sides[0].selected_count, sequence_state->activation.sides[1].selected_count,
            sequence_state->activation.sides[0].dirty, sequence_state->activation.sides[0].rebuild,
            sequence_state->activation.sides[1].dirty, sequence_state->activation.sides[1].rebuild,
            sequence_state->classes.shared.resource_count, sequence_state->transition_count);
        if (i == 2 && !dump("sequence-4-4-7-after-60-updates.bin", sequence_bytes, size)) {
            fclose(sequence_log); free(sequence_bytes); free(sequence_state); return 0;
        }
    }
    fprintf(sequence_log, "final_ids=%u,%u\nfinal_windows=%u,%u,%u;%u,%u,%u\nfinal_lineup=%u,%u\nfinal_dirty_rebuild=%u,%u;%u,%u\nplayer_spawns=%u\npending_focus=%u\nresource_requests=%u\ntransition_requests=%u\n",
        sequence_state->game.startup.selected_id[0], sequence_state->game.startup.selected_id[1],
        sequence_state->game.startup.carousel.windows[0][0], sequence_state->game.startup.carousel.windows[0][1], sequence_state->game.startup.carousel.windows[0][2],
        sequence_state->game.startup.carousel.windows[1][0], sequence_state->game.startup.carousel.windows[1][1], sequence_state->game.startup.carousel.windows[1][2],
        sequence_state->activation.sides[0].selected_count, sequence_state->activation.sides[1].selected_count,
        sequence_state->activation.sides[0].dirty, sequence_state->activation.sides[0].rebuild,
        sequence_state->activation.sides[1].dirty, sequence_state->activation.sides[1].rebuild,
        sequence_state->player_spawn_count, sequence_state->classes.shared.pending_count,
        sequence_state->classes.shared.resource_count, sequence_state->transition_count);
    int sequence_log_ok = fclose(sequence_log) == 0;
    if (!sequence_log_ok) { free(sequence_bytes); free(sequence_state); return 0; }
    free(sequence_bytes); free(sequence_state);
    return 1;
}

static int start_loaded_args(uint8_t *bytes, size_t size, uint32_t base,
                             nflstreet_team_screen *state) {
    if (!bytes || !state) return 0;
    if (!dump("native-prestart.bin", bytes, size) ||
        !write_state("prestart-state.txt", bytes, size, state)) return 0;
    for (unsigned i = 0; i < 4; ++i) {
        char name[64];
        int count = snprintf(name, sizeof name, "font-%u.bin", i);
        if (count < 0 || (size_t)count >= sizeof name ||
            !state->objects.fonts[i].bytes || !state->objects.fonts[i].size ||
            !dump(name, state->objects.fonts[i].bytes, state->objects.fonts[i].size)) return 0;
    }
    if (state->bootstrap && state->bootstrap->constants &&
        state->bootstrap->constants->mapped_dol && state->bootstrap->constants->mapped_size &&
        !dump("mapped-dol.bin", state->bootstrap->constants->mapped_dol,
              state->bootstrap->constants->mapped_size)) return 0;
    uint8_t *before = malloc(size);
    nflstreet_team_screen *state_before = malloc(sizeof *state_before);
    if (!before || !state_before) { free(before); free(state_before); return 0; }
    memcpy(before, bytes, size);
    *state_before = *state;

    uint32_t loaded[2] = {1, 0x69};
    uint32_t excessive[17] = {0};
    if (nflstreet_team_screen_start_context(bytes, size, base, state,
            7, 5, 0x817c0000u, UINT32_MAX, 17, excessive) ||
        memcmp(before, bytes, size) || memcmp(state_before, state, sizeof *state) ||
        nflstreet_team_screen_start_context(bytes, size, base, state,
            7, 5, 0x817c0000u, UINT32_MAX, 2, NULL) ||
        memcmp(before, bytes, size) || memcmp(state_before, state, sizeof *state) ||
        nflstreet_team_screen_start_context(bytes, size, base, state,
            7, 5, base, UINT32_MAX, 2, loaded) ||
        memcmp(before, bytes, size) || memcmp(state_before, state, sizeof *state)) {
        free(before); free(state_before); return 0;
    }
    free(before); free(state_before);
    int ok = nflstreet_team_screen_start_context(bytes, size, base, state,
        7, 5, 0x817c0000u, UINT32_MAX, 2, loaded);
    if (!ok || !dump("native-post-start.bin", bytes, size) ||
        !write_state("post-start-state.txt", bytes, size, state)) return 0;
    FILE *manifest = fopen(START_ARGS_OUT "manifest.txt", "wb");
    if (!manifest) return 0;
    fprintf(manifest,
        "format=nflstreet-native-loaded-entry-start-v1\n"
        "boundary=controlled native source; proprietary bytes stay in private W research storage\n"
        "prestart=after-relocation-and-construction-before-FFFE/FFFC\n"
        "post_start=immediately-after-FFFE/FFFC-before-advance-or-input\n"
        "source_base=0x81100000\nsource_size=%zu\n"
        "load_event=0xFFFE\nload_argument=0xFFFFFFFF\nload_argc=2\nload_args=0x00000001,0x00000069\n"
        "reactivation_event=0xFFFC\nreactivation_argument=0\nreactivation_argc=0\n"
        "startup_vm_context=7,5,0x817c0000\n"
        "input_event_context=7,5,0x817c0000 via legacy input wrapper; no forced focus\n"
        "device_projection=record(handle0,side0);mapping[0]=0;others=FFFFFFFF\n"
        "source_files=native-prestart.bin,native-post-start.bin,native-pre-input.bin\n"
        "state_files=prestart-state.txt,post-start-state.txt,pre-input-state.txt\n"
        "font_count=4\nfonts=font-0.bin,font-1.bin,font-2.bin,font-3.bin\n"
        "mapped_dol_file=mapped-dol.bin\nmapped_dol_address_base=0x80000000\n",
        size);
    int manifest_ok = fclose(manifest) == 0;
    return manifest_ok;
}
