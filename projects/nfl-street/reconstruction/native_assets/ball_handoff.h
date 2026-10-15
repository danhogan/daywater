#ifndef NFLSTREET_BALL_HANDOFF_H
#define NFLSTREET_BALL_HANDOFF_H
#include "ball_update.h"
/* GNNE69 r0 ownership services on original-layout big-endian MEM1.
   These are event consumers, NOT automatic catch/contact/AI eligibility.
   No native stand-ins for unreconstructed event services. Failure can leave
   matrix scratch or state writes: stop/discard the update on rejection. */
int nflstreet_ball_handoff_assign(const nflstreet_ball_update_context *,uint32_t ball,uint32_t actor);
/* 800C25FC: exits 0,1,2,3,5,6,8,9; entries 0,1,2,3 and 5 with argument 0.
   State IDs are opaque. Reads the real clock pointer; no host clock input. */
int nflstreet_ball_handoff_transition(const nflstreet_ball_update_context *,uint32_t ball,uint32_t state,uint32_t argument);
/* 800BE204 (keep_velocity=0) / 800BE264 (keep_velocity=1). */
int nflstreet_ball_handoff_release(const nflstreet_ball_update_context *,uint32_t ball,uint32_t state,uint32_t argument,unsigned keep_velocity);
/* Complete 800815B4; active ball selected by pool byte +1, not first record.
   Probable snap event from caller/animation context; enum meaning unverified. */
int nflstreet_ball_handoff_snap(const nflstreet_ball_update_context *,uint32_t actor);
/* Complete 8007BEE8 only when ball==0 or queried context!=3. Context 3
   reaches unreconstructed event/log/controller services and is rejected. */
int nflstreet_ball_handoff_event(const nflstreet_ball_update_context *,uint32_t ball,uint32_t actor);
#endif
