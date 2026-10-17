#ifndef NFLSTREET_DOWN_CYCLE_H
#define NFLSTREET_DOWN_CYCLE_H
#include "ai_policy.h"
#include "ai_policy_transaction.h"
/* NFL Street GNNE69 r0. Candidate pending the external all-RAM gate.
   Source-owned football rules; explicit host scheduling around menu/replay UI.
   Failure retains partial writes: stop/discard that update, never continue. */
typedef struct {
 nflstreet_ai_policy native;
 unsigned updates,whistles;
} nflstreet_down_cycle;
typedef struct {
 uint32_t phase,previous_phase,down,possession,flags;
 uint32_t scores[2];float to_go,spot;
 int whistle,select_play;
 const char *reason; /* diagnostic interpretation of the original result */
} nflstreet_down_cycle_event;
void nflstreet_down_cycle_init(nflstreet_down_cycle *,nflstreet_actor_ai);
/* Independent source entry for the oracle. args r3..r8; weight f1.
   The same SDK caller/scheduler scope as ai_policy_host.h; no original DLL. */
int nflstreet_down_cycle_call(nflstreet_down_cycle *,uint32_t pc,const uint32_t args[6],double weight,uint32_t *result);
/* Call AFTER existing actor/channels/ball updates, once per game tick.
   Phase 3: 800BE3F0 -> 800F9C50, with the real active carrier (possibly NULL).
   Phase 4: advance original result callback queue 8004EBF0; when drained,
   original transition 8002E45C(5). This host boundary omits replay/menu waits,
   not football rules. Phase 5 requests play selection by the caller.
   No duplicate AI/physics or presentation update is executed here. */
int nflstreet_down_cycle_update(nflstreet_down_cycle *,nflstreet_down_cycle_event *);
/* After caller selects/installs plays, original lineup positioning for both
   teams, then transition to pre-snap phase 2. Host chooses when to leave UI. */
int nflstreet_down_cycle_lineup(nflstreet_down_cycle *);
/* Host snap request in phase 2: original 800F946C setup/transition, then
   800815B4 for the offensive play's actual +16 actor slot. Snap input timing
   is a host choice; source assignments, ball ownership and rules are native. */
int nflstreet_down_cycle_snap(nflstreet_down_cycle *);
/* Extended source closure for live AI/clip/event calls. Same context ABI and
   scoped SDK boundary as ai_policy.h, with the down module's audited service
   entries and complete command/callback target set. The older AI module is
   unchanged. Use a separate context with the AI transaction's span. */
void nflstreet_down_policy_init(nflstreet_ai_policy *,nflstreet_actor_ai);
int nflstreet_down_policy_call(nflstreet_ai_policy *,const nflstreet_ai_call *,double,uint32_t *);
int nflstreet_down_policy_service(void *,const nflstreet_ai_call *,uint32_t *);
/* Host-controlled input, full original update instead of the observed-player
   partial movement adapter. Input bytes are owned outside MEM1; failure
   restores every MEM1 write across input -> commit -> complete movement. */
typedef struct {
 nflstreet_ai_policy native;
 nflstreet_ai_transaction journal;
 uint32_t actor,failure_call;
 uint8_t input[32];
} nflstreet_down_control;
int nflstreet_down_control_init(nflstreet_down_control *,nflstreet_actor_ai,uint32_t actor);
int nflstreet_down_control_step(nflstreet_down_control *,int dx,int dy);
void nflstreet_down_control_close(nflstreet_down_control *);
#endif
