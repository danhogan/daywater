#ifndef NFLSTREET_ACTOR_WORLD_H
#define NFLSTREET_ACTOR_WORLD_H
/* Placing an actor's visual from its gameplay state, on saved RAM. */
#include "matrix_stack.h"
/* 800D4B0C: the visual's world matrix (+3AC) on a pushed identity:
   translate to +04, turn about z by +24 plus a quarter turn, about x by a
   quarter turn (model y up to field z up), and for flag 0x2000 also
   translate by +34 and turn about y by +4C. */
int nflstreet_actor_world(nflstreet_draw *,const nflstreet_matrix_stacks *,uint32_t visual);
/* 800D4A6C: copy the gameplay actor's position (+1A8) and heading (+1C0)
   to its visual (+04 and +24; mirrored, x and y negated and the heading
   turned half way, when the side record at the word side (r13-7078) has
   +39 = 1), +30C to the visual's +3EC, then build the world matrix. */
int nflstreet_actor_place(nflstreet_draw *,const nflstreet_matrix_stacks *,uint32_t actor,uint32_t side);
#endif
