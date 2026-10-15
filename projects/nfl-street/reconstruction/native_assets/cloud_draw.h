#ifndef NFLSTREET_CLOUD_DRAW_H
#define NFLSTREET_CLOUD_DRAW_H
/* The field camera scene node's entry 1, 800E398C (object 80DE8E1C), and
   the clouds it draws. When the byte r13-71E3 is set it calls 80187CB4(0)
   (*(r13-6608), passed to the sky, which ignores it); when the byte
   r13-71E4 is also set: z off without update (GXSetZMode(0, LEQUAL, 0)),
   the current projection (GXGetProjectionv, the GX shadow at +420..+438)
   with its words 2, 4 and 5 replaced by 802536CC (GXSetProjectionv), the
   sky (801655D8, scenery_draw.c) and the clouds (800D5B3C), then the
   projection restored and z LEQUAL with update.

   800D5B3C: nothing without the texture object at 802AA7A4; else that
   texture on map 0 and 8015F83C: render-state reset (801C7648), the sky
   color record (8015F164 into 8030FEB4 from the period index 802A3088, its
   blend 802A308C and the overlay amount 802A3068 over the table 8028CBC0,
   then 8015F3F0's two colors at +20/+30), the viewport stack top pushed,
   turned a quarter about x (80189854(0x400000)), loaded as position matrix
   0 and popped; z off, direct positions (F32 xyz) and colors (RGBA8),
   indexed texture coordinates (8-bit, 8028CC80 stride 8), one lit-off
   channel, one modulate stage, source-alpha blending; then 8015F450 for
   each of the r13-7284 (u16) records listed at 80385C8C, the blend off,
   the unrotated top reloaded and the NULL shader (801C7A3C(0)).

   8015F450(record {layers u8 +00, size +08, layer height +0C, fade +10,
   alpha +14, x +18, z +1C, top y +20}): one quad strip of (layers + 1)
   quads, each a square of side size at x/z, descending from the top y by
   height / (layers - 1), colored from the two sky colors and alpha scaled
   by the layer count. Field names are descriptive.

   GXSetProjectionv is not in player_draw.h's op table; it is emitted as
   NFLSTREET_GX_SET_PROJECTION_V (the next op value) with its seven-word
   argument array (type as a float, then the six parameters) as data. */
#include "scenery_draw.h"
#include "matrix_stack.h"
#define NFLSTREET_GX_SET_PROJECTION_V ((nflstreet_gx_op)NFLSTREET_GX_OPS) /* 80207D5C; data = the 7 words of GXSetProjectionv */
typedef struct {
 const nflstreet_scenery_context *scene;  /* draw services and the sky (801655D8) */
 const nflstreet_matrix_stacks *stacks;   /* the viewport's matrix stack (selected, r13-65E8) */
 uint32_t enable;        /* r13-71E3: byte, the node entry draws only when set */
 uint32_t sky_enable;    /* r13-71E4: byte, the sky and clouds draw only when set */
 uint32_t gx_state;      /* value of r13-4C90: GX shadow, projection type +420 and parameters +424..+438 */
 uint32_t texture;       /* 802AA7A4: word, the cloud texture object (0: no clouds) */
 uint32_t records;       /* 80385C8C: cloud record pointers */
 uint32_t count;         /* r13-7284: u16 record count */
 uint32_t texcoords;     /* 8028CC80: the four corner texture coordinates (F32 s, t) */
 uint32_t period;        /* 802A3088: word, period index (8002771x getters); +4 its blend (float) */
 uint32_t overlay;       /* 802A3068: float overlay amount (80027660) */
 uint32_t palette;       /* 8028CBC0: four 0x20-byte color records, then overlay records +80 and +A0 */
 uint32_t sky_record;    /* 8030FEB4: the computed record; its colors at +20 and +30 (8015F3F0) */
 float projection_value; /* 802536CC: the projection words 2, 4 and 5 meanwhile */
 float blend_cap,blend_half,blend_floor; /* 802606D8, 802606DC, 802606E0 */
 float color_alpha;      /* 802606E4: the second color's fourth word */
 float one,zero,half,byte_scale; /* 802606F8, 802606FC, 80260700, 80260704 */
} nflstreet_cloud_context;
/* 800D5B3C: the clouds. The render state carries over like the original's. */
int nflstreet_clouds_draw(const nflstreet_cloud_context *,nflstreet_render_state *);
/* 800E398C: the node entry (projection changes, sky, clouds, restore). */
int nflstreet_sky_node_draw(const nflstreet_cloud_context *,nflstreet_render_state *);
#endif
