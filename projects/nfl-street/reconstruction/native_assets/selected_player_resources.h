#ifndef NFLSTREET_SELECTED_PLAYER_RESOURCES_H
#define NFLSTREET_SELECTED_PLAYER_RESOURCES_H
#include "face_resource_pair.h"
#include "body_resource_pool.h"
#include "../menus/team_player_resolve.h"
typedef struct {
    nflstreet_team_player_manifest manifest;
    nflstreet_face_resource_pair face;
    const nflstreet_team_body_binding *body;
    const uint8_t *body_view;
    uint32_t body_token,body_variant;
} nflstreet_selected_player_resources;
/* Resolve exact team/DIGP and load its processed face pair; borrow an explicit
   processed body variant. Body pool/resolver archives must outlive the result.
   This is resource assembly, not complete actor/material/texture creation. */
int nflstreet_selected_player_resources_open(const nflstreet_team_player_resolver *,
    const uint8_t *team_db,size_t,uint32_t team,uint32_t player,
    const nflstreet_body_resource_pool *,uint32_t body_variant,
    uint32_t face_identity_base,nflstreet_selected_player_resources *);
void nflstreet_selected_player_resources_close(nflstreet_selected_player_resources *);
#endif
