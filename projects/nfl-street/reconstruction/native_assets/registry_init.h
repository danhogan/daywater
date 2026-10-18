#ifndef NFLSTREET_REGISTRY_INIT_H
#define NFLSTREET_REGISTRY_INIT_H
#include "object_pool.h"
typedef struct {
 void *user;
 uint32_t (*heap)(void *,uint32_t owner,uint32_t size,uint32_t flags);
 uint32_t (*get_error)(void *);
} nflstreet_registry_init_services;
typedef struct {
 nflstreet_pool_ram ram;
 uint32_t count_slot,root_slot,flag_slot;
 const nflstreet_registry_init_services *services;
} nflstreet_registry_init_context;
/* 80195234: heap(1,count*20,0); clear only each record's first word,
   publish count then root and return1. Heap0 returns0 with globals unchanged.
   count0/nonzero heap still publishes. Leaves the wrapper flag untouched. */
int nflstreet_registry_init(const nflstreet_registry_init_context *,uint32_t count,uint32_t *result);
/* 80195368: flag!=0 returns90001 with no heap/count access. Else initialize,
   set flag1 on success/return0; failure returns exact get_error with flag0.
   Result is an original guest return. Host rejection is separate (return0,
   result0), requires nonwrapping count*20 and bounded/aligned accessed spans,
   and may follow an observed heap request. No free/rollback is invented. */
int nflstreet_registry_open(const nflstreet_registry_init_context *,uint32_t count,uint32_t *result);
#endif
