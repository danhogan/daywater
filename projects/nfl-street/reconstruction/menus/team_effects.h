#ifndef NFLSTREET_TEAM_EFFECTS_H
#define NFLSTREET_TEAM_EFFECTS_H

#include "team_screen.h"

#define NFLSTREET_TEAM_EFFECT_LANES 11u
#define NFLSTREET_TEAM_EFFECT_ACK_MAX 1024u

typedef enum {
 NFLSTREET_TEAM_EFFECT_RESOURCES=0,
 NFLSTREET_TEAM_EFFECT_BROADCASTS,
 NFLSTREET_TEAM_EFFECT_PENDING,
 NFLSTREET_TEAM_EFFECT_AUDIO,
 NFLSTREET_TEAM_EFFECT_OPTIONS,
 NFLSTREET_TEAM_EFFECT_ACTORS,
 NFLSTREET_TEAM_EFFECT_ACTIVATION,
 NFLSTREET_TEAM_EFFECT_CONFIRMATION,
 NFLSTREET_TEAM_EFFECT_TRANSITIONS,
 NFLSTREET_TEAM_EFFECT_PLAYER_SPAWNS,
 NFLSTREET_TEAM_EFFECT_PLAYER_RESOURCES
} nflstreet_team_effect_lane;

typedef struct {uint32_t function,argc,args[3];} nflstreet_team_options_effect;
typedef struct {uint32_t kind,side,archive,entry;} nflstreet_team_player_resource_effect;

/* Immutable copy of the queue prefixes present at capture time. The captured
   lengths define the maximum prefix the host may acknowledge from this batch. */
typedef struct {
 uint32_t counts[NFLSTREET_TEAM_EFFECT_LANES];
 nflstreet_ui_button_effect resources[128];
 nflstreet_ui_service_request broadcasts[128],pending[128];
 uint32_t audio[128];
 nflstreet_team_options_effect options[128];
 nflstreet_team_actor_request actors[NFLSTREET_TEAM_ACTOR_REQUEST_CAPACITY];
 nflstreet_team_activation_effect activation[NFLSTREET_TEAM_ACTIVATION_EFFECTS];
 nflstreet_team_confirm_effect confirmation[NFLSTREET_TEAM_CONFIRM_EFFECTS];
 nflstreet_team_transition_effects transitions[16];
 nflstreet_team_player_spawn_request player_spawns[64];
 nflstreet_team_player_resource_effect player_resources[1024];
} nflstreet_team_effect_batch;

typedef struct {uint32_t count[NFLSTREET_TEAM_EFFECT_LANES];} nflstreet_team_effect_ack;

/* Capture all one-shot queue prefixes in their source order. This copies
   requests only; no renderer, archive, audio, focus, or broadcast service runs. */
int nflstreet_team_effects_capture(const nflstreet_team_screen *,nflstreet_team_effect_batch *);

/* Atomically acknowledge exact prefixes from a prior batch. A lane may be
   partially acknowledged; appends made since capture remain queued. Any queue
   count shrink, prefix mutation, invalid count, or changed batch rejects with
   the entire screen unchanged. Persistent screen owners are never modified. */
int nflstreet_team_effects_ack(nflstreet_team_screen *,const nflstreet_team_effect_batch *,
 const nflstreet_team_effect_ack *);

#endif
