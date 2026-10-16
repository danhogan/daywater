#ifndef NFLSTREET_LIVE_AI_H
#define NFLSTREET_LIVE_AI_H
#include <stdio.h>
#include "actor_ai_dispatch.h"
#include "ai_policy_transaction.h"
#include "live_movement.h"
/* Live integration only. Policies/effects absent from actor_ai_dispatch are
   still missing; this adapter never replaces an actor's saved command queue. */
typedef struct {
 uint32_t actor;nflstreet_live_movement movement;
 int rejected,moved,held,moved_second;
} nflstreet_live_ai_actor;
typedef struct {
 nflstreet_ai_dispatch dispatch;
 nflstreet_draw_span span;void *memory;FILE *log;
 uint8_t scratch[8];
 nflstreet_ai_transaction transaction;
 nflstreet_live_ai_actor actors[14];unsigned count,updates,last_moved;
} nflstreet_live_ai;
int nflstreet_live_ai_init(nflstreet_live_ai *,nflstreet_draw_span,void *,uint32_t r13,
 uint32_t roster,unsigned count,uint32_t controlled,FILE *);
/* Dispatch all, commit all, then original 800F86BC per actor (reset, input
   finish and complete movement). Policy/selector/movement native failures
   roll back all their MEM1 writes and hold that actor with exact diagnostics.
   Native policy calls share owned temporaries and have no external effects. */
int nflstreet_live_ai_step(nflstreet_live_ai *,nflstreet_locomotion_context *,unsigned frame);
int nflstreet_live_ai_sync(nflstreet_live_ai *);
unsigned nflstreet_live_ai_moving(nflstreet_live_ai *);
void nflstreet_live_ai_report(const nflstreet_live_ai *);
void nflstreet_live_ai_close(nflstreet_live_ai *);
#endif
