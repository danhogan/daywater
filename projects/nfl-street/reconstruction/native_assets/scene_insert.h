#ifndef NFLSTREET_SCENE_INSERT_H
#define NFLSTREET_SCENE_INSERT_H
#include "registered_pool.h"
typedef struct {
 nflstreet_pool_ram ram;
 uint32_t type_table;
 const nflstreet_pool_services *pool;
 uint32_t (*get_error)(void *);
 int32_t (*compare)(void *,uint32_t function,uint32_t left_payload,uint32_t right_payload);
} nflstreet_scene_insert_context;
/* Native80194F04: payloads hold BE guest object pointers; compare BE u16+10.
   Host status is separate from signed original comparator return. */
int nflstreet_scene_default_compare(nflstreet_pool_ram,uint32_t left_payload,uint32_t right_payload,int32_t *result);
/* 8019573C, type0 only. result=0 on insertion, exact get_error on node
   exhaustion (which may also be0). Byte2 increments modulo256 only on insert.
   Scene/list/type descriptor/pool/links must remain valid and stable during services;
   custom comparator is an explicit pure service, default executes natively.
   Nodes/objects/list/table must not overlap; live nodes must not be free slots.
   Malformed host spans/unsupported dispatch reject with result0 and host0;
   rejection may follow allocation/services. No rollback or free is invented. */
int nflstreet_scene_insert(const nflstreet_scene_insert_context *,uint32_t scene,uint32_t object,uint32_t *result);
/* 801957A4: unchanged factory allocator join, then insert. Return object even
   when insertion's original error is nonzero; only factory0 skips insertion.
   Both contexts must name the same RAM. Scratch follows registered_create. */
int nflstreet_scene_create_insert(const nflstreet_scene_insert_context *,const nflstreet_registered_context *,
 nflstreet_factory_record *scratch,size_t scratch_count,uint32_t scene,uint32_t type,uint8_t header,uint32_t input,uint32_t *result);
#endif
