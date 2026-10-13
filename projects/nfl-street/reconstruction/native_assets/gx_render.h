#ifndef NFLSTREET_GX_RENDER_H
#define NFLSTREET_GX_RENDER_H
#include "player_draw.h"
/* Bounded software executor for the GX calls recorded by player_draw: it
   keeps GX state from the calls, runs display lists (indexed matrix loads and
   indexed/direct vertices), transforms and lights vertices (one color
   channel with separate color/alpha controls, clamped diffuse, no
   attenuation), clips, culls and rasterizes
   triangles with integer TEV stages, alpha compare, depth test and blending.
   Semantics follow the public GX API and register definitions, checked
   against the local Dolphin VideoCommon reference (TEV lerp/rounding,
   lighting, cull winding, konst fractions). This is an inspection renderer:
   mip levels with derivative LOD (no anisotropy or diagonal LOD), no fog,
   indirect textures, swap tables, the second channel, logic ops, EFB copies;
   fog calls must be type none. Exact original
   pixels are not claimed. */
typedef struct {unsigned width,height;uint32_t *bgra;uint32_t *depth;} nflstreet_gx_target;
/* Frame state the player draw does not set (set earlier by the scene). */
typedef struct {
 uint32_t projection_type;float projection[6]; /* GXSetProjection (shadow +420..+438) */
 float viewport[6];       /* GXSetViewport x, y, width, height, near, far (shadow +43C..+450) */
 uint32_t zmode;          /* BP 0x40 image: enable bit 0, compare bits 1-3, update bit 4 */
 uint32_t cmode0;         /* BP 0x41 image: color update bit 3 */
 uint32_t ksel[8];        /* BP 0xF6..0xFD images (konst selections per stage pair) */
 uint8_t konst[4][4];     /* konst colors K0..K3 RGBA */
 uint8_t registers[4][4]; /* TEV PREV, C0, C1, C2 RGBA at frame start */
 uint32_t texgen_src[8],texgen_mtx[8]; /* per coordinate: source (4 + n = TEXn) and matrix id */
} nflstreet_gx_frame;
typedef struct {unsigned display_lists,primitives,triangles,culled,clipped,textures;size_t pixels;} nflstreet_gx_stats;
typedef struct nflstreet_gx_machine nflstreet_gx_machine;
/* Memory comes from the same span as the draw (display lists, arrays,
   matrices, texture and TLUT objects and their data). */
nflstreet_gx_machine *nflstreet_gx_open(nflstreet_draw_span,void *memory,const nflstreet_gx_frame *,nflstreet_gx_target *);
/* Executes calls in order; returns 0 on an unsupported call or malformed
   display list (state up to that point is kept). */
int nflstreet_gx_run(nflstreet_gx_machine *,const nflstreet_gx_call *,unsigned count,nflstreet_gx_stats *);
void nflstreet_gx_close(nflstreet_gx_machine *);
/* Decode one GX texture (formats I4, I8, IA4, IA8, RGB565, RGB5A3, RGBA8,
   CI4, CI8, CMPR; TLUT formats IA8, RGB565, RGB5A3) to RGBA bytes. */
int nflstreet_gx_decode(const uint8_t *image,size_t size,unsigned width,unsigned height,unsigned format,
 const uint8_t *tlut,unsigned entries,unsigned tlut_format,uint8_t *rgba);
#endif
