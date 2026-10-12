#include "selected_player_geometry.h"
#include <string.h>
void nflstreet_selected_player_geometry_close(nflstreet_selected_player_geometry *r){
 if(r){nflstreet_selected_player_resources_close(&r->base);nflstreet_hair_resources_close(&r->hair);memset(r,0,sizeof *r);}
}
static int overlaps(uint32_t a,size_t an,uint32_t b,size_t bn){
 return (uint64_t)a<(uint64_t)b+bn&&(uint64_t)b<(uint64_t)a+an;
}
int nflstreet_selected_player_geometry_open(const nflstreet_team_player_resolver *resolver,
 const uint8_t *db,size_t size,uint32_t team,uint32_t player,
 const nflstreet_body_resource_pool *bodies,uint32_t variant,
 const nflstreet_headwear_catalog *catalog,const nflstreet_archive *local,
 uint32_t face_base,uint32_t hair_base,nflstreet_selected_player_geometry *out){
 if(!out)return 0;nflstreet_selected_player_geometry r={0};
 if(!nflstreet_selected_player_resources_open(resolver,db,size,team,player,bodies,variant,face_base,&r.base)||
    !nflstreet_headwear_resolve(catalog,db,size,team,player,&r.headwear)||
    r.headwear.play_row!=r.base.manifest.play_row||
    !nflstreet_hair_plan_make(&resolver->hair,r.base.manifest.normalized_name,
       (uint16_t)r.base.manifest.plht,(uint16_t)(r.base.manifest.phar==63?12:r.base.manifest.phar),
       (uint16_t)r.headwear.state,&r.hair_plan)||
    !nflstreet_hair_resources_open(local,&resolver->hair,&r.hair_plan,hair_base,&r.hair))goto fail;
 for(unsigned i=0;i<3;++i)if(r.hair.bytes[i]){
  for(unsigned j=0;j<2;++j)if(overlaps(r.hair.tokens[i],r.hair.sizes[i],r.base.face.tokens[j],r.base.face.sizes[j]))goto fail;
  for(unsigned j=0;j<9;++j)if(overlaps(r.hair.tokens[i],r.hair.sizes[i],bodies->tokens[j],bodies->assets[j].size))goto fail;
 }*out=r;return 1;
fail:nflstreet_selected_player_geometry_close(&r);return 0;
}
