#ifndef NFLSTREET_ACTOR_PALETTES_H
#define NFLSTREET_ACTOR_PALETTES_H
/* An actor's skinning palettes on saved RAM, as 80162760 builds them each
   frame before the player draw (called from 80164000 with no lineup
   record): the body, its attachments, the head and, by LOD, the hands and
   held items. Matrices come from the viewport's stack and are built in the
   locked cache (0xE0000000) and copied out as the original does. */
#include "matrix_stack.h"
#define NFLSTREET_LOCKED_CACHE_BYTES 0x4000u
typedef struct {
 const nflstreet_player_draw_context *x; /* memory */
 nflstreet_matrix_stacks stacks;
 float unit[2];        /* 803A3958: concat lanes */
 uint32_t pool;        /* r13-5680: two palette buffer bases */
 uint32_t pool_index;  /* r13-5678 */
 uint32_t pool_used;   /* r13-5670: matrices allocated this frame */
 uint32_t cache_on;    /* r13-6200: byte, locked cache enabled */
 uint8_t locked[NFLSTREET_LOCKED_CACHE_BYTES];
} nflstreet_actor_palette_context;
/* 80162760(actor, scale, world, 0, 0, -, items, lod): scale is an optional
   x/y/z vector (800DFB34), world the address of the actor's 4x4 matrix
   (+3AC), items the address of the two held-item models.
   Flag 0x4000 selects gekko_quaternion/actor_slerp.c (801EAD98/801CA900),
   verified against the original with the rest of this routine.
   The secondary head and hand dynamics 8004C268/8004BF1C run natively
   (actor_dynamics.c). */
int nflstreet_actor_palettes(nflstreet_actor_palette_context *,uint32_t actor,const float *scale,uint32_t world,uint32_t items,
 uint32_t lod);
#endif
