#ifndef NFLSTREET_VIEW_FRUSTUM_H
#define NFLSTREET_VIEW_FRUSTUM_H
#include "object_pool.h"
typedef struct {nflstreet_pool_ram ram;uint32_t r13,angle_constants,fov_scale;} nflstreet_view_frustum_context;
/* 80188C80 + reached original tangent kernel/first reduction. Angle is signed
   guest bits; supports finite converted radians with absbits<=4016CBE3.
   Larger-angle reduction rejects as host scope, not original validation.
   Context constants must be original/stable; result is original binary32. */
int nflstreet_view_tangent(const nflstreet_view_frustum_context *,uint32_t angle,float *result);
/* 801DF60C. Finite binary32 input contract; exact single/FMA/integer angle order.
   Original is void; return is host status. No stack/scene/global publication. */
int nflstreet_view_frustum_set(const nflstreet_view_frustum_context *,uint32_t node,float fov,float aspect,float near_z,float far_z);
/* 801DF6B4. Uses tangent(100000), two single divisions and scale product. */
int nflstreet_view_scale_set(const nflstreet_view_frustum_context *,uint32_t node,float scale);
/* 801DF5F8: only byte45 and word48. */
int nflstreet_view_binding_set(nflstreet_pool_ram,uint32_t node,uint32_t identity,uint8_t flag);
#endif
