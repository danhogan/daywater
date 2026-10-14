#ifndef NFLSTREET_LIVE_MOVEMENT_H
#define NFLSTREET_LIVE_MOVEMENT_H
#include "../command_to_movement/actor_runtime.h"
#include <stddef.h>
/* A small private captured-state fixture, not native actor initialization. */
typedef struct {
    nflstreet_actor_runtime seed,runtime;
    float origin[2];
} nflstreet_live_movement;
typedef struct {float x,y,facing,speed_per_update;} nflstreet_live_motion;
int nflstreet_live_movement_open(const uint8_t *,size_t,nflstreet_live_movement *);
/* Seeds from an actor's own fields (header word, signed ratings +BB4/+BBC,
   +408, the +200 input block and the +1A8 movement block). */
int nflstreet_live_movement_init(nflstreet_live_movement *,uint32_t header,int16_t rating_bb4,int16_t rating_bbc,uint32_t actor_408,
 const uint32_t actor[12],const uint32_t state[21]);
void nflstreet_live_movement_reset(nflstreet_live_movement *);
/* 1 moving (including braking), 0 stopped, -1 unsupported branch. No fallback. */
int nflstreet_live_movement_step(nflstreet_live_movement *,int,int,nflstreet_live_motion *);
void nflstreet_live_movement_observe(const nflstreet_live_movement *,nflstreet_live_motion *);
/* Compare each full actor and movement block against original recordings. */
int nflstreet_live_braking_check(const uint8_t *,size_t,unsigned *);
int nflstreet_live_movement_check(const uint8_t *,size_t,unsigned *);
#endif
