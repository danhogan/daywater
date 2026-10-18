#ifndef NFLSTREET_SCENE_REMOVE_H
#define NFLSTREET_SCENE_REMOVE_H
#include "scene_insert.h"
/* Default predicate80195344: payload contains object; target is an in/out
   host word initialized to object, replaced with payload on match. Original
   guest result is0(match)/1(other). */
int nflstreet_scene_match(nflstreet_pool_ram,uint32_t payload,uint32_t *target,uint32_t *result);
/* Type0 forward search80180A14 with built-in predicate, mode0,direction1.
   Returns actual child status1(empty)/0(absent)/4(found), and target remains
   object unless found, when it becomes the first matching payload identity. */
int nflstreet_scene_find(const nflstreet_scene_insert_context *,uint32_t list,uint32_t object,
 uint32_t *target,uint32_t *result);
/* 80181334 arithmetic only: node token is NOT dereferenced. Unsigned wrapping
   (node-data)/stride quotient is pushed/truncated by unchanged pool kernel. */
int nflstreet_scene_pool_release(nflstreet_pool_ram,uint32_t pool,uint32_t node);
/* 801808EC on a valid owned payload. A marked node is unlinked, bit31 cleared;
   stale links/payload remain, then its index is returned to the list pool. */
int nflstreet_scene_node_release(const nflstreet_scene_insert_context *,uint32_t list,uint32_t payload);
/* Original scene remove8019585C and reorder801958E8 are void. These return
   HOST status only. Default scene+0C=0 is supported; nonzero callback identity
   is unavailable. Caller must supply a present member with valid links.
   Missing membership is host rejection, not recovered original behavior.
   Byte2 may be0 (wrap); no positive-counter validation is invented.
   Reorder removes, stores low16 order at object+10, then uses existing insert.
   Same stable/disjoint RAM/service contract as scene_insert; no actor free,
   rollback or teardown. Rejection may follow earlier writes/services. */
int nflstreet_scene_remove(const nflstreet_scene_insert_context *,uint32_t scene,uint32_t object);
int nflstreet_scene_reorder(const nflstreet_scene_insert_context *,uint32_t scene,uint32_t object,uint32_t order);
#endif
