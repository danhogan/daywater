#ifndef NFLSTREET_BALL_OWNERSHIP_H
#define NFLSTREET_BALL_OWNERSHIP_H
#include "ball_orientation.h"
/* Caller-owned attachment inputs; no RAM addresses or ownership policy. */
typedef struct {
 const float *world,*model_root;float model_scale;
 const float (*bones)[12];size_t count;int selector;
 const float *local;const int32_t *rotation;
} nflstreet_ball_attachment_inputs;
/* Original query/cache and optional 8014629C side-effect words.
   Opaque context numbers do not imply named match modes. */
typedef struct {uint32_t value,cached_value,actor_word;} nflstreet_ball_assignment_context;
/* Bounded complete 800BE2B4. All outputs remain unchanged on rejection.
   Actor header and flag word are supplied native identity/state values. */
int nflstreet_ball_assign(uint8_t *record,size_t bytes,uint32_t actor_header,
 uint32_t *actor_flags,nflstreet_ball_assignment_context *,uint32_t elapsed,
 const nflstreet_ball_attachment_inputs *,nflstreet_ball_pose *model_pose);
#endif
