#ifndef NFLSTREET_BODY_RESOURCE_POOL_H
#define NFLSTREET_BODY_RESOURCE_POOL_H
#include "model_view.h"
#include "model_relocate.h"
#include "../menus/team_player_body_binding.h"
typedef struct {
    nflstreet_team_body_binding assets[9];
    uint8_t views[9][NFLSTREET_MODEL_VIEW_BYTES];
    uint32_t tokens[9];
} nflstreet_body_resource_pool;
/* Owns decoded base-body bytes and raw attached views for all verified keys
   and variants. identity_base is a caller namespace, not original RAM.
   These raw section offsets are not relocated runtime model pointers. */
int nflstreet_body_resource_pool_open(const uint8_t *, size_t, uint32_t identity_base,
    nflstreet_body_resource_pool *);
void nflstreet_body_resource_pool_close(nflstreet_body_resource_pool *);
/* Relocate each owned asset using its namespace identity, then attach its
   processed view. Rejection retains earlier changes; caller may close pool. */
int nflstreet_body_resource_pool_process(nflstreet_body_resource_pool *);
/* TBYP7 aliases key2, matching the existing verified body binder. Returned
   views/assets remain borrowed until close. No automatic variant policy. */
int nflstreet_body_resource_pool_get(const nflstreet_body_resource_pool *, uint32_t tbyp,
    uint32_t variant, const nflstreet_team_body_binding **, const uint8_t **view);
#endif
