#ifndef NFLSTREET_SCENERY_SHARED_H
#define NFLSTREET_SCENERY_SHARED_H
/* Scene helpers shared by the scenery and HUD draws (internal). */
#include "scenery_draw.h"
#include "draw_engine.h"
/* 801EDDF8: set the fog byte (applying 801EDD74 on a change); *previous
   receives the value that restores the earlier state. */
int nflstreet_scenery_fog_switch(nflstreet_draw *,const nflstreet_scenery_context *,uint32_t value,uint32_t *previous);
/* 8000C2D8 on r13-7F40: play is held (flags +B64 & 0xC00880, or +B60 == 4). */
int nflstreet_play_held(nflstreet_draw *,const nflstreet_scenery_context *,int *held);
/* 8002E4F8: the mode word at *r13-7CF8, also stored to r13-7CF4. */
int nflstreet_play_mode(nflstreet_draw *,const nflstreet_scenery_context *,uint32_t *mode);
/* 8012D844: the count at 80308560 is positive. */
int nflstreet_play_counting(nflstreet_draw *,const nflstreet_scenery_context *,int *counting);
/* 801CAE78: the eight box corners through a 3x4 matrix. */
void nflstreet_box_corners(const float box[8],const float m[12],float out[24]);
/* 801CB05C: clip codes, (or << 8) | and. */
uint32_t nflstreet_clip_codes(const float *points,unsigned n,const float P[16]);
/* 801CB1C4 (no depth output): clip codes and the projected bounds. */
uint32_t nflstreet_clip_bounds(const float *points,unsigned n,const float P[16],float lo[3],float hi[3]);
#endif
