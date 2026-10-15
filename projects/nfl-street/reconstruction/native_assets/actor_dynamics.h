#ifndef NFLSTREET_ACTOR_DYNAMICS_H
#define NFLSTREET_ACTOR_DYNAMICS_H
#include "actor_dynamics_math.h"
/* The caller has selected the viewport matrix stack, then pushed and loaded
   actor+3AC exactly as 80162760 does before either original routine. */
typedef struct {const nflstreet_player_draw_context *x;nflstreet_matrix_stacks stacks;} nflstreet_actor_dynamics_context;
int nflstreet_actor_hand_dynamics(nflstreet_actor_dynamics_context *,uint32_t actor,uint32_t state,int enabled); /* 8004BF1C */
int nflstreet_actor_head_dynamics(nflstreet_actor_dynamics_context *,uint32_t actor,uint32_t state,int enabled); /* 8004C268 */
#endif
