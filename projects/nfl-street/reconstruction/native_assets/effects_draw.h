#ifndef NFLSTREET_EFFECTS_DRAW_H
#define NFLSTREET_EFFECTS_DRAW_H
/* The in-match frame's per-actor strips and particle emitters: scene node
   entries 15 (80040C84 -> 801649C4 -> 801590F4) and 16 (800CF284 ->
   80158D24, 80158798 per emitter). Both issue the original GX calls and the
   immediate vertex data written to the GX pipe, and the same matrix-stack
   writes, through the shared draw engine and matrix library.

   Strips (801649C4): for each actor of the list (r13-7BB8 count, r13-7BBC
   pointers) whose record (+4A8) flag +04 bit 0 is clear, and whose +14 bit 3
   is clear (or the scaled-form gate 800DFAF0 is not modeled), with +14 bit
   16 set: 801590F4 on actor + 0x1368 draws four triangle strips (positions
   from an eight-entry ring at strip +00 (head +78, count +7C, edge selector
   +80, edge points +60/+6C), colors from the strip object, faded by age or
   taken from the table 80270CA4 when its mode byte +23C is set).

   Particles (800CF284): when r13-7304 is set, the top as position and
   normal matrix 0 (80158D24), then 80158798 for each of the r13-7310
   emitters of the list r13-730C: a quad per particle, textured (shader
   r13-68FC with its 80158580 setup, texture object and TLUT from the
   emitter's record +2AC) or flat (r13-68F8 with 80158648), size by +30 or
   +4C, fades 80197594, sprite cell 8017A7C8.

   Not modeled (the draw returns 0 where the original would run them): the
   scaled-actor gate 800DFAF0 when r13-7238 is set (800DF1EC), the particle
   color gradient 80198D74 (emitter +3D set with a gradient +8C), 800CF66C's
   80158E00 (r13-72FC == 1), 80167874 (r13-58BC nonzero), and a sprite sheet
   whose divisor fields (+4C, +4E) are zero (the original divides by zero). */
#include "scenery_draw.h"
#include "matrix_stack.h"
/* Entry 11 (80040C60 -> 801646CC): each actor's textured fans. With the
   actor's +133D byte set, its +133C records (0x90 bytes from actor + 0x10FC:
   vertices of 3 floats, texture st pairs at +54, count byte at +8C) draw as
   GX fans of up to seven vertices in the actor's color (actor + 0x109C:
   r, g, b, a floats), with fog forced on (801EDDF8) and the shadow texture
   (r13-58E4/-58E0). It runs before the ball in the frame. */
typedef struct {
 const nflstreet_scenery_context *scene; /* fog, shadow texture and TLUT, actor list and count */
 const nflstreet_matrix_stacks *stacks;
 float byte_scale;      /* 80260B88: 255.0 */
 float depth[2];        /* 80260B8C, 80260B90: the depth range restored afterwards */
} nflstreet_fans_context;
int nflstreet_fans_draw(const nflstreet_fans_context *,nflstreet_render_state *);
typedef struct {
 const nflstreet_player_draw_context *draw;  /* span, memory, GX sink, NULL shader */
 const nflstreet_matrix_stacks *stacks;      /* the selected stack at the point of the call */
 uint32_t actors;       /* value of r13-7BBC: actor pointer list */
 uint32_t actor_count;  /* r13-7BB8: u16 */
 uint32_t gate;         /* r13-72C0: byte, 801590F4 draws only when set */
 uint32_t scaled_gate;  /* r13-7238: byte, 800DFAF0's scaled form (not modeled) */
 uint32_t history;      /* 80270CA4: colors for strip mode 1 (r, g, b, a floats, 16 bytes each) */
 float byte_scale;      /* 80260438: 255.0 */
} nflstreet_strips_context;
int nflstreet_strips_draw(const nflstreet_strips_context *,nflstreet_render_state *);
typedef struct {
 const nflstreet_player_draw_context *draw;
 const nflstreet_matrix_stacks *stacks;
 uint32_t enable;       /* r13-7304: byte */
 uint32_t count;        /* r13-7310: u16 */
 uint32_t list;         /* value of r13-730C: emitter pointers */
 uint32_t gate_72fc;    /* r13-72FC: byte, 1 would run 80158E00 (not modeled) */
 uint32_t gate_58bc;    /* r13-58BC: word, nonzero would run 80167874 (not modeled) */
 uint32_t shader_flat,shader_textured; /* values of r13-68F8 and r13-68FC */
 uint32_t corners;      /* 802898C0: four corner offsets (x, y, z floats) */
 float color_unit;      /* 802603D8: 0.01, times emitter +35B */
 float zero,one;        /* 802603DC, 802603E0: the clamp of the textured color */
 float angle_scale;     /* 802603E4: spin to a 24-bit angle */
 float byte_scale;      /* 802603E8: 255.0 */
 float cell[2];         /* 80261218, 8026121C: 0.0, 1.0 (an unanimated sprite) */
 float fade_one;        /* 8026323C: 1.0 (fades) */
} nflstreet_particles_context;
int nflstreet_particles_draw(const nflstreet_particles_context *,nflstreet_render_state *);
#endif
