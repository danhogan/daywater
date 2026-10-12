#ifndef NFLSTREET_SELECTED_PLAYER_GEOMETRY_H
#define NFLSTREET_SELECTED_PLAYER_GEOMETRY_H
#include "selected_player_resources.h"
#include "hair_resources.h"
#include "headwear_state.h"
typedef struct {
 nflstreet_selected_player_resources base;
 nflstreet_headwear_selection headwear;
 nflstreet_hair_plan hair_plan;
 nflstreet_hair_resources hair;
} nflstreet_selected_player_geometry;
/* Own selected face/hair files, borrow a processed body variant, derive
   headwear from PLAY/GEAR. Body variant remains explicit. Catalog/archive/body
   lifetimes are caller-owned. Binding/materials/actor creation remain separate.
   Any failure leaves output unchanged and releases newly owned files. */
int nflstreet_selected_player_geometry_open(const nflstreet_team_player_resolver *,
 const uint8_t *,size_t,uint32_t team,uint32_t player,
 const nflstreet_body_resource_pool *,uint32_t variant,
 const nflstreet_headwear_catalog *,const nflstreet_archive *local_hair,
 uint32_t face_base,uint32_t hair_base,nflstreet_selected_player_geometry *);
void nflstreet_selected_player_geometry_close(nflstreet_selected_player_geometry *);
#endif
