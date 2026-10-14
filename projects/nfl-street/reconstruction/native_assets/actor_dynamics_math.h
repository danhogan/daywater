#ifndef NFLSTREET_ACTOR_DYNAMICS_MATH_H
#define NFLSTREET_ACTOR_DYNAMICS_MATH_H
/* Math and matrix-stack leaves used by 8004BF1C/8004C268.  Addresses below
   are executable-data inputs, not host constants: the functions read them
   through the saved-RAM draw span. */
#include "matrix_stack.h"
uint32_t nflstreet_dynamics_atan(nflstreet_draw *,float y,float x); /* 80188D98 */
float nflstreet_dynamics_length(const float[3]);                    /* 801DDFFC */
void nflstreet_dynamics_sub(float[3],const float[3],const float[3]);/* 801DE5DC */
float nflstreet_dynamics_dot(const float[3],const float[3]);        /* 801DE62C */
int nflstreet_dynamics_normalize(nflstreet_draw *,float[3],const float[3]); /* 801DE118 */
int nflstreet_dynamics_angles(nflstreet_draw *,const nflstreet_matrix_stacks *,uint32_t out); /* 80189F54 */
int nflstreet_dynamics_top_translation(nflstreet_draw *,const nflstreet_matrix_stacks *,float[3]); /* 80189F10 */
int nflstreet_dynamics_rotate(nflstreet_draw *,const nflstreet_matrix_stacks *,float[3],const float[3]); /* 801DEB54 */
int nflstreet_dynamics_clamp(nflstreet_draw *,uint32_t limits,int32_t offset,uint32_t value); /* 8004BED8 */
#endif
