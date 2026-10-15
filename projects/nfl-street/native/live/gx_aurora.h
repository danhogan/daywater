#ifndef NFLSTREET_GX_AURORA_H
#define NFLSTREET_GX_AURORA_H
/* Replays the native draws' recorded GX calls (nflstreet_gx_call) through
   Aurora's GX implementation as they are issued. GameCube addresses in the
   calls resolve into the caller's RAM image; texture, TLUT and light objects
   are decoded from their SDK layouts and rebuilt with Aurora's init calls;
   immediate vertex data goes to the GX FIFO; vertex arrays are sized from
   the highest index their display lists use. */
#include "player_draw.h"
typedef struct nflstreet_gx_aurora nflstreet_gx_aurora;
nflstreet_gx_aurora *nflstreet_gx_aurora_open(uint8_t *ram,uint32_t ram_bytes);
void nflstreet_gx_aurora_close(nflstreet_gx_aurora *);
/* The nflstreet_gx_emit callback (sink = the bridge). */
int nflstreet_gx_aurora_emit(void *bridge,const nflstreet_gx_call *);
/* Frame start: projection and viewport from the saved GX shadow (type +420,
   parameters +424, viewport +43C) and the 801C769C defaults (depth LEQUAL
   with update, every texture coordinate MTX2x4 from TEX0 with identity,
   konst selections from +1B0). */
int nflstreet_gx_aurora_begin_frame(nflstreet_gx_aurora *,uint32_t gx_shadow);
/* Frame start: projection and viewport from the saved GX shadow (type +420,
   parameters +424, viewport +43C) and the 801C769C defaults (depth LEQUAL
   with update, every texture coordinate MTX2x4 from TEX0 with identity,
   konst selections from +1B0). */
int nflstreet_gx_aurora_begin_frame(nflstreet_gx_aurora *,uint32_t gx_shadow);
/* Overlay EFB service: real Aurora Z24 readback, delayed until a capture completes. */
int nflstreet_gx_aurora_depth(void *,uint32_t x,uint32_t y,uint32_t *depth);
/* Optional light view delta D (3x4, row-major), applied while set: light
   positions and directions are re-expressed for a new camera (D = new view
   x inverse of the view they were stored with). NULL turns it off. */
void nflstreet_gx_aurora_set_light_delta(nflstreet_gx_aurora *,const float *delta);
/* Frame start: the matrix palettes (arrays 21/22) are set again, as their contents change each frame. */
void nflstreet_gx_aurora_new_frame(nflstreet_gx_aurora *);
/* Closes an open primitive. */
void nflstreet_gx_aurora_flush(nflstreet_gx_aurora *);
unsigned nflstreet_gx_aurora_calls(const nflstreet_gx_aurora *);
#include <stdio.h>
extern FILE *nflstreet_gx_aurora_trace;
#endif
