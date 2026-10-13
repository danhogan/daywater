#ifndef NFLSTREET_HUD_DRAW_H
#define NFLSTREET_HUD_DRAW_H
/* Native in-match HUD draws: the original text renderer (801863A4 layout,
   80186B1C/80186924/8018675C quads) and the player name tag (800DADC0).
   Like the scenery draws they produce the original GX calls and pipe data
   from saved RAM tokens, with the original RAM side effects. */
#include "scenery_draw.h"
#include "matrix_stack.h"
typedef struct {
 const nflstreet_scenery_context *scene; /* draw callbacks, fog, play-state gates */
 uint32_t tag_enable;      /* r13-7240: byte, name tags on */
 uint32_t tag_black;       /* r13-723F: byte, black name text */
 uint32_t option_table;    /* 8030B2AC: option words 16 bytes apart (8014E29C) */
 uint32_t option_override; /* r13-6334: byte; with r13-6FF0 == 8 the mode is re-read (not modeled) */
 uint32_t option_mode;     /* r13-6FF0 */
 uint32_t profile_base;    /* value of r13-6608: profile buckets 0x7C apart (80187CB4) */
 uint32_t profile_current; /* r13-6618 */
 uint32_t profile_next;    /* r13-6610 */
 uint32_t text_state;      /* 8039C480: flags, colors +04/+14, shadow offset +24 */
 uint32_t text_boxes;      /* 8039C4BC: glyph boxes, 0x14 bytes each */
 uint32_t text_ready;      /* r13-6650: byte, the text system is up */
 uint32_t error_hook;      /* value of r13-64B8: must be 0 (error callbacks not modeled) */
 uint32_t error_code;      /* r13-64BC: last error code (801AF57C) */
 nflstreet_matrix_stacks stacks;
 uint32_t camera_types;    /* value of r13-5780: camera handlers 12 bytes apart (8017D16C) */
 uint32_t screen_camera;   /* 8017DB70: the handler whose view 8017D604 is modeled */
 uint32_t field_camera;    /* 800C3508: by data +98, modes 0 (8017DB70), 1 (8017D938) and 5 reach 8017D604 */
 uint32_t tluts[2];        /* 80331460, 80331480: the two font palettes */
 float tag_offset;         /* 80252B68: name tag x offset */
} nflstreet_hud_context;
/* Original 80186B1C(font, text, flags, limit, position): lay out the text
   (flags 1 center, 2 right, 4 no kerning; positive limit wraps) and draw
   its glyph quads at the matrix-stack top translated to position. */
int nflstreet_text_draw(const nflstreet_hud_context *,uint32_t font,uint32_t text,uint32_t flags,float limit,uint32_t position,
 nflstreet_render_state *);
/* Original 8017D16C for a camera node (+40 its data) of the screen type,
   or of the field type in modes 0, 1 and 5 (other modes leave the stack):
   the viewport's stack reset to identity, rotated about x, y and z by the
   24-bit angles at data +14/+18/+1C and translated by minus the position
   +04/+08/+0C (8017D604). Other camera types are rejected. */
int nflstreet_camera_view(const nflstreet_hud_context *,uint32_t camera,nflstreet_render_state *);
/* Original 800DADC0 for one name tag (0x24 bytes: screen position +04,
   font +14, text +18, shown +1C, depth +20). */
int nflstreet_name_tag_draw(const nflstreet_hud_context *,uint32_t tag,nflstreet_render_state *);
#endif
