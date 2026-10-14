#ifndef NFLSTREET_LIGHT_RIG_H
#define NFLSTREET_LIGHT_RIG_H
/* The per-frame light update 801ED3A0 on saved RAM. The match registers it
   (801ED47C, from 80004870) as the phase-1 callback of the field camera's
   view node (r13-5CE4 = 806A1CA8), so 801DEF50 runs it after the camera
   handler (801DF458 -> 8017D16C) and 801DFAB0 have left the field view on
   top of the viewport's matrix stack, and before the scene draw (801DF4BC).
   It turns the active rig's world-space lights into the view-space GX light
   objects of the light sets (8034FE9C) that 801EDC60 loads for the players
   and the ball, and rebuilds the shadow projections and colors.

   Rig (8034FD88; 801ED408 sets it per venue): +00 light count, +04 lights
   (0x30: type, world direction +04, color +10, callback +20, view
   direction +24), +08 set count, +0C sets (five 0x14 entries: light, color
   scale), +10 shadow count, +14 shadows (0x20: type, color +04, light +14,
   angle +18, scale +1C), +94 color scale. */
#include "matrix_stack.h"
typedef struct {
 const nflstreet_player_draw_context *x; /* memory */
 nflstreet_matrix_stacks stacks; /* the viewport's stack (801DF594/801893C8): its top is the view */
 uint32_t rig;           /* 8034FD88: word, the active rig */
 uint32_t shadows;       /* 8034FD8C: 4x4 shadow projections, 0x40 apart */
 uint32_t shadow_colors; /* 8034FE8C: one word per shadow */
 uint32_t light_sets;    /* 8034FE9C: 0x108 per set (count, ambient, four GXLightObj) */
 uint32_t shadow_limit;  /* r13-6228: float, largest shadow offset */
 uint32_t sun;           /* 802A3030 (80027764): the venue's sun direction, read by the light callbacks */
} nflstreet_light_rig_context;
/* 801ED3A0: with no rig nothing happens. Light callbacks 8015B858/8015B8A4
   (sun direction) and 8015B8C4 (negated sun x/y, z 0) are modeled; any
   other callback is rejected (0) before any write. Clears the render
   state's light set (r13-6220, 801EDCCC). */
int nflstreet_light_rig_update(const nflstreet_light_rig_context *,nflstreet_render_state *);
#endif
