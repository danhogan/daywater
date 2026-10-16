#ifndef NFLSTREET_PLAY_SELECTION_H
#define NFLSTREET_PLAY_SELECTION_H
#include "ai_policy.h"
/* GNNE69 r0. Candidate until the external all-RAM gate accepts this revision.
   The caller retains this private-stack context across select/install calls. */
typedef nflstreet_ai_policy nflstreet_play_selection;
typedef struct {uint32_t team,id;int32_t script_result;char name[29];} nflstreet_selected_play;
void nflstreet_play_selection_init(nflstreet_play_selection *,nflstreet_actor_ai);
/* Source-level entry for the independent oracle and diagnostics. No DLL or
   original-backed provider. The host scheduler scope matches ai_policy.h. */
int nflstreet_play_selection_call(nflstreet_play_selection *,uint32_t pc,const uint32_t args[6],uint32_t *result);
/* Original script choice 0, flags 3, expansion; no host random generator.
   team must be 0 or 1. It selects the offensive/defensive original script
   by the game's current possession, not by the host controller index. */
int nflstreet_play_select(nflstreet_play_selection *,uint32_t team,nflstreet_selected_play *);
/* 800EF234: current expanded play -> actual actor command queues, including
   mirror/override handling and original entry callbacks. Does not reposition
   actors or advance the original huddle/snap/physics/clock loop. */
int nflstreet_play_install(nflstreet_play_selection *,uint32_t team);
#endif
