#ifndef NFLSTREET_CAMERA_PROJECTION_H
#define NFLSTREET_CAMERA_PROJECTION_H
#include "camera_slots.h"
/* Rebuild slots 1 and 2 from one camera node. viewport_scale is the value
   computed by 801DFAB0 from the active viewport stack (f5 to 801DF8C4). */
int nflstreet_camera_projection(const nflstreet_view_context *,uint32_t node,float viewport_scale);
#endif
