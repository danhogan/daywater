#ifndef NFLSTREET_PALETTE_POOL_H
#define NFLSTREET_PALETTE_POOL_H
#include "object_pool.h"
typedef struct {
 void *user;
 uint32_t (*heap)(void *,uint32_t owner,uint32_t size,uint32_t flags);
} nflstreet_palette_pool_services;
typedef struct {
 nflstreet_pool_ram ram;
 uint32_t globals; /* source r13-5680: roots[2], active, capacity, cursor */
 const nflstreet_palette_pool_services *services;
} nflstreet_palette_pool_context;
/* 801CA3F4, through original heap wrapper8018B7B8. Host status only: original
   is void and publishes even a zero heap response. Multiplier2 alone selects
   a distinct second root; all other multipliers publish identical roots.
   Requires nonwrapping capacity*multiplier*48. Does not touch payload bytes. */
int nflstreet_palette_pool_create(const nflstreet_palette_pool_context *,uint32_t capacity,uint32_t multiplier);
/* 801CA494. Zero cursor, active=(old_active==0), without clearing payload. */
int nflstreet_palette_pool_flip(const nflstreet_palette_pool_context *);
/* 801CA4B0. Raw wrapping round-up/cursor/data arithmetic; no capacity guard.
   Writes palette+8 rounded count, then palette+0 data, preserving palette+4.
   Active0/1 and bounded aligned accessed words are host preconditions. */
int nflstreet_palette_pool_reserve(const nflstreet_palette_pool_context *,uint32_t palette,uint32_t requested);
/* Host convenience only, no original counterpart: bounds-check descriptor's
   data/count*48 against RAM. Does not establish pool ownership or capacity.
   NULL on rejection, clears size. Caller retains RAM/services and lifetimes. */
uint8_t *nflstreet_palette_pool_span(const nflstreet_palette_pool_context *,uint32_t palette,size_t *size);
#endif
