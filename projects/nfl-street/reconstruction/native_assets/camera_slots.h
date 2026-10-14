#ifndef NFLSTREET_CAMERA_SLOTS_H
#define NFLSTREET_CAMERA_SLOTS_H
/* The camera's derived matrix slots on saved RAM: the tail of 801DFAB0
   (the camera node's matrix setup, run once per camera node per frame),
   which turns the viewport stack's top (the camera's view, from 8017D16C)
   into the slot matrices the draws use. The slots are 4x4 matrices 0x40
   apart from 80331EC0:
     slot 1  the screen mapping (set earlier by 801DF8C4, not modeled)
     slot 2  the perspective projection (801DF938, not modeled)
     slot 3  the view          = identity x top (801895BC)
     slot 4  slot 2 x slot 3   (clip space; what the name tags project with)
     slot 5  slot 1 x slot 2 x slot 3
     slot 6  slot 1 x slot 2
   Slots 1 and 2 are read as they stand. */
#include "matrix_stack.h"
typedef struct {
 const nflstreet_player_draw_context *x; /* memory */
 nflstreet_matrix_stacks stacks;         /* the viewport's stack (venue_stacks) */
 uint32_t r13;                           /* 803A9A40 */
 uint32_t slots;                         /* 80331EC0 */
} nflstreet_view_context;
/* 801DFAB0 from the selection of the viewport stack (801DF594 + 801893C8)
   to its final pop: needs the camera view at the stack top. Net effects:
   slots 3 to 6, the stack top replaced by slot 3 and the stack's scratch
   slot above it. Returns 0 when RAM or the stack is unreachable. */
int nflstreet_camera_slots(const nflstreet_view_context *);
#endif
