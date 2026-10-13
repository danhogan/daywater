#ifndef NFLSTREET_SELECTED_PLAYER_VISUAL_H
#define NFLSTREET_SELECTED_PLAYER_VISUAL_H
#include "selected_player_geometry.h"
#include "player_surface.h"
typedef struct {
 const nflstreet_team_player_resolver *resolver;
 const nflstreet_body_resource_pool *bodies;
 const nflstreet_headwear_catalog *headwear;
 const nflstreet_archive *local_hair,*dynamic_palettes;
 const nflstreet_gear_records *gear;
 uint32_t body_variant,face_identity,hair_identity;
 nflstreet_player_surface_context surface;
} nflstreet_selected_player_visual_context;
/* Geometry plus every selected slot and its material instances. Borrowed body,
   resolver archives and catalogs must outlive this owner. A renderer can use
   these native views; skeleton attachment and actor creation remain separate. */
typedef struct {
 nflstreet_selected_player_geometry geometry;
 nflstreet_player_surface surface;
} nflstreet_selected_player_visual;
/* The completed player record must describe this selected roster identity.
   It is supplied by the match/player-record owner, never inferred from RAM.
   Output must be empty. Failure releases staged ownership and preserves output;
   successful flush callback side effects cannot be undone. All surface token
   spans are checked against the face/hair and borrowed body resource spans. */
int nflstreet_selected_player_visual_open(const uint8_t *team_db,size_t,uint32_t team,uint32_t player,
 const uint8_t record[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],const nflstreet_selected_player_visual_context *,
 nflstreet_selected_player_visual *);
void nflstreet_selected_player_visual_close(nflstreet_selected_player_visual *);
#endif
