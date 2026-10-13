#include "match_attachment_palettes.h"
#include <stdlib.h>
#include <string.h>
static uint32_t word(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int nflstreet_match_attachment_palettes_build(const nflstreet_match_visuals *visuals,
 const nflstreet_match_attachment_pose poses[14][5],const float polynomial[6],nflstreet_match_attachment_palettes *out){
 if(!visuals||visuals->count!=14||!poses||!polynomial||!out)return 0;
 nflstreet_match_attachment_palettes *staged=calloc(1,sizeof *staged);if(!staged)return 0;
 for(unsigned p=0;p<14;++p)for(unsigned part=0;part<5;++part){const nflstreet_selected_player_geometry*g=&visuals->players[p].geometry;
  nflstreet_model_view_source model;const uint8_t*view;
  if(part<2){model=(nflstreet_model_view_source){g->base.face.bytes[part],g->base.face.sizes[part],g->base.face.tokens[part]};view=g->base.face.views[part];}
  else{unsigned h=part-2;model=(nflstreet_model_view_source){g->hair.bytes[h],g->hair.sizes[h],g->hair.tokens[h]};view=g->hair.views[h];}
  if(!model.bytes){if(model.size||model.token)goto fail;continue;}
  const nflstreet_match_attachment_pose*pose=&poses[p][part];float scratch[256][12];unsigned count=0;
  if(!nflstreet_cluster_palette(&model,view,pose->pose.rotations,pose->pose.rotation_count,pose->pose.root,polynomial,scratch,&count))goto fail;
  nflstreet_model_view_chunk clst;if(nflstreet_model_view_find(&model,0x436c7374u,&clst)!=1||clst.size<8)goto fail;
  uint32_t token=word(clst.bytes);if(token<model.token)goto fail;size_t off=token-model.token;
  if(off>model.size||count*4u>model.size-off)goto fail;
  memcpy(staged->matrices[p][part],scratch,count*48u);
  if(!nflstreet_attachment_palette_scale(model.bytes+off,model.size-off,count,pose->slot,pose->lod,pose->scale,scratch,staged->matrices[p][part]))goto fail;
  staged->counts[p][part]=count;
 }
 *out=*staged;free(staged);return 1;
fail:free(staged);return 0;
}
