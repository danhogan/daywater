#include "match_body_palettes.h"
#include <stdlib.h>
#include <string.h>
int nflstreet_match_body_palettes_build(const nflstreet_match_visuals *visuals,
 const nflstreet_match_body_pose *poses,const float polynomial[6],nflstreet_match_body_palettes *out){
 if(!visuals||visuals->count!=NFLSTREET_MATCH_PLAYERS||!poses||!polynomial||!out)return 0;
 nflstreet_match_body_palettes *staged=calloc(1,sizeof *staged);if(!staged)return 0;
 for(unsigned p=0;p<NFLSTREET_MATCH_PLAYERS;++p){const nflstreet_selected_player_resources *body=&visuals->players[p].geometry.base;
  if(!body->body||!body->body_view){free(staged);return 0;}
  nflstreet_model_view_source source={body->body->bytes,body->body->size,body->body_token};
  if(!nflstreet_cluster_palette(&source,body->body_view,poses[p].rotations,poses[p].rotation_count,
      poses[p].root,polynomial,staged->matrices[p],staged->counts+p)){free(staged);return 0;}
 }
 *out=*staged;free(staged);return 1;
}
