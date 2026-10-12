#ifndef NFLSTREET_PLAYER_MODEL_BINDING_H
#define NFLSTREET_PLAYER_MODEL_BINDING_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint8_t *bytes;size_t size; } nflstreet_player_model_span;
typedef int (*nflstreet_player_model_resolver)(void *,uint32_t token,size_t minimum,nflstreet_player_model_span *);
typedef enum {
 NFLSTREET_MODEL_PREPARE_KEY,NFLSTREET_MODEL_RESOURCE,NFLSTREET_MODEL_RENDER_BLOCK,
 NFLSTREET_MODEL_PREPARE_STYLE,NFLSTREET_MODEL_NAMES_EQUAL,NFLSTREET_MODEL_RESET,
 NFLSTREET_MODEL_COMMIT_STYLE,NFLSTREET_MODEL_BIND_WORDS,NFLSTREET_MODEL_BIND_A,
 NFLSTREET_MODEL_BIND_B,NFLSTREET_MODEL_FINISH
} nflstreet_player_model_service_kind;
typedef struct { nflstreet_player_model_service_kind kind;uint32_t target_offset,arguments[3]; } nflstreet_player_model_request;
typedef int (*nflstreet_player_model_service)(void *,const nflstreet_player_model_request *,uint8_t *,size_t,uint32_t *);
typedef int (*nflstreet_player_model_allocator)(void *,uint32_t count,uint32_t stride,uint32_t *token,nflstreet_player_model_span *);
typedef int (*nflstreet_player_model_release)(void *,uint32_t token);
typedef struct {
 uint32_t command_bytes,mapping_table_token;
 uint8_t *scratch;size_t scratch_size;
 uint8_t *shared;size_t shared_size;
 nflstreet_player_model_resolver resolve;void *resolve_context;
 nflstreet_player_model_allocator allocate;void *allocate_context;
 nflstreet_player_model_release release;void *release_context;
 nflstreet_player_model_service service;void *service_context;
} nflstreet_player_model_binding_context;
/* Owns model copies, command headers, index buffers/remapping, table mutations
   and original orchestration. Actual renderer/cache owners remain required
   services. All pointers are opaque namespace tokens resolved to bounded views.
   Returns0 on malformed/rejected inputs without undoing prior owned writes. */
int nflstreet_player_model_bind(uint8_t *,size_t,const nflstreet_player_model_binding_context *);
#endif
