#ifndef NFLSTREET_FIELD_SCREEN_RUNTIME_H
#define NFLSTREET_FIELD_SCREEN_RUNTIME_H

#include "field_bootstrap.h"
#include "field_catalog.h"
#include "focus.h"
#include "font.h"
#include "main_menu.h"
#include "team_classes.h"
#include "team_transition.h"

typedef struct { uint8_t acquire; uint16_t archive, entry; } nflstreet_field_resource_effect;
typedef struct {
    uint32_t ui_slot, raw_key, supplied_owner_value;
} nflstreet_field_commit_request;

typedef struct {
    nflstreet_font fonts[4];
    nflstreet_team_classes classes;
    nflstreet_ui_main_menu main_menu;
    nflstreet_field_bootstrap field_bootstrap;
    nflstreet_field_catalog_entry fields[8];
    char supplied_text42[33];
    int32_t supplied_stat42;
    uint8_t supplied_c9, c9_from_no_profile;
    uint32_t supplied_selected_index;
    uint32_t supplied_finish_owner_value;
    nflstreet_field_commit_request field_commit_requests[4];
    unsigned field_commit_request_count, field_cache_release_requests;
    uint32_t controller_sides[4];
    uint32_t port_mapping[10], supplied_port_mapping_mask;
    uint32_t supplied_static_archives, supplied_rate_clock;
    unsigned image_objects, text_objects, text_objects_skipped, text_updates;
    unsigned image_requests, cache_build_requests, cache_read_requests;
    unsigned selected_index_reads, controller_projection_calls, unsupported_calls;
    uint32_t image_ids[256];
    unsigned image_id_count;
    nflstreet_field_resource_effect resource_effects[128];
    unsigned resource_effect_count;
    nflstreet_ui_focus_effects focus_effects[128];
    unsigned focus_effect_count;
    /* The source requests 80137B84(500); clock, audio and context belong to
       the host. The helper mutates this owned object and records deferred
       audio/broadcast effects without dispatching them. */
    uint8_t transition_object[NFLSTREET_TEAM_TRANSITION_OBJECT_MIN_SIZE];
    uint32_t supplied_transition, transition_clock, transition_audio,
             transition_context;
    nflstreet_team_transition_effects transition_requests[16];
    nflstreet_team_transition_completion transition_completions[16];
    unsigned transition_request_count, transition_completion_count;
    unsigned startup_fffe_completed, startup_fffc_completed;
    uint32_t runtime_event_count, runtime_tick_count, last_runtime_event;
    unsigned runtime_finish_completed, runtime_finish_in_progress;
    /* A link callback is a source-observed route event. Its queue entry is a
       one-shot notification that a host may acknowledge independently. */
    unsigned linked_route_observed;
    uint32_t linked_route_target;
} nflstreet_field_screen_state;

typedef struct {
    uint8_t *source;
    size_t source_size;
    nflstreet_field_screen_state *state;
    uint32_t current_screen, linked_screen, stack_base;
} nflstreet_field_screen_runtime;

/* Prepare an entry8 source from supplied template/DOL/font inputs. This performs
   RVNE reads, no-profile C9 resolution when requested, DOL UI-slot reordering,
   relocation, and bounded class construction. The caller retains all byte buffers
   and may preset the explicitly supplied port mapping/controller words in state;
   prepare does not infer or replace those values. */
int nflstreet_field_screen_runtime_prepare(uint8_t *source, size_t source_size,
    const uint8_t *template_bytes, size_t template_size,
    const uint8_t *dol_bytes, size_t dol_size, const char *font_directory,
    nflstreet_field_screen_state *, int32_t stat42, uint32_t c9,
    int no_profile, uint32_t selected_index, uint32_t static_archive_mask,
    uint32_t initial_rate_clock);
/* Start an already relocated and constructed entry8 source. FFFE receives
   exactly the supplied load args; FFFC is issued with argc0/argsNULL. */
int nflstreet_field_screen_runtime_start(uint8_t *, size_t,
    nflstreet_field_screen_state *, uint32_t current_screen,
    uint32_t linked_screen, uint32_t stack_base, uint32_t load_argument,
    unsigned load_argc, const uint32_t *load_args);
/* Translated numeric source event2..7 and elapsed scheduler tick. */
int nflstreet_field_screen_runtime_event(void *, unsigned event);
int nflstreet_field_screen_runtime_tick(void *, uint32_t elapsed);
/* Run FFFD only after the field's phase3 completion and supplied linked route.
   The finish owner value remains an explicit caller input; value1 captures a
   bounded selected-field request but never starts match creation. */
int nflstreet_field_screen_runtime_finish(nflstreet_field_screen_runtime *);
/* Current selection remains caller/source-owned; this helper only resolves the
   DOL UI-slot to raw DIVE key when the caller supplies a valid slot. */
int nflstreet_field_screen_runtime_key_for_slot(
    const nflstreet_field_screen_state *, uint32_t ui_slot, uint32_t *raw_key);

/* Acknowledge an exact consumed queue prefix. On mismatch or invalid count,
   the state is unchanged. The host calls these only after handling the effects. */
int nflstreet_field_screen_runtime_ack_resource_effects(
    nflstreet_field_screen_state *, const nflstreet_field_resource_effect *, unsigned count);
int nflstreet_field_screen_runtime_ack_button_resources(
    nflstreet_field_screen_state *, const nflstreet_ui_button_effect *, unsigned count);
int nflstreet_field_screen_runtime_ack_audio(
    nflstreet_field_screen_state *, const uint32_t *, unsigned count);
int nflstreet_field_screen_runtime_ack_broadcasts(
    nflstreet_field_screen_state *, const nflstreet_ui_service_request *, unsigned count);
int nflstreet_field_screen_runtime_ack_links(
    nflstreet_field_screen_state *, const uint32_t *, unsigned count);
int nflstreet_field_screen_runtime_ack_focus_effects(
    nflstreet_field_screen_state *, const nflstreet_ui_focus_effects *, unsigned count);

#endif
