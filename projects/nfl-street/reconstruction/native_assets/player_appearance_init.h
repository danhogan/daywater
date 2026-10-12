#ifndef NFLSTREET_PLAYER_APPEARANCE_INIT_H
#define NFLSTREET_PLAYER_APPEARANCE_INIT_H
#include <stddef.h>
#include <stdint.h>
/* Data catalog values are supplied from the source asset owner. No embedded
   proprietary tables or original RAM addresses appear in the implementation. */
typedef struct {
    const uint32_t *indexed_words; size_t indexed_word_count;
    const uint32_t (*mapping_words)[25]; size_t mapping_count;
    uint32_t channel_default_word, matrix_binding_token;
    const uint32_t *side_texture_tokens; size_t side_count;
    const uint32_t (*finish_records)[7]; size_t finish_count;
} nflstreet_player_appearance_catalog;
typedef struct {
    uint8_t *actor_entries; size_t actor_entries_size; uint32_t actor_entries_token, actor_count;
    uint8_t *binding_entries; size_t binding_entries_size; uint32_t binding_count;
} nflstreet_player_appearance_registry;
typedef enum {
    NFLSTREET_APPEARANCE_BASE_RESOURCE, NFLSTREET_APPEARANCE_SIDE_RESOURCE,
    NFLSTREET_APPEARANCE_MODEL_RESOURCE, NFLSTREET_APPEARANCE_BIND_MODEL,
    NFLSTREET_APPEARANCE_TEXTURE_RESET,
    NFLSTREET_APPEARANCE_TEXTURE_SELECT, NFLSTREET_APPEARANCE_COPY_NODES,
    NFLSTREET_APPEARANCE_BIND_NODES, NFLSTREET_APPEARANCE_BIND_NODE,
    NFLSTREET_APPEARANCE_FIND_SIDE_NODE, NFLSTREET_APPEARANCE_RESET_SIDE_NODE,
    NFLSTREET_APPEARANCE_APPLY_SIDE_NODE
} nflstreet_player_appearance_service_kind;
typedef struct {
    nflstreet_player_appearance_service_kind kind;
    uint32_t target_offset, arguments[6];
} nflstreet_player_appearance_request;
/* Callback returns a resource/token word. Actor span is mutable so explicit
   unresolved binding owners can write their owned fields. */
typedef int (*nflstreet_player_appearance_service)(void *,const nflstreet_player_appearance_request *,
    uint8_t *,size_t,uint32_t *);
typedef enum { NFLSTREET_APPEARANCE_NODE_TABLE,NFLSTREET_APPEARANCE_CHANNEL_TABLE } nflstreet_player_appearance_allocation_kind;
typedef struct { uint32_t token; uint8_t *bytes; size_t size; } nflstreet_player_appearance_memory;
typedef int (*nflstreet_player_appearance_allocator)(void *,
    nflstreet_player_appearance_allocation_kind,uint32_t count,uint32_t stride,nflstreet_player_appearance_memory *);
/* Model resource response is a token plus a separately supplied node-list token
   consumed by COPY_NODES. The service owns their actual resource mapping. */
typedef struct {
    uint32_t actor_token, model_node_list_token;
    const nflstreet_player_appearance_catalog *catalog;
    nflstreet_player_appearance_registry *registry;
    nflstreet_player_appearance_service service; void *service_context;
    nflstreet_player_appearance_allocator allocate; void *allocation_context;
} nflstreet_player_appearance_context;
/* Owns original appearance orchestration, vectors, node descriptors, mapping
   state and registry writes. Complex rendering/resource bindings stay explicit
   services. Returns0 on malformed inputs/rejected services without rollback. */
int nflstreet_player_appearance_initialize(uint8_t *,size_t,const uint8_t *,size_t,
    const nflstreet_player_appearance_context *);
#endif
