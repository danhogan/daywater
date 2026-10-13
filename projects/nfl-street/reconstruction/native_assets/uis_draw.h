#ifndef NFLSTREET_UIS_DRAW_H
#define NFLSTREET_UIS_DRAW_H
/* Native draw of a uis screen (801D0BC4 with no update ticks): the node
   tree walk 801CEFDC with the screen tint, node transforms and color stack
   (801D8398 → 801D8268/801D8364, 801D7E74), and the draw messages of the
   image (801D60DC → 801D6690/801D6A54) and text (801D9244 → 801D97D4)
   classes. The two game classes 8012CB94 and 8012C5D8 do nothing on a draw
   message. Paths the saved screens do not use (resource loads and
   acquisitions, dirty fonts and layouts, nine-slice strips, display-list
   and indirect images, other classes and callbacks) are rejected. */
#include "hud_draw.h"
#include "gx_texobj.h"
typedef struct {
 const nflstreet_hud_context *hud; /* text renderer, matrix stacks, error code */
 uint32_t manager;          /* value of r13-63DC (801D5C24(1)): archives 0x24 bytes apart at +0C */
 uint32_t tint;             /* 8034CDB8: screen tint scale RGBA, then add RGBA (801D3C90/801D3CAC) */
 float tint_start[2];       /* 8026396C (scale), 80263970 (add) */
 uint32_t color_depth;      /* r13-5648 */
 uint32_t color_current;    /* r13-63C8: the current color-stack entry (0x28 bytes) */
 float color_limits[3];     /* 80263B70 (1), 80263B74 (0), 80263B78 (255) */
 float degrees;             /* 80263B80: degrees to 24-bit angle */
 float zero;                /* 80263B7C: the "no rotation" value */
 uint32_t text_mode;        /* r13-63C4 (801D7DAC) */
 uint32_t draw_mode;        /* r13-63C0 (801D7DCC) */
 uint32_t text_mode_on;     /* r13-63BC: byte (801D7DB4) */
 uint32_t channel_cache;    /* r13-565C */
 uint32_t color_cache[2];   /* r13-5664 (material), r13-5660 (light) */
 uint32_t light;            /* 8039E700 */
 uint32_t texture;          /* 8039E740 */
 uint32_t tlut;             /* 8039E780 */
 uint32_t texture_matrix;   /* 80293DEC */
 uint32_t palette_cache[4]; /* r13-564C (palette), -5650 (manager), -5658, -5654 (ids) */
 uint32_t mark;             /* r13-566C */
 uint32_t resources_up;     /* r13-6538: byte */
 uint32_t notify;           /* value of r13-63D8: must be 0 */
 uint32_t refresh;          /* r13-63A8: byte (801DD318) */
 float color_unit;          /* 80263BA8: 1/255 */
 float text_limit;          /* 80263BB8 */
 float lod;                 /* 80263B98 */
 nflstreet_gx_texobj_constants texobj;
 uint32_t image_class,text_class,quiet_classes[2]; /* 801D60DC, 801D9244, 8012CB94, 8012C5D8 */
 uint32_t transform;        /* 801D8398 */
 /* 8012AB5C */
 uint32_t camera;           /* r13-6CD8: the camera node the screens draw under */
 uint32_t screens[4];       /* r13-6C84, -6C94, -6C88, -6C8C */
 uint32_t screen_flags;     /* r13-6C98: bits 8, 1, 2, 4 enable them */
 uint32_t pending_on;       /* r13-6CF0: byte */
 uint32_t pending;          /* 8030834C: two pending loads, 0x10 apart (none modeled) */
 uint32_t refresh_hook;     /* 80381C40: refresh check when its +4 is refresh_target */
 uint32_t refresh_target;   /* 800044A4 */
 uint32_t after_second;     /* r13-4F80: cleared after the second screen (80161C88) */
} nflstreet_uis_context;
int nflstreet_uis_screen_draw(const nflstreet_uis_context *,uint32_t screen,nflstreet_render_state *);
/* Original 8012AB5C(camera, 0): the in-match screens under their camera
   node, between fog off and on and depth tests off and on. */
int nflstreet_hud_screens_draw(const nflstreet_uis_context *,uint32_t camera,nflstreet_render_state *);
#endif
