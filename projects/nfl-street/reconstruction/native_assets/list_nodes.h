#ifndef NFLSTREET_LIST_NODES_H
#define NFLSTREET_LIST_NODES_H
#include "object_pool.h"
typedef struct {
 nflstreet_pool_ram ram;
 uint32_t type_table;
 const nflstreet_pool_services *services;
 void *compare_user;
 int (*compare)(void *,uint32_t function,uint32_t left,uint32_t right,int32_t *result);
} nflstreet_list_nodes_context;
/* Type0 80180700: allocate a raw node, clear exactly its 16-byte header,
   clear membership bit, return payload. Exhaustion is host success/result0.
   Payload and list fields remain dirty/unchanged. */
int nflstreet_list_node_allocate(const nflstreet_list_nodes_context *,uint32_t list,uint32_t *result);
/* Type0 80180784, mode0 only. Publish a unique, unlinked allocated payload;
   comparator0 prepends, otherwise insert before first compare>=0, or append.
   Set membership bit and return payload; no scene object membership byte.
   List/table/nodes must be bounded, word aligned, disjoint and stable through
   services. Optional comparator is a pure borrowed service, not gameplay policy.
   Unsupported dispatch/invalid RAM rejects (host0/result0), possibly after
   earlier writes; no rollback. Caller retains all storage and services. */
int nflstreet_list_node_publish(const nflstreet_list_nodes_context *,uint32_t list,uint32_t payload,uint32_t *result);
#endif
