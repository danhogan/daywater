#ifndef NFLSTREET_LIVE_AI_H
#define NFLSTREET_LIVE_AI_H
#include <stdio.h>
#include "actor_ai_dispatch.h"
#include "live_movement.h"
/* Live integration only. Policies/effects absent from actor_ai_dispatch are
   still missing; this adapter never replaces an actor's saved command queue. */
typedef struct {
 uint32_t actor;nflstreet_live_movement movement;
 int rejected,moved,held;
} nflstreet_live_ai_actor;
typedef struct {
 nflstreet_ai_dispatch dispatch;
 nflstreet_draw_span span;void *memory;FILE *log;
 uint8_t scratch[8];
 uint8_t *rollback;
 nflstreet_live_ai_actor actors[14];unsigned count,updates,last_moved;
} nflstreet_live_ai;
int nflstreet_live_ai_init(nflstreet_live_ai *,nflstreet_draw_span,void *,uint32_t r13,
 uint32_t roster,unsigned count,uint32_t controlled,FILE *);
/* Dispatch all, commit all, finish all, then movement. Missing AI services
   roll back their RAM writes and hold that actor. Clip/event failures stop
   the update because their providers also retain state outside MEM1. */
int nflstreet_live_ai_step(nflstreet_live_ai *,nflstreet_locomotion_context *,unsigned frame);
int nflstreet_live_ai_sync(nflstreet_live_ai *);
void nflstreet_live_ai_report(const nflstreet_live_ai *);
void nflstreet_live_ai_close(nflstreet_live_ai *);
#endif
