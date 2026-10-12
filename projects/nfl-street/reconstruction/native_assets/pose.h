#ifndef NFLSTREET_POSE_H
#define NFLSTREET_POSE_H
#include <stddef.h>
#include <stdint.h>
/* Original direct compressed bank, unrelocated file offsets. Output remains
   in compressed stream order, as in 0x8015D104 with scatter disabled. */
int nflstreet_pose_frame(const uint8_t *,size_t,unsigned,int16_t *,size_t,unsigned *);
/* Original 0x80166730 wrapped-short interpolation, weight 0..4096. */
int nflstreet_pose_mix(int16_t *,const int16_t *,const int16_t *,size_t,unsigned);
/* Original TMdl stored skeleton plus optional model-ordered rotations.
   Clst weights interpolate angles before concatenation, not skin matrices. */
int nflstreet_pose_clusters(const uint8_t *,size_t,const int16_t *,size_t,float [256][12],unsigned *);
#endif
