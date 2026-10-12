#ifndef NFLSTREET_BALL_FRAME_H
#define NFLSTREET_BALL_FRAME_H
#include "ball_ownership.h"
/* Owned 800C0FE4 frame record effects. Association resolution is supplied:
   an associated record requires its resolved attachment inputs; detached
   records use the bounded airborne service. Retains final 801087B8 spatial
   query without pretending to reconstruct the environment spatial index. */
typedef struct {
 nflstreet_ball_pose visual_pose;
 float spatial_query_position[3];
 unsigned spatial_query;
} nflstreet_ball_frame_output;
int nflstreet_ball_frame(uint8_t *,size_t,double delta,double target_plane,
 const nflstreet_ball_attachment_inputs *resolved_actor,
 const nflstreet_ball_pose *previous_visual,nflstreet_ball_frame_output *);
#endif
