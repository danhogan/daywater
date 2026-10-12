#ifndef NFLSTREET_ACTOR_TAIL_INIT_H
#define NFLSTREET_ACTOR_TAIL_INIT_H
#include <stddef.h>
#include <stdint.h>
#define NFLSTREET_ACTOR_TAIL_STORAGE 0x728u
/* Literal source defaults, supplied by the owning data catalog. Names do not
   assign gameplay meanings to the original fields. */
typedef struct {
    uint32_t repeated_float_bits;
    uint32_t default_float_bits;
    uint32_t defaults[4];
} nflstreet_actor_tail_defaults;
/* Reconstructs800D0448 and800D1330. Preserves all bytes outside their writes.
   Does not allocate, load assets, or initialize player appearance. */
int nflstreet_actor_tail_initialize(uint8_t *,size_t,
    const nflstreet_actor_tail_defaults *);
#endif
