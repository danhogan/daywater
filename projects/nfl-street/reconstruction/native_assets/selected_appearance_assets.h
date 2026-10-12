#ifndef NFLSTREET_SELECTED_APPEARANCE_ASSETS_H
#define NFLSTREET_SELECTED_APPEARANCE_ASSETS_H
#include "selected_player_geometry.h"
#include "face_texture.h"
typedef struct {
 const nflstreet_team_player_resolver *resolver;
 const nflstreet_body_resource_pool *bodies;
 const nflstreet_headwear_catalog *headwear;
 const nflstreet_archive *local_hair,*face_textures;
 uint32_t body_variant,face_identity,hair_identity,texture_identity;
} nflstreet_selected_appearance_context;
typedef struct {nflstreet_selected_player_geometry geometry;nflstreet_face_texture_resource face_texture;} nflstreet_selected_appearance_assets;
/* Join owned geometry and selected face texture. Body variants are explicit;
   palette/material application, remaining textures and actor startup are not
   part of this owner. Failure preserves output and releases new allocations. */
int nflstreet_selected_appearance_assets_open(const uint8_t *,size_t,uint32_t team,uint32_t player,
 const nflstreet_selected_appearance_context *,nflstreet_selected_appearance_assets *);
void nflstreet_selected_appearance_assets_close(nflstreet_selected_appearance_assets *);
#endif
