#ifndef NFLSTREET_VIEW_SYSTEM_H
#define NFLSTREET_VIEW_SYSTEM_H
#include "scene_owner.h"
#include "matrix_storage.h"
typedef struct {
 nflstreet_scene_owner_context owner;
 const nflstreet_matrix_storage_context *matrix;
 uint32_t r13;
 uint32_t (*get_error)(void *); /* owner.services->user */
} nflstreet_view_system_context;
/* Original80180FB4 ->80180490, type0 record only. Writes callback identities,
   source alignment arithmetic for32/16, byte2C=1; trailing3bytes stay dirty.
   This does not run whole nine-type library initialization80180444. */
int nflstreet_view_system_type0(nflstreet_pool_ram,uint32_t table,uint32_t alignment);
/* Original801DEFD8 ->801DF834. Descriptor: U32 view/callback capacities,
   U16 optional scene capacity, U8 stack index/depth (all multibyte BE).
   Original result in code, host status separately. Attempt sets initflag1
   even on failure, preserves partial allocations; repeated init returns80001.
   RAM/contexts/table/alignment/count and service state must remain stable.
   Matrix/table/descriptor/globals/allocations disjoint and bounded. Invalid
   index/layout/service responses reject after possible earlier writes; no rollback. */
int nflstreet_view_system_open(const nflstreet_view_system_context *,uint32_t descriptor,uint32_t *code);
#endif
