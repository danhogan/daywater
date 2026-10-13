#include "selected_player_visual.h"
#include <string.h>
void nflstreet_selected_player_visual_close(nflstreet_selected_player_visual *v){
 if(!v)return;nflstreet_player_surface_close(&v->surface);nflstreet_selected_player_geometry_close(&v->geometry);memset(v,0,sizeof *v);
}
static int overlaps(uint32_t a,size_t an,uint32_t b,size_t bn){return(uint64_t)a<(uint64_t)b+bn&&(uint64_t)b<(uint64_t)a+an;}
static int conflict(const nflstreet_selected_player_visual_context *c,const nflstreet_player_surface *s,uint32_t token,size_t size){
 return overlaps(token,size,s->textures.token,s->textures.block_size)||
  overlaps(token,size,c->surface.scratch_token,s->textures.scratch_capacity)||
  overlaps(token,size,c->surface.material_token,c->surface.material_capacity);
}
int nflstreet_selected_player_visual_open(const uint8_t *db,size_t size,uint32_t team,uint32_t player,
 const uint8_t *record,const nflstreet_selected_player_visual_context *c,nflstreet_selected_player_visual *out){
 if(!c||!record||!out||!c->bodies||out->geometry.base.face.bytes[0]||out->surface.textures.block||out->surface.allocation_count)return 0;
 nflstreet_selected_player_visual v={0};
 if(!nflstreet_selected_player_geometry_open(c->resolver,db,size,team,player,c->bodies,c->body_variant,c->headwear,c->local_hair,
    c->face_identity,c->hair_identity,&v.geometry)||!v.geometry.base.manifest.custom_name_branch)goto fail;
 /* Establish the slot layout before any upload to reject cross-owner token
    collisions. It is discarded; the surface rebuilds the same layout once. */
 if(!nflstreet_slot_textures_open(c->surface.gear,c->surface.textures,c->surface.destination_token,&v.surface.textures))goto fail;
 for(unsigned i=0;i<2;++i)if(conflict(c,&v.surface,v.geometry.base.face.tokens[i],v.geometry.base.face.sizes[i]))goto fail;
 for(unsigned i=0;i<3;++i)if(v.geometry.hair.bytes[i]&&conflict(c,&v.surface,v.geometry.hair.tokens[i],v.geometry.hair.sizes[i]))goto fail;
 for(unsigned i=0;i<9;++i)if(conflict(c,&v.surface,c->bodies->tokens[i],c->bodies->assets[i].size))goto fail;
 nflstreet_slot_textures_close(&v.surface.textures);
 if(!nflstreet_player_surface_from_player(&c->surface,c->gear,c->dynamic_palettes,record,v.geometry.base.manifest.normalized_name,&v.surface))goto fail;
 *out=v;return 1;
fail:nflstreet_selected_player_visual_close(&v);return 0;
}
