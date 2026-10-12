#ifndef NFLSTREET_COMPACT_POSE_H
#define NFLSTREET_COMPACT_POSE_H
#include <stddef.h>
#include <stdint.h>
/* Original 801EAE1C compact SKL1 evaluator, with owned native root matrix.
   Rotations are signed 16-bit angles in compact bone order. This is a distinct
   palette from TMdl weighted render clusters. Invalid inputs reject atomically. */
int nflstreet_compact_pose(const uint8_t *,size_t,const int16_t *,size_t,
                          const float root[12],float [256][12],unsigned *);
#endif
