#ifndef NFLSTREET_VIEW_NODES_H
#define NFLSTREET_VIEW_NODES_H
#include "list_nodes.h"
typedef struct {
 nflstreet_list_nodes_context lists;
 uint32_t view_list_slot,callback_list_slot,display_word,constants;
 uint32_t (*get_error)(void *); /* lists.services->user */
} nflstreet_view_nodes_context;
/* 801DEF40/801DEE3C: signed interpretation of wrapping first-word subtraction.
   Adapter user is a borrowed nflstreet_pool_ram*. Other functions reject. */
int nflstreet_view_node_compare(void *,uint32_t function,uint32_t left,uint32_t right,int32_t *result);
/* 801DF14C and literal no-op801DF8B4. Original return is payload or0 on
   exhaustion. Descriptor words and F8-byte payload must be disjoint/stable.
   countbyte+3 <=50 is host safety; copied words may be overwritten by later
   original default stores. Constants borrow original32bytes from80263CC8.
   Does not initialize projection/frustum, bind a scene or allocate a stack. */
int nflstreet_view_node_create(const nflstreet_view_nodes_context *,uint32_t descriptor,uint32_t *result);
/* 801DF39C: payload={arg4,arg2,arg3,arg1}, publish mode0, return0;
   allocation exhaustion returns exact get_error, including0/highbit. */
int nflstreet_view_callback_register(const nflstreet_view_nodes_context *,uint32_t arg1,
 uint32_t arg2,uint32_t arg3,uint32_t arg4,uint32_t *result);
#endif
