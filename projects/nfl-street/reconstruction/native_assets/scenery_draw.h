#ifndef NFLSTREET_SCENERY_DRAW_H
#define NFLSTREET_SCENERY_DRAW_H
#include "player_draw.h"
/* Scenery instance (0xBC bytes, original layout): flags +00, group variant
   +04, model-view 3x4 +08, depth value +48, ambient RGBA +4C, model +54,
   materials {count, pointers} +5C, model view +64 (its group table at +78),
   bounding box {min xyz, -, max xyz, -} +98, shader override +B8.
   The draw services (span, GX sink, NULL shader, default texture, shader
   setups) are those of the player context; the rest are original globals. */
typedef struct {
 const nflstreet_player_draw_context *draw;
 uint32_t enable;            /* value of r13-6230: scenery drawing enabled */
 uint32_t projection;        /* matrix library slot 2 (80331F40): culling projection */
 uint32_t cull_projection;   /* 80348AD4: copy written by 801CB3EC */
 uint32_t cull_modelview;    /* 80348B14: copy written by 801CB430 */
 uint32_t fog;               /* r13-6208: fog enable byte (read and written) */
 uint32_t fog_camera;        /* value of r13-6218: camera with near/far at +E8/+EC */
 uint32_t fog_type,fog_color;/* values of r13-562C and r13-620C */
 float fog_start,fog_end;    /* r13-6214, r13-6210 */
 float fog_off;              /* 80268E28: the fog-off call's four floats */
 float depth_trim;           /* r13-622C: far-depth trim of flag 0x40000 */
 uint32_t pass_table;        /* 8028A004: field pass records, 0x1C each */
 uint32_t pass_offset;       /* 8028A090: field pass 2 translation (alpha at +0C) */
 float pass_alpha,pass3_alpha; /* 80260588 and r13-58F4 */
 float pass_color,pass_depth;  /* 80260580 (RGB scale) and 80260584 (depth threshold) */
} nflstreet_scenery_context;
/* Original 801EC8E4(instance, -, lights): frustum test of the bounding box
   against the matrix-library projection when flags bit 14 (801CB3EC/
   801CB430/801CB708: corners 801CAE78, clip codes 801CB05C), shader override,
   material color, position/normal matrix 0, blend preset, z compare
   location and alpha compare, depth mode and range, fog off unless bit 0
   (801EDDF8/801EDD74), channel controls, then the model's chunks (801C84D4)
   or group 0 variant +04 (801C8544). The render state carries over between
   calls like the original's. */
int nflstreet_scenery_draw(const nflstreet_scenery_context *,uint32_t instance,uint32_t lights,nflstreet_render_state *);
/* Original 80165DC4 for the venue's main field object: instances +258,
   +E0, +19C, +24, +314 and ten from +3D0 (0xBC apart), each 801EC8C0. The
   per-frame model-view updates (matrix stack 801893C8/8018941C/80189BB0/
   8018949C and 801EC89C) are not modeled: instances keep their stored
   matrices, as player draws keep their stored skinning palettes. Weather
   draws 8015A5CC/8015A7C4 are not modeled. */
int nflstreet_field_draw(const nflstreet_scenery_context *,uint32_t object,nflstreet_render_state *);
/* Original 8015B670/8015B564 for the field-pass object: layers 0-4
   (instance object + 0x2C + 0xBC*layer) when the object's +3D8 entry and the
   pass record's enable byte are set; each gets ambient (scale, scale, scale,
   alpha * record alpha) through 801ECD98 and, when the record depth exceeds
   the threshold, depth mode LESS with that range through 801ECD54. Matrix
   updates as above; 800D75EC (between layers 3 and 4) and 801652BC (after)
   are not modeled. */
int nflstreet_field_passes_draw(const nflstreet_scenery_context *,uint32_t object,nflstreet_render_state *);
#endif
