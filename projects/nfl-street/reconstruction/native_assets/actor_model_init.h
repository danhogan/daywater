#ifndef NFLSTREET_ACTOR_MODEL_INIT_H
#define NFLSTREET_ACTOR_MODEL_INIT_H
#include <stddef.h>
#include <stdint.h>
#define NFLSTREET_ACTOR_MODEL_STORAGE 0x1a90u
/* Tokens belong to the caller's isolated object/resource namespace. They are
   never host pointers and this API has no game RAM addresses. */
typedef struct { uint32_t token; const uint8_t *bytes; size_t size; } nflstreet_actor_model_resource;
typedef struct {
    nflstreet_actor_model_resource skeleton, primary[2], secondary[2], composite;
    uint32_t initial_word_bits;
} nflstreet_actor_model_descriptor;
typedef enum { NFLSTREET_ACTOR_TRANSFORMS, NFLSTREET_ACTOR_CHANNEL } nflstreet_actor_model_allocation_kind;
typedef struct { nflstreet_actor_model_allocation_kind kind; uint32_t bytes, source_count; } nflstreet_actor_model_allocation;
typedef struct { uint32_t token; uint8_t *bytes; size_t size; } nflstreet_actor_model_memory;
typedef int (*nflstreet_actor_model_allocator)(void *,const nflstreet_actor_model_allocation *,nflstreet_actor_model_memory *);
/* Owns base model initialization and its channel writes. Caller supplies an
   isolated big-endian opaque actor span and allocation service. The actor is
   normally fresh/zeroed; bytes outside original-owned writes are preserved.
   Returns0 for malformed resources or rejected allocation, retaining any
   earlier allocation and mutation. No player appearance or actors are created. */
int nflstreet_actor_model_initialize(uint8_t *actor,size_t actor_size,
    const nflstreet_actor_model_descriptor *,nflstreet_actor_model_allocator,void *);
#endif
