#ifndef NFLSTREET_ACTOR_SKELETON_H
#define NFLSTREET_ACTOR_SKELETON_H
/* A visual actor's compact bone matrices on saved RAM, as 8015F08C builds
   them each update (from 800D51B4, at the end of the actor's animation
   update 80020FDC): push identity, translate by +34 and turn about y by +4C
   (unless flag 0x2000), then 801EAE1C evaluates the compact SKL1 skeleton
   (+64) from the pose (+5C, signed 16-bit x/y/z angles per compact bone) in
   the locked cache (0xE0000000, 0x40 per bone, 0x30 bytes written) and DMAs
   whole 0x40 slots to the bone matrices (+60). The arithmetic is the
   verified compact_pose.c kernel. Uses the actor palette context for the
   memory span, matrix stacks, locked cache and its enable flag. */
#include "actor_palettes.h"
/* 8015F08C(visual). Returns 0 when the sine table at 8039E100 is not the
   original's or the skeleton is not an SKL1 (before any write), and, after
   the stack push but before any matrix or cache write, when it has 0 or
   more than 256 bones, a parent that is not an earlier bone, or a matrix
   that is not finite; the original would evaluate those. */
int nflstreet_actor_skeleton(nflstreet_actor_palette_context *,uint32_t visual);
/* 800D51B4 without its +35C countdown (801EE24C random draw, not modeled):
   select matrix stack 1 (801893C8), reset it (801893EC), then 8015F08C. */
int nflstreet_actor_skeleton_frame(nflstreet_actor_palette_context *,uint32_t visual);
#endif
