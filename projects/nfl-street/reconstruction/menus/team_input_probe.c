#include "team_screen.h"
#include "screen.h"
#include "events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    nflstreet_team_screen state;
    unsigned count;
    nflstreet_ui_service_request requests[256];
} input_trace;
static input_trace *observed_trace;
static input_trace observed_storage;

static void record_request(const nflstreet_ui_service_request *request) {
    if (observed_trace && observed_trace->count < 256)
        observed_trace->requests[observed_trace->count++] = *request;
}

static uint32_t read_be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static void describe_object_near(const uint8_t *bytes, size_t size,
                                 uint32_t file_offset) {
    uint32_t nodes_count = read_be(bytes);
    uint32_t nodes_address = read_be(bytes + 4);
    if (nodes_address < 0x81100000u ||
        nodes_address - 0x81100000u > size ||
        (size_t)nodes_count * 20 > size - (nodes_address - 0x81100000u))
        return;
    const uint8_t *nodes = bytes + nodes_address - 0x81100000u;
    unsigned printed = 0;
    for (uint32_t node = 0; node < nodes_count; ++node) {
        const uint8_t *row = nodes + node * 20;
        uint32_t group_count = read_be(row + 4);
        uint32_t groups_address = read_be(row + 8);
        if (groups_address < 0x81100000u ||
            groups_address - 0x81100000u > size ||
            (size_t)group_count * 4 > size - (groups_address - 0x81100000u))
            continue;
        const uint8_t *groups = bytes + groups_address - 0x81100000u;
        for (uint32_t group_index = 0; group_index < group_count; ++group_index) {
            uint32_t group_address = read_be(groups + group_index * 4);
            if (group_address < 0x81100000u || group_address - 0x81100000u > size ||
                size - (group_address - 0x81100000u) < 12) continue;
            const uint8_t *group = bytes + group_address - 0x81100000u;
            uint32_t count = read_be(group + 4), links_address = read_be(group + 8);
            if (links_address < 0x81100000u || links_address - 0x81100000u > size ||
                (size_t)count * 8 > size - (links_address - 0x81100000u)) continue;
            const uint8_t *links = bytes + links_address - 0x81100000u;
            for (uint32_t i = 0; i < count; ++i) {
                const uint8_t *link = links + i * 8;
                unsigned cls = ((unsigned)link[0] << 8) | link[1];
                uint32_t cell_address = read_be(link + 4);
                if (cls == 0xffff || cell_address < 0x81100000u ||
                    cell_address - 0x81100000u > size ||
                    size - (cell_address - 0x81100000u) < 4) continue;
                /* Cell contents remain source-file offsets; construction adds
                   the supplied source base when making a class callback. */
                uint32_t object_offset = read_be(bytes + cell_address - 0x81100000u);
                if (object_offset > size) continue;
                int64_t distance = (int64_t)file_offset - object_offset;
                if (distance >= -0x20 && distance < 0x100 && printed < 12) {
                    printf("  nearby-object node%u group%u link%u class%u object+%x field+%llx\n",
                           node, group_index, i, cls, object_offset,
                           (long long)distance);
                    ++printed;
                }
            }
        }
    }
    if (!printed) printf("  nearby-object none for source+%x\n", file_offset);
}

static int trace_service(void *context, uint8_t *bytes, size_t size,
                         uint32_t base,
                         const nflstreet_ui_service_request *request,
                         uint32_t *result) {
    input_trace *trace = context;
    record_request(request);
    return nflstreet_team_screen_service(&trace->state, bytes, size, base,
                                         request, result);
}

static int trace_memory(void *context, uint8_t *bytes, size_t size,
                        uint32_t base,
                        const nflstreet_ui_service_request *request,
                        uint32_t *result, uint32_t stack_base,
                        uint32_t stack_words[256]) {
    input_trace *trace = context;
    record_request(request);
    return nflstreet_team_screen_memory(&trace->state, bytes, size, base,
                                        request, result, stack_base,
                                        stack_words);
}

static int run_event(uint8_t *bytes, size_t size, nflstreet_team_screen *source,
                     unsigned event, input_trace *trace) {
    uint8_t *input_copy = malloc(size);
    if (!input_copy) return 0;
    memcpy(input_copy, bytes, size);
    nflstreet_team_screen input_state = *source;
    uint32_t value;
    float one = 1.0f;
    memcpy(&value, &one, sizeof value);
    *trace = (input_trace){0};
    trace->state = *source;
    observed_storage.count = 0;
    observed_trace = &observed_storage;
    unsigned executed = 0;
    if (!nflstreet_ui_events_memory(bytes, size, 0x81100000u, 7, 5,
                                    &trace->state.classes.shared.entry_flags,
                                    0, (uint16_t)event, 0, 1, &value,
                                    trace_service, trace, sizeof *trace,
                                    &executed, NULL, trace_memory)) {
        observed_trace = NULL;
        trace->count = observed_storage.count;
        memcpy(trace->requests, observed_storage.requests,
               trace->count * sizeof trace->requests[0]);
        nflstreet_team_screen api_state = input_state;
        int api_ok = nflstreet_team_screen_input(input_copy, size, 0x81100000u,
                                                 &api_state, 0,
                                                 (uint16_t)event, 1.0f);
        int same = !api_ok && !memcmp(input_copy, bytes, size) &&
                   !memcmp(&api_state, source, sizeof api_state);
        free(input_copy);
        if (!same) return 0;
        return 0;
    }
    *source = trace->state;
    observed_trace = NULL;
    trace->count = observed_storage.count;
    memcpy(trace->requests, observed_storage.requests,
           trace->count * sizeof trace->requests[0]);
    nflstreet_team_screen api_state = input_state;
    int api_ok = nflstreet_team_screen_input(input_copy, size, 0x81100000u,
                                             &api_state, 0,
                                             (uint16_t)event, 1.0f);
    int same = api_ok && !memcmp(input_copy, bytes, size) &&
               !memcmp(&api_state, source, sizeof api_state);
    free(input_copy);
    return same;
}

static int transition_matches(const nflstreet_team_screen *state) {
    if (state->transition_count != 1 || state->transition_clock != 12345 ||
        state->transition_audio != 0 || state->transition_context != 7)
        return 0;
    const nflstreet_team_transition_effects *effect = &state->transition_requests[0];
    return effect->audio_requested == 0 && effect->audio_handle == 0 &&
           effect->audio_delay == 0 && effect->broadcast_context == 7 &&
           effect->broadcast_property == 0x80000003u &&
           effect->broadcast_arg0 == 0 && effect->broadcast_arg1 == 0 &&
           read_be(state->transition_object + 12) == 12845 &&
           state->transition_object[19] == 3 && state->player_callback == 0 &&
           state->actors.callback_registered == 0;
}

static int roster_views_match(const nflstreet_team_screen *state) {
    for (unsigned side = 0; side < 2; ++side) {
        const nflstreet_team_activation_side *a = &state->activation.sides[side];
        const nflstreet_team_player_window_side *w = &state->player_window.sides[side];
        if (a->count != w->count || a->count > 20) return 0;
        unsigned selected = 0;
        for (unsigned i = 0; i < a->count; ++i) {
            if (a->roster[i].id != w->rows[i].id ||
                a->roster[i].selected != w->rows[i].slot ||
                a->roster[i].auxiliary != w->rows[i].auxiliary) return 0;
            if (a->roster[i].selected >= 0) {
                if (selected >= 7 || a->selected_ids[selected] != a->roster[i].id)
                    return 0;
                ++selected;
            }
        }
        if (selected != a->selected_count) return 0;
    }
    return 1;
}

static void print_selected_ids(const nflstreet_team_screen *state,
                               const char *label) {
    printf("%s selected-row counts=%u/%u ids=", label,
           state->activation.sides[0].selected_count,
           state->activation.sides[1].selected_count);
    for (unsigned side = 0; side < 2; ++side) {
        if (side) putchar('/');
        putchar('[');
        for (unsigned i = 0; i < state->activation.sides[side].selected_count; ++i)
            printf("%s%x", i ? "," : "", state->activation.sides[side].selected_ids[i]);
        putchar(']');
    }
    putchar('\n');
}

/* The source screen's controller callback receives a caller supplied four
   word side projection. This is a deliberate isolated test input. */
int daywater_team_startup_host(nflstreet_team_screen *screen) {
    if (!screen) return 0;
    nflstreet_team_device_record records[] = {{0, 0}};
    uint32_t mapping[NFLSTREET_TEAM_DEVICE_MAPPING_COUNT];
    for (unsigned i = 0; i < NFLSTREET_TEAM_DEVICE_MAPPING_COUNT; ++i)
        mapping[i] = UINT32_MAX;
    mapping[0] = 0;
    if (!nflstreet_team_screen_devices(screen, records,
                                       sizeof records / sizeof records[0],
                                       mapping)) return 0;
    screen->supplied_transition = 1;
    screen->transition_clock = 12345;
    screen->transition_audio = 0;
    screen->transition_context = 7;
    return 1;
}

/* Optional DAYWATER_TEAM_STARTUP_CONTROLS hook for team_screen_probe.c.
   It executes each already-translated source event from the same authentic
   passive-startup snapshot, so the probes cannot contaminate one another. */
int daywater_team_startup_controls(uint8_t *bytes, size_t size,
                                   nflstreet_team_screen *screen) {
    if (!bytes || !screen || size < 32 || size > 8u * 1024u * 1024u)
        return 0;
    uint8_t *before = malloc(size);
    uint8_t *trial = malloc(size);
    uint8_t *event2_bytes = malloc(size);
    uint8_t *event3_bytes = malloc(size);
    if (!before || !trial || !event2_bytes || !event3_bytes) {
        free(before);
        free(trial);
        free(event2_bytes);
        free(event3_bytes);
        return 0;
    }
    memcpy(before, bytes, size);
    uint32_t node_count = ((uint32_t)bytes[0] << 24) |
                          ((uint32_t)bytes[1] << 16) |
                          ((uint32_t)bytes[2] << 8) | bytes[3];
    printf("Source event table (kind0 script rows):");
    for (unsigned event = 2; event <= 7; ++event) {
        for (uint32_t node = 0; node < node_count; ++node) {
            uint32_t pc = 0;
            unsigned found = 0;
            if (!nflstreet_ui_event(bytes, size, 0x81100000u, node, 0,
                                    (uint16_t)event, 0, &pc, &found)) {
                free(before);
                free(trial);
                free(event2_bytes);
                free(event3_bytes);
                return 0;
            }
            if (found) {
                printf(" node%u/event%u=+%x", node, event, pc);
                if (node == 0 && (event == 6 || event == 7)) {
                    printf("[");
                    for (unsigned k = 0; k < 24 && pc + k < size; ++k)
                        printf("%02x", bytes[pc + k]);
                    printf("]");
                }
            }
        }
    }
    putchar('\n');
    uint32_t root = 0;
    if (size >= 8) {
        uint32_t rows = ((uint32_t)bytes[4] << 24) |
                        ((uint32_t)bytes[5] << 16) |
                        ((uint32_t)bytes[6] << 8) | bytes[7];
        if (rows >= 0x81100000u && rows - 0x81100000u <= size - 4) {
            size_t off = rows - 0x81100000u;
            root = ((uint32_t)bytes[off] << 24) |
                   ((uint32_t)bytes[off + 1] << 16) |
                   ((uint32_t)bytes[off + 2] << 8) | bytes[off + 3];
        }
    }
    printf("Input snapshot: root=%08x root-selected-flag=%02x teams=%u/%u lineup=%u/%u controller-sides=%u,%u,%u,%u\n",
           root, (root >= 0x81100000u && root - 0x81100000u + 8 <= size)
                     ? bytes[root - 0x81100000u + 7] : 0,
           screen->game.startup.selected_id[0],
           screen->game.startup.selected_id[1],
           screen->activation.sides[0].selected_count,
           screen->activation.sides[1].selected_count,
           screen->controller_sides[0], screen->controller_sides[1],
           screen->controller_sides[2], screen->controller_sides[3]);

    for (unsigned event = 2; event <= 7; ++event) {
        memcpy(trial, before, size);
        nflstreet_team_screen state = *screen;
        input_trace trace;
        if (!run_event(trial, size, &state, event, &trace)) {
            size_t partial_changes = 0;
            for (size_t i = 0; i < size; ++i)
                if (trial[i] != before[i]) ++partial_changes;
            printf("Translated event %u value=1: bounded VM rejected after%u VM services (public-wrapper rollback=%s, partial direct-VM byte changes=%zu):", event, trace.count, event == 6 ? "verified" : "not applicable", partial_changes);
            for (unsigned i = 0; i < trace.count; ++i) {
                const nflstreet_ui_service_request *q = &trace.requests[i];
                printf(" [%u k%u p%x c%u n%u t%x", i, q->kind, q->property,
                       q->class_id, q->argc, q->target);
                for (unsigned a = 0; a < q->argc && a < 4; ++a)
                    printf(" a%u=%x", a, q->args[a]);
                printf("]");
            }
            printf(" unexpected-rejection\n");
            free(before); free(trial); free(event2_bytes); free(event3_bytes);
            return 0;
        }
        size_t first = size, changed = 0;
        for (size_t i = 0; i < size; ++i) {
            if (trial[i] != before[i]) {
                if (first == size) first = i;
                ++changed;
            }
        }
        if (event == 6) {
            unsigned selected_play_queries = 0;
            for (unsigned i = 0; i < trace.count; ++i)
                if (trace.requests[i].kind == 1 &&
                    trace.requests[i].property == 0xdu &&
                    trace.requests[i].argc == 1 &&
                    trace.requests[i].args[0] == 0x163u)
                    ++selected_play_queries;
            if (!changed || selected_play_queries == 0 ||
                state.player_spawn_count != screen->player_spawn_count + 2 ||
                state.game.startup.selected_id[0] != screen->game.startup.selected_id[0] ||
                state.game.startup.selected_id[1] != screen->game.startup.selected_id[1] ||
                state.activation.sides[0].selected_count != screen->activation.sides[0].selected_count ||
                state.activation.sides[1].selected_count != screen->activation.sides[1].selected_count) {
                fprintf(stderr, "Event6 assertion mismatch: changed=%zu selected-PLAY-queries=%u spawns=%u->%u game-teams=%u/%u->%u/%u selected-rows=%u/%u->%u/%u roster-views-match=%u\n",
                        changed, selected_play_queries, screen->player_spawn_count,
                        state.player_spawn_count,
                        screen->game.startup.selected_id[0],
                        screen->game.startup.selected_id[1],
                        state.game.startup.selected_id[0],
                        state.game.startup.selected_id[1],
                        screen->activation.sides[0].selected_count,
                        screen->activation.sides[1].selected_count,
                        state.activation.sides[0].selected_count,
                        state.activation.sides[1].selected_count,
                        roster_views_match(&state));
                free(before); free(trial); free(event2_bytes); free(event3_bytes);
                return 0;
            }
        }
        printf("Translated event %u value=1: source-bytes-changed=%zu first=%s%zx teams=%u/%u lineup=%u/%u resources=%u preview-spawns=%u pending=%u broadcasts=%u links=%u\n",
               event, changed, first == size ? "none/" : "", first,
               state.game.startup.selected_id[0],
               state.game.startup.selected_id[1],
               state.activation.sides[0].selected_count,
               state.activation.sides[1].selected_count,
               state.player_resource_count, state.player_spawn_count,
               state.classes.shared.pending_count,
               state.classes.shared.broadcast_count,
               state.classes.shared.link_count);
        printf("  VM services(%u):", trace.count);
        for (unsigned i = 0; i < trace.count; ++i) {
            const nflstreet_ui_service_request *q = &trace.requests[i];
            printf(" [%u k%u p%x c%u n%u t%x", i, q->kind, q->property,
                   q->class_id, q->argc, q->target);
            for (unsigned a = 0; a < q->argc && a < 4; ++a)
                printf(" a%u=%x", a, q->args[a]);
            printf("]");
        }
        putchar('\n');
        if (changed) {
            printf("  changed-source-offsets:");
            unsigned shown = 0;
            for (size_t i = 0; i < size && shown < 32; ++i) {
                if (trial[i] != before[i]) {
                    printf(" %zx:%02x>%02x", i, before[i], trial[i]);
                    ++shown;
                }
            }
            if (changed > shown) printf(" ... (%zu total)", changed);
            putchar('\n');
            if (event == 2 || event == 3) {
                describe_object_near(before, size, 0xfadf);
                describe_object_near(before, size, 0xfb03);
                describe_object_near(before, size, 0xfd6f);
                describe_object_near(before, size, 0x29f18);
                describe_object_near(before, size, 0x2a018);
            }
        }
        if (event == 2) {
            memcpy(event2_bytes, trial, size);
        } else if (event == 3) {
            memcpy(event3_bytes, trial, size);
        }
        if ((event == 2 || event == 3) &&
            (state.game.startup.selected_id[0] != screen->game.startup.selected_id[0] ||
             state.game.startup.selected_id[1] != screen->game.startup.selected_id[1] ||
             state.activation.sides[0].selected_count != screen->activation.sides[0].selected_count ||
             state.activation.sides[1].selected_count != screen->activation.sides[1].selected_count)) {
            free(before); free(trial); free(event2_bytes); free(event3_bytes);
            return 0;
        }
        if ((event == 4 || event == 5) &&
            (changed != 0 || memcmp(&state, screen, sizeof state))) {
            free(before); free(trial); free(event2_bytes); free(event3_bytes);
            return 0;
        }
        if (event == 7 && !transition_matches(&state)) {
            fprintf(stderr, "Event7 transition/callback state disagrees with supplied clock/audio/context\n");
            free(before); free(trial); free(event2_bytes); free(event3_bytes);
            return 0;
        }
        if (event == 7) {
            for (unsigned frame = 0; frame < 60; ++frame) {
                if (!nflstreet_team_screen_advance(trial, size, 0x81100000u,
                                                   &state, 16)) {
                    fprintf(stderr, "Event7 continuation update failed frame%u\n", frame);
                    free(before); free(trial); free(event2_bytes); free(event3_bytes);
                    return 0;
                }
            }
            if (!transition_matches(&state) ||
                state.classes.shared.broadcast_count != 1 ||
                state.classes.shared.link_count != 1 ||
                state.classes.shared.links[0] != 5) {
                fprintf(stderr, "Event7 continuation did not produce one supplied-context completion/link\n");
                free(before); free(trial); free(event2_bytes); free(event3_bytes);
                return 0;
            }
            printf("Event7 after60 updates: broadcast=%u property=%x args=%x,%x link=%u transition-deadline=%u state=%u\n",
                   state.classes.shared.broadcast_count,
                   state.classes.shared.broadcasts[0].property,
                   state.classes.shared.broadcasts[0].args[0],
                   state.classes.shared.broadcasts[0].args[1],
                   state.classes.shared.links[0],
                   read_be(state.transition_object + 12),
                   state.transition_object[19]);
        }
    }
    if (memcmp(event2_bytes, event3_bytes, size)) {
        fprintf(stderr, "Independent event2/event3 source results differ\n");
        free(before); free(trial); free(event2_bytes); free(event3_bytes);
        return 0;
    }
    memcpy(trial, before, size);
    nflstreet_team_screen sequence = *screen;
    static const unsigned sequence_events[] = {2, 3, 6, 7};
    int sequence_no_transition_path = 0;
    for (unsigned i = 0; i < sizeof sequence_events / sizeof sequence_events[0]; ++i) {
        unsigned event = sequence_events[i];
        memcpy(event2_bytes, trial, size);
        nflstreet_team_screen prior = sequence;
        input_trace trace;
        int ok = run_event(trial, size, &sequence, event, &trace);
        if (event == 7 && ok) {
            printf("Sequential event7 services(%u):", trace.count);
            for (unsigned j = 0; j < trace.count; ++j) {
                const nflstreet_ui_service_request *q = &trace.requests[j];
                printf(" [%u k%u p%x n%u", j, q->kind, q->property, q->argc);
                for (unsigned a = 0; a < q->argc && a < 4; ++a)
                    printf(" a%u=%x", a, q->args[a]);
                printf("]");
            }
            putchar('\n');
            if (!transition_matches(&sequence)) {
                unsigned confirm_calls = 0, roster_updates = 0;
                for (unsigned j = 0; j < trace.count; ++j) {
                    const nflstreet_ui_service_request *q = &trace.requests[j];
                    if (q->kind == 1 && q->property == 0x80000008u &&
                        q->argc == 3 && q->args[0] == 0 &&
                        q->args[1] == 0x817c0014u && q->args[2] == 0xbu)
                        ++confirm_calls;
                    if (q->kind == 1 && q->property == 0x80000012u &&
                        q->argc == 2 && q->args[0] == 0 &&
                        q->args[1] == 0x817c005cu)
                        ++roster_updates;
                }
                sequence_no_transition_path = sequence.transition_count == 0 &&
                    sequence.player_callback == 0 &&
                    sequence.actors.callback_registered == 0 &&
                    confirm_calls == 1 && roster_updates == 1 &&
                    sequence.game.startup.selected_id[0] == screen->game.startup.selected_id[0] &&
                    sequence.game.startup.selected_id[1] == screen->game.startup.selected_id[1] &&
                    roster_views_match(&sequence);
                if (sequence_no_transition_path)
                    printf("Sequential event7 after2/3/6 completes source event with one property 0x80000008 query and one property 0x80000012 update; no transition request is present\n");
            }
            if (!sequence_no_transition_path) {
                fprintf(stderr, "Sequential event7 did not match the asserted no-transition source branch\n");
                free(before); free(trial); free(event2_bytes); free(event3_bytes);
                return 0;
            }
        }
        if (event == 6) {
            if (!ok || !memcmp(trial, event2_bytes, size) ||
                sequence.player_spawn_count != prior.player_spawn_count + 2) {
                fprintf(stderr, "Sequential event6 did not complete the selected PLAY lookup\n");
                free(before); free(trial); free(event2_bytes); free(event3_bytes);
                return 0;
            }
        } else if (!ok) {
            fprintf(stderr, "Sequential translated event%u unexpectedly rejected\n", event);
            free(before); free(trial); free(event2_bytes); free(event3_bytes);
            return 0;
        }
    }
    if (!sequence_no_transition_path) {
        const nflstreet_team_transition_effects *effect =
            sequence.transition_count ? &sequence.transition_requests[0] : NULL;
        fprintf(stderr, "Sequential event7 transition mismatch after2/3/6/7: count=%u clock=%u audio=%u context=%u callback=%u actor-callback=%u deadline=%u state=%u property=%x bctx=%u args=%u/%u\n",
                sequence.transition_count, sequence.transition_clock,
                sequence.transition_audio, sequence.transition_context,
                sequence.player_callback, sequence.actors.callback_registered,
                read_be(sequence.transition_object + 12),
                sequence.transition_object[19],
                effect ? effect->broadcast_property : 0,
                effect ? effect->broadcast_context : 0,
                effect ? effect->broadcast_arg0 : 0,
                effect ? effect->broadcast_arg1 : 0);
        free(before); free(trial); free(event2_bytes); free(event3_bytes);
        return 0;
    }
    uint32_t post_event_team_ids[2];
    uint32_t post_event_selected_counts[2];
    uint32_t post_event_selected_ids[2][NFLSTREET_TEAM_ACTIVATION_POSES];
    for (unsigned side = 0; side < 2; ++side) {
        const nflstreet_team_activation_side *a = &sequence.activation.sides[side];
        if (a->selected_count > NFLSTREET_TEAM_ACTIVATION_POSES) {
            fprintf(stderr, "Post-event selected row count exceeds seven on side%u\n", side);
            free(before); free(trial); free(event2_bytes); free(event3_bytes);
            return 0;
        }
        post_event_team_ids[side] = sequence.game.startup.selected_id[side];
        post_event_selected_counts[side] = a->selected_count;
        memcpy(post_event_selected_ids[side], a->selected_ids,
               sizeof post_event_selected_ids[side]);
    }
    for (unsigned frame = 0; frame < 60; ++frame) {
        if (!nflstreet_team_screen_advance(trial, size, 0x81100000u,
                                           &sequence, 16)) {
            fprintf(stderr, "Sequential event7 continuation failed frame%u\n", frame);
            free(before); free(trial); free(event2_bytes); free(event3_bytes);
            return 0;
        }
    }
    if (sequence.transition_count || !roster_views_match(&sequence)) {
        fprintf(stderr, "No-transition event7 path changed transition or roster identity during 60 updates\n");
        free(before); free(trial); free(event2_bytes); free(event3_bytes);
        return 0;
    }
    for (unsigned side = 0; side < 2; ++side) {
        const nflstreet_team_activation_side *a = &sequence.activation.sides[side];
        if (sequence.game.startup.selected_id[side] != post_event_team_ids[side] ||
            a->selected_count != post_event_selected_counts[side] ||
            memcmp(a->selected_ids, post_event_selected_ids[side],
                   sizeof post_event_selected_ids[side])) {
            fprintf(stderr, "Sequential selected row/team IDs changed during 60 no-transition updates on side%u\n", side);
            free(before); free(trial); free(event2_bytes); free(event3_bytes);
            return 0;
        }
    }
    print_selected_ids(&sequence, "Sequential 2/3/6/7 after60 updates");
    printf("Sequential post-update effects: transitions=%u broadcasts=%u links=%u\n",
           sequence.transition_count, sequence.classes.shared.broadcast_count,
           sequence.classes.shared.link_count);
    printf("Team source input checks pass: independent events2/3 mutate identical source artwork/text state; 4/5 are source/state no-ops; 6 completes with selected PLAY lookup 0x163 and two preview spawn requests; standalone event7 schedules supplied transition (500 ticks, clock12345, audio disabled, context7) and completes after60 updates; sequence2/3/6/7 takes the asserted no-transition source branch and preserves selected roster rows over60 updates.\n");
    free(before);
    free(trial);
    free(event2_bytes);
    free(event3_bytes);
    return 1;
}
