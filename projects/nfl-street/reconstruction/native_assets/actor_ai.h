#ifndef NFLSTREET_ACTOR_AI_H
#define NFLSTREET_ACTOR_AI_H
/* GNNE69 r0 assignment-follow input leaves. Candidate until the original-
   module gate passes. This is NOT the 8008D988 state callback: its resolver,
   eligibility, facing, interaction and transition calls remain unmodeled. */
#include <stdint.h>
typedef struct {
 uint8_t *(*span)(void *,uint32_t,uint32_t);
 void *memory;
 uint32_t r13;
} nflstreet_actor_ai;
/* 8008D5DC: signed bytes at *(actor+310)+1/+2 -> two floats at output.
   8008D954: clears actor+150 and decodes into actor+154/+158. Call on
   entry to this assignment state, NOT unconditionally every update. */
int nflstreet_actor_ai_decode(nflstreet_actor_ai *,uint32_t actor,uint32_t output);
int nflstreet_actor_ai_enter(nflstreet_actor_ai *,uint32_t actor);
/* 8008D764: offset is TWO floats (normally actor+154). Other is an
   explicitly supplied gameplay actor; output is EIGHT bytes. Uses the
   field half-width from *(*(r13-7078)+90). No target actor selection. */
int nflstreet_actor_ai_target(nflstreet_actor_ai *,uint32_t actor,uint32_t other,uint32_t offset,uint32_t output);
/* 8008D864: target is TWO floats; writes only +200/+204/+208/+20E and,
   above the threshold, +20F (below threshold the byte is PRESERVED).
   Does not clear/commit inputs, execute physics, or change position. */
int nflstreet_actor_ai_project(nflstreet_actor_ai *,uint32_t actor,uint32_t target);
/* Finite, non-overflowing arithmetic and valid nonaliasing input/output
   spans are the supported boundary. Returns 0 for unsupported spans or
   nonfinite intermediates, 1 for success. Do not continue a frame on 0.
   Bad spans are host errors; the original has no such error interface. */
#endif
