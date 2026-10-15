#ifndef NFLSTREET_FIELD_OVERLAY_H
#define NFLSTREET_FIELD_OVERLAY_H
/* Field-camera traversal entry 14, 800E5CA0 -> 800D1118(0). RAM is the
   original big-endian layout. No scissor call occurs in this entry.
   The depth service supplies GXPeekZ's EFB result; it must return success.
   Projectionv uses cloud_draw.h's existing extension. The two additional
   operations below have deliberately local values, outside the shared table. */
#include "cloud_draw.h"
#define NFLSTREET_FIELD_GX_SET_PROJECTION ((nflstreet_gx_op)(NFLSTREET_GX_OPS+1)) /* arg type, data 16 matrix words */
#define NFLSTREET_FIELD_GX_PEEK_Z ((nflstreet_gx_op)(NFLSTREET_GX_OPS+2)) /* args x,y; data returned depth */
typedef int (*nflstreet_field_depth)(void *,uint32_t x,uint32_t y,uint32_t *depth);
typedef struct {
 const nflstreet_scenery_context *scene;
 const nflstreet_matrix_stacks *stacks;
 uint32_t r13; /* small data base, 803A9A40 in GNNE69 r0 */
 nflstreet_field_depth depth;
 void *depth_user;
} nflstreet_field_overlay_context;
/* Returns 0 for unavailable spans/services or unsupported random generator
   kinds (only the saved stream kind 0, 80188008, is reconstructed). */
int nflstreet_field_overlay_draw(const nflstreet_field_overlay_context *,nflstreet_render_state *);
#endif
