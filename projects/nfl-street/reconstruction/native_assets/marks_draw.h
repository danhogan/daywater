#ifndef NFLSTREET_MARKS_DRAW_H
#define NFLSTREET_MARKS_DRAW_H
/* Native ground marks (800D75EC): three mark lists, each drawn between its
   type's begin and end handlers (table 8028CCB4, 0x20 bytes per type).
   List 2 (type 2, 80160104/80160298/80160250) starts at the oldest mark
   and ramps the alpha +34 by 0.1 every eight marks; lists 1 and 0 (types 1
   and 0, begin 8015FD28, end a bare return) draw in order. A mark is 0x74
   bytes: position +00, scale +0C, z angle +30, alpha +34, texture record
   +60 (+28/+2C), type +68, active +70. Type 0/1 marks (8015FD88) are not
   modeled and are rejected when active. */
#include "scenery_draw.h"
#include "matrix_stack.h"
typedef struct {
 const nflstreet_scenery_context *scene; /* draw callbacks, fog */
 nflstreet_matrix_stacks stacks;
 uint32_t enabled;       /* r13-7280: byte */
 uint32_t newest;        /* r13-5B60: list 2 start */
 uint32_t lists;         /* 802AA7C4: three list pointers */
 uint32_t counts;        /* 80271268: three counts */
 uint32_t handlers;      /* 8028CCB4 */
 uint32_t fog_saved;     /* r13-58F0: byte */
 float ramp_step,ramp_cap,alpha_scale; /* 802529C8, 802529CC, 8026072C */
 float depth[3];         /* 80260720 (begin near/far), 80260724/80260728 (end) */
 float quad[6];          /* 80260730..80260744: corner coordinates */
 uint32_t type_begin[2],type2[3]; /* 8015FD28, 8015FD84; 80160104, 80160298, 80160250 */
} nflstreet_marks_context;
int nflstreet_marks_draw(const nflstreet_marks_context *,nflstreet_render_state *);
#endif
