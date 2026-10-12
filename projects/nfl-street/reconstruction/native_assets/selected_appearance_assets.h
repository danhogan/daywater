#ifndef NFLSTREET_SELECTED_APPEARANCE_ASSETS_H
#define NFLSTREET_SELECTED_APPEARANCE_ASSETS_H
#include "selected_player_geometry.h"
#include "face_texture.h"
#include "texture_apply.h"
typedef struct {
 const nflstreet_team_player_resolver *resolver;
 const nflstreet_body_resource_pool *bodies;
 const nflstreet_headwear_catalog *headwear;
 const nflstreet_archive *local_hair,*face_textures;
 uint32_t body_variant,face_identity,hair_identity,texture_identity;
} nflstreet_selected_appearance_context;
typedef struct {nflstreet_selected_player_geometry geometry;nflstreet_face_texture_resource face_texture;nflstreet_applied_face_texture applied_face;} nflstreet_selected_appearance_assets;
/* Join owned geometry and selected face texture. Body variants are explicit;
   palette/material application, remaining textures and actor startup are not
   part of this owner. Failure preserves output and releases new allocations. */
int nflstreet_selected_appearance_assets_open(const uint8_t *,size_t,uint32_t team,uint32_t player,
 const nflstreet_selected_appearance_context *,nflstreet_selected_appearance_assets *);
/* Apply the selected image/palette into an owned template, preserving the old
   destination on failure. Native GPU/upload behavior belongs to flush owner. */
int nflstreet_selected_appearance_assets_apply_face(nflstreet_selected_appearance_assets *,
 const nflstreet_selected_appearance_context *,uint32_t,nflstreet_texture_flush,void *);
void nflstreet_selected_appearance_assets_close(nflstreet_selected_appearance_assets *);
#endif
