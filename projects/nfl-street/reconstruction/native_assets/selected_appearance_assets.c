#include "selected_appearance_assets.h"
#include <string.h>
void nflstreet_selected_appearance_assets_close(nflstreet_selected_appearance_assets *r){
 if(r){nflstreet_selected_player_geometry_close(&r->geometry);nflstreet_face_texture_resource_close(&r->face_texture);memset(r,0,sizeof *r);}
}
static int overlap(uint32_t a,size_t an,uint32_t b,size_t bn){return(uint64_t)a<(uint64_t)b+bn&&(uint64_t)b<(uint64_t)a+an;}
int nflstreet_selected_appearance_assets_open(const uint8_t *db,size_t size,uint32_t team,uint32_t player,
 const nflstreet_selected_appearance_context *c,nflstreet_selected_appearance_assets *out){
 if(!c||!out)return 0;nflstreet_selected_appearance_assets r={0};
 if(!nflstreet_selected_player_geometry_open(c->resolver,db,size,team,player,c->bodies,c->body_variant,
    c->headwear,c->local_hair,c->face_identity,c->hair_identity,&r.geometry)||
    !r.geometry.base.manifest.custom_name_branch||
    !nflstreet_face_texture_resource_open(c->face_textures,r.geometry.base.manifest.normalized_name,c->texture_identity,&r.face_texture))goto fail;
 for(unsigned i=0;i<2;++i)if(overlap(r.face_texture.token,r.face_texture.size,r.geometry.base.face.tokens[i],r.geometry.base.face.sizes[i]))goto fail;
 for(unsigned i=0;i<3;++i)if(r.geometry.hair.bytes[i]&&overlap(r.face_texture.token,r.face_texture.size,r.geometry.hair.tokens[i],r.geometry.hair.sizes[i]))goto fail;
 for(unsigned i=0;i<9;++i)if(overlap(r.face_texture.token,r.face_texture.size,c->bodies->tokens[i],c->bodies->assets[i].size))goto fail;
 *out=r;return 1;
fail:nflstreet_selected_appearance_assets_close(&r);return 0;
}
