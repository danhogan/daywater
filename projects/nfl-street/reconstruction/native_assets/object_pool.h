#ifndef NFLSTREET_OBJECT_POOL_H
#define NFLSTREET_OBJECT_POOL_H
#include <stddef.h>
#include <stdint.h>
/* GNNE69 r0 original pool kernel. All identities are guest addresses; bytes
   are BE. Host rejection is separate from the original's zero/-1 results.
   No original heap, registry, scene owner or actor construction is supplied. */
typedef struct { uint8_t *bytes; size_t size; uint32_t base; } nflstreet_pool_ram;
typedef struct {
 void *user;
 uint32_t (*heap)(void *,uint32_t owner,uint32_t size,uint32_t flags);
 void (*error)(void *,uint32_t code);
} nflstreet_pool_services;
/* Exact 80180328 arithmetic, including unsigned wrap/non-power-of-two masks. */
uint32_t nflstreet_pool_align(uint32_t size,uint32_t alignment);
/* Exact 80181040 arithmetic; outputs header/index offset, data offset, total. */
int nflstreet_pool_layout(uint32_t capacity,uint32_t stride,uint32_t alignment,
 uint32_t *indices,uint32_t *data,uint32_t *total);
/* Construct requires nonzero power-of-two alignment and nonwrapping layout.
   A supplied block bypasses heap. A heap zero is original failure (host success,
   result zero, no error callback). Services never own/free guest RAM here.
   A nonzero heap response must name a word-aligned valid block;
   bad callback responses are host rejection, after the observed heap request. */
int nflstreet_pool_construct(nflstreet_pool_ram,uint32_t owner,uint32_t capacity,
 uint32_t stride,uint32_t supplied,uint32_t alignment,
 const nflstreet_pool_services *,uint32_t *result);
/* Exact accessed ranges must fit the span; no capacity/fullness/index-value
   checks are attributed to the original. Push truncates to u8/u16 and increments
   count, even if it exceeds capacity. Index bytes must not overlap the header
   (host precondition). Alignment controls the data offset, not block identity.
   Callers own valid backing storage. */
int nflstreet_pool_reset(nflstreet_pool_ram,uint32_t pool);
int nflstreet_pool_push(nflstreet_pool_ram,uint32_t pool,uint32_t index);
int nflstreet_pool_pop(nflstreet_pool_ram,uint32_t pool,uint32_t *result);
int nflstreet_pool_allocate(nflstreet_pool_ram,uint32_t pool,
 const nflstreet_pool_services *,uint32_t *result);
#endif
