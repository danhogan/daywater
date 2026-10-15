#ifndef NFLSTREET_BALL_UPDATE_H
#define NFLSTREET_BALL_UPDATE_H
#include "matrix_stack.h"
/* GNNE69 r0, 800C0FE4 on a caller-owned big-endian MEM1 span. r13 is an
   explicit input. No controller policy, assignment, collision or state
   dispatcher. Detached records currently require the existing ball_flight
   no-spin/no-wind branch; rejection is reported, never silently simulated.
   A rejected call may have written matrix scratch; discard the update on
   failure. Valid original-layout RAM and nonaliasing object spans required. */
typedef struct {
 const nflstreet_player_draw_context *memory;
 nflstreet_matrix_stacks stacks;
 uint32_t r13;
} nflstreet_ball_update_context;
int nflstreet_ball_update(const nflstreet_ball_update_context *,uint32_t record,double delta);
/* 800C09D8 publishes record pose to visual +04/+6C, including side flip.
   Call after physics. This is distinct from the attached model pose written
   by 800C0ECC during physics (the original has both). */
int nflstreet_ball_update_visual(const nflstreet_ball_update_context *,uint32_t record);
/* Host frame connection: 801A3008 rotation, then position into +20 matrix's
   translation column. Preserves bottom row. Not the complete scene-node
   transform traversal; provided explicitly for scene_frame/live integration. */
int nflstreet_ball_update_matrix(const nflstreet_ball_update_context *,uint32_t visual);
#endif
