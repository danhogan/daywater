#ifndef NFLSTREET_REGISTERED_POOL_H
#define NFLSTREET_REGISTERED_POOL_H
#include "actor_factory.h"
#include "object_pool.h"
typedef struct {
 nflstreet_pool_services pool;
 uint32_t (*get_error)(void *);
 void (*construct)(void *,uint32_t function,uint32_t object,uint32_t input);
} nflstreet_registered_services;
typedef struct {
 nflstreet_pool_ram ram;
 uint32_t registry_slot,alignment;
 size_t record_count;
 const nflstreet_registered_services *services;
} nflstreet_registered_context;
/* 80195400. Table/backing lifetime is borrowed. result is the original error
   code, including arbitrary get_error result on heap failure. Host rejection
   returns 0 and clears result; callback-induced invalid spans can reject after
   earlier original writes/services. No rollback/cleanup is invented. */
int nflstreet_registered_register(const nflstreet_registered_context *,uint32_t type,
 uint32_t size,uint32_t capacity,uint32_t constructor,uint32_t destructor,uint32_t *result);
/* 801956A4 through unchanged actor_factory. scratch has at least record_count
   host records; only scratch[type] is touched and its ephemeral callbacks must
   not be retained. Guest root/size/ctor are reread after allocation services.
   Output is the original object identity, zero for exhaustion. Host rejection
   returns 0/output0. Registry records and root must be disjoint from cleared
   object bytes. Object size >=0x12 and valid callback-mutated spans are host
   preconditions, not validation performed by the original game. */
int nflstreet_registered_create(const nflstreet_registered_context *,nflstreet_factory_record *scratch,
 size_t scratch_count,uint32_t type,uint8_t header,uint32_t input,uint32_t *result);
#endif
