#ifndef NFLSTREET_ACTOR_SLERP_H
#define NFLSTREET_ACTOR_SLERP_H

#include "../actor_palettes.h"

/* 801EAD98(model, descriptor, compact_pose, -, enables), as called by
   80162760 for flag 0x4000.  The viewport stack must already contain the
   actor's body transform.  The enable table is 8028DD70 in GNNE69 r0.
   Returns zero on an invalid native memory span; no rollback is promised. */
int nflstreet_801ead98_slerp_body(nflstreet_actor_palette_context *context,
    nflstreet_draw *draw, uint32_t model, uint32_t descriptor,
    uint32_t compact_pose, uint32_t enables);

#endif
