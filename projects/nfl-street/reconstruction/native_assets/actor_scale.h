#ifndef NFLSTREET_ACTOR_SCALE_H
#define NFLSTREET_ACTOR_SCALE_H
/* 800DFBA8 / 800DFB34 on saved RAM. Disassembly-derived; original-module
   execution is pending. r13 is the game's small-data base (803A9A40 here).
   No actor writes: +1361 selects a slot in the global object at r13-7234.
   Returns 0 on an inaccessible span; this is a host validation choice. */
#include "draw_engine.h"
/* 800DFBA8: if byte r13-7238 != 0, write 1 to global+4E3+slot. */
int nflstreet_actor_scale_mark(nflstreet_draw *,uint32_t r13,uint32_t actor);
/* 800DFB34: read global+49C+4*slot, or float at 80252D24 when disabled,
   and store z, x, y. out must name three host float objects. No arithmetic
   occurs in the original; the single-precision word is preserved. */
int nflstreet_actor_scale_vector(nflstreet_draw *,uint32_t r13,uint32_t actor,float out[3]);
#endif
