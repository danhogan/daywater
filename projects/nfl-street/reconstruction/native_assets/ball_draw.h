#ifndef NFLSTREET_BALL_DRAW_H
#define NFLSTREET_BALL_DRAW_H
/* Native ball shadows (801652BC with 80164E78): each active shadow entry
   (0x14 bytes: type +00, position pointer +04, size +08, color bytes
   +0C..+0F written in the order 0C, 0E, 0D, 0F, active +10) is a textured
   four-vertex fan. Type 0 entries lie under the stack top times light 0's
   matrix (801ED450) along light 0's direction (801ED4D4: its rotated z
   axis subtracted from z, normalized), rotated 45 degrees in xy and
   normalized (801DE2AC/801DE0CC); type 1 entries lie under the top
   flattened in z along (0.5, 0.5). */
#include "hud_draw.h"
typedef struct {
 const nflstreet_hud_context *hud; /* matrix stacks, profile buckets, draw callbacks */
 uint32_t enabled;        /* r13-58D8: byte */
 uint32_t count;          /* r13-6834: u16 */
 uint32_t entries;        /* 80310898: entry pointers */
 uint32_t texture;        /* 80310A58: the shadow texture object */
 uint32_t lights;         /* 8034FD8C: light matrices 0x40 apart */
 float unit[2];           /* 803A3958: (0, 1) */
 float axis[2];           /* 80268DE0 (0), 80268DE4 (1): the z axis */
 float consts[5];         /* 80260D88 (0), 80260D8C (1), 80260D90 (0.5), 80260D6C (0), 80260D70 (1) */
 float recip[3];          /* 802617D8 epsilon, 802617DC one, 802617E0 zero result */
} nflstreet_ball_context;
int nflstreet_ball_shadows_draw(const nflstreet_ball_context *,nflstreet_render_state *);
/* Native ball (8015CB60). Light set 4 scaled by the ball's light color
   (+294 → +0C..+14, times +298..+2A0 for flag 0x20), material white with
   alpha +28C x 128. Two frustum tests of the box 8028CA20 (801EC298:
   stack slot 3 times light 0's matrix and the ball matrix +20, then the
   ball matrix alone with projected bounds) unless flag 8 (cleared, both
   culled). The level of detail (801EB68C over the table 803897D8) picks
   the model view; materials +284. The ball draws through the stack top
   times +20 in blend preset 2; then, unless flag 2 or culled, chunk 3
   through light 0's matrix at a fixed depth as a black shadow (alpha 0x68
   in preset 4 when the alpha is 1, else alpha x 48 in preset 2). */
typedef struct {
 const nflstreet_ball_context *shadows; /* stacks, light 0, unit lanes */
 uint32_t ball;           /* the ball object */
 uint32_t box;            /* 8028CA20 */
 uint32_t lod;            /* 803897D8: count +0C, entries +10 (0x50: threshold, id +0C, used +10, view +1C) */
 uint32_t slots;          /* 80331EC0: 4x4 slots (2 projection, 3 view) */
 float bounds_init[2];    /* 8026389C, 802638A0 */
 float k[6];              /* 80260614 (1), 80260618 (128), 80260628, 8026062C (shadow depth), 80260630 (48), 80260634 (0) */
 uint32_t refresh_hook,refresh_target; /* 80381C40 (+4) compared with 800044A4: LOD 0 */
} nflstreet_ball_model_context;
int nflstreet_ball_draw(const nflstreet_ball_model_context *,nflstreet_render_state *);
#endif
