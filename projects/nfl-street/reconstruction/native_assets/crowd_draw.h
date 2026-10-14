#ifndef NFLSTREET_CROWD_DRAW_H
#define NFLSTREET_CROWD_DRAW_H
/* The venue's crowd and street props as the in-match frame draws them: the
   scene node's 800D2F20 entries (EARS_SPECTATOR_A..K, EARS_GARBAGECAN and
   the static props), each calling its +14 hook. Hook 80168234 draws a
   spectator or prop model and its chain of copies (80168E08): lights,
   channel, alpha/z/cull/blend presets, model and model-view matrices from
   the viewport's stack (translation +04, quaternion +18 through 801897B8,
   x quarter turn, offset +B4, heading +CC), the frustum test of the box at
   8028E2B4 (801EC298), the spectator hide test (8016870C/801687FC), then by
   path: CPU skinning (801EAC78 palette, 801CABC4 scale and DMA, 801C83A0/
   801C7FB4 skinned positions and normals into the vertex scratch ring
   801C6344) and the chunk list; GPU skinning (801EAC78 on the model-view,
   801EC140); or a rigid draw (801EC140). Hook 800D2860 returns 0.

   Not modeled (the draw returns 0 where the original would run them): the
   distance fade of objects with +13C set (80005854/80007D90/801DDFFC), the
   override allocation 801C6BF4 of a model view without overrides (+1C),
   any other +14 hook, and CPU skinning without an override array. */
#include "scenery_draw.h"
#include "actor_palettes.h"
typedef struct {
 const nflstreet_scenery_context *scene;  /* draw services, cull copies (80348AD4/80348B14), projection slot, play gates, mode word */
 nflstreet_actor_palette_context *palettes; /* viewport stacks, concat lanes, palette pool and locked cache (shared with the players) */
 uint32_t enable;        /* r13-72A4: byte, 800D2F20 draws only when set */
 uint32_t view_slot;     /* 80331F80: matrix-library slot 3 (8018A214(3)) */
 uint32_t cull_box;      /* 8028E2B4: the frustum-test box */
 uint32_t game_mode;     /* 802A301C (8002777C): the hide test needs 4 or 7 */
 uint32_t sides;         /* value of r13-5BA4 (800C7160): its +08 entry's +08 is the reference y */
 uint32_t hide_limit;    /* 802A3050 (800275D8): float */
 uint32_t scratch;       /* r13-6430: vertex scratch ring {start -6430, current -642C, end -6428} */
 float hide_sign[2];     /* 80260F2C (80168234), 80260F30 (80168E08) */
 float light_default[2]; /* 80260F04, 80260F54: light scale without a +144/+58 source */
 float palette_scale[2]; /* 80260F1C, 80260F58: skinning palette scale (801CABC4) */
 float normal_scale;     /* 8026381C: skinned normals before quantizing */
 float quat_one;         /* 802634D8: 801A3008's 1.0 */
} nflstreet_crowd_context;
/* 800D2F20(object): nothing unless the enable byte is set; then the
   object's +14 hook. The render state carries over like the original's. */
int nflstreet_crowd_draw(const nflstreet_crowd_context *,uint32_t object,nflstreet_render_state *);
#endif
