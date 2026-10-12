#ifndef NFLSTREET_MENU_TEXT_H
#define NFLSTREET_MENU_TEXT_H
#include "font.h"
/* Bounded 801D8E54 unwrapped single-line path, including inline line record.
   Object is 40 original-format bytes. Pointer fields remain caller-owned.
   Wrapped/multiline allocation and named palette banks are unsupported. */
int nflstreet_ui_text_prepare(uint8_t object[40],const nflstreet_font*,const char*);
typedef struct {
 float position[3],color[2][4],shadow_offset[3];
 unsigned flags,visible;int archive,entry;
} nflstreet_ui_text_plan;
/* Bounded 801D97D4 draw arguments. Context is four multipliers followed by
   four offsets. Supports a prepared unwrapped line with literal colors and
   flags containing 0x10 or 0x40. Fixed height, dirty and named-palette paths
   reject atomically. Produces local coordinates, not a viewport/projection. */
int nflstreet_ui_text_draw(const uint8_t object[40],const char*,const float context[8],nflstreet_ui_text_plan*);
/* Find a direct class8 text link in a relocated node; return object40 bytes
   and a bounded borrowed source string. Nested nodes remain unsupported. */
int nflstreet_ui_text_find(const uint8_t*,size_t,uint32_t base,unsigned node,unsigned ordinal,uint8_t object[40],const char**text);
#endif
