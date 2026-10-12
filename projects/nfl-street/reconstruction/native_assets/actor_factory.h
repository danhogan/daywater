#ifndef NFLSTREET_ACTOR_FACTORY_H
#define NFLSTREET_ACTOR_FACTORY_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint32_t token; uint8_t *bytes; size_t size; } nflstreet_factory_memory;
typedef void (*nflstreet_factory_constructor)(void *, nflstreet_factory_memory *, const void *);
typedef struct {
    uint32_t object_size, allocation_owner;
    nflstreet_factory_constructor construct;
    void *construction_context;
} nflstreet_factory_record;
typedef int (*nflstreet_factory_allocator)(void *, uint32_t owner, uint32_t minimum_size,
    nflstreet_factory_memory *);
/* Owns original factory allocation request, zeroing and header writes before
   optional construction. Callback has no result, as in the original factory.
   Caller owns registry, allocations and cleanup; malformed bounded inputs or
   allocation rejection return zero and clear only the result descriptor. */
int nflstreet_actor_create(const nflstreet_factory_record *, size_t record_count,
    uint32_t type, uint8_t header_byte, const void *input,
    nflstreet_factory_allocator, void *, nflstreet_factory_memory *result);
#endif
