#ifndef NFLSTREET_SCENE_OWNER_H
#define NFLSTREET_SCENE_OWNER_H
#include "object_pool.h"
typedef struct {
 nflstreet_pool_ram ram;
 uint32_t type_table,alignment,registry_count;
 const nflstreet_pool_services *services;
} nflstreet_scene_owner_context;
/* Borrowed type-table descriptor, alignment and registry-count snapshot must
   match the original call inputs. Only type0/init80180C20 is supported; table
   layout fields and alignment must remain stable during service callbacks.
   Host success is separate from original result0. Host rejection clears result
   and may follow earlier game writes/services; no rollback is invented. */
int nflstreet_scene_list_layout(const nflstreet_scene_owner_context *,uint32_t type,
 uint32_t capacity,uint32_t element_size,uint32_t *stride,uint32_t *total);
int nflstreet_scene_list_create(const nflstreet_scene_owner_context *,uint32_t owner,
 uint32_t type,uint32_t capacity,uint32_t element_size,uint32_t callback,uint32_t supplied,uint32_t *result);
/* 801954E4. Arguments follow the original eight-register order. Type0 only;
   callback0 selects80194F04, stored but never called in this slice. List heap
   failure still returns/initializes the scene, with scene+0 list pointer0. */
int nflstreet_scene_owner_create(const nflstreet_scene_owner_context *,uint32_t owner,
 uint32_t capacity,uint32_t type,uint32_t callback,uint32_t field0c,uint32_t multiplier,
 uint32_t field10,uint32_t field1c,uint32_t *result);
#endif
