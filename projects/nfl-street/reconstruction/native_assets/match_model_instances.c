#include "match_model_instances.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static uint32_t word(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
int nflstreet_match_model_instances_build(const nflstreet_match_visuals *visuals,const nflstreet_match_body_palettes *body,
 const nflstreet_match_attachment_palettes *attachment,const nflstreet_match_instance_input inputs[14],nflstreet_match_model_instances*out){
 if(!visuals||visuals->count!=14||!body||!attachment||!inputs||!out)return 0;
 const uint32_t span=6u*NFLSTREET_MATCH_INSTANCE_PART_BYTES;
 for(unsigned p=0;p<14;++p){if(!inputs[p].base||inputs[p].base>UINT32_MAX-span||!inputs[p].materials)return 0;
  for(unsigned j=0;j<p;++j)if((uint64_t)inputs[p].base<inputs[j].base+(uint64_t)span&&(uint64_t)inputs[j].base<inputs[p].base+(uint64_t)span)return 0;
 }
 nflstreet_match_model_instances *staged=calloc(1,sizeof *staged);if(!staged)return 0;
 for(unsigned p=0;p<14;++p)for(unsigned part=0;part<6;++part){const nflstreet_selected_player_geometry*g=&visuals->players[p].geometry;
  const uint8_t*view;const float(*matrices)[12];unsigned count;nflstreet_model_view_source model;
  if(!part){view=g->base.body_view;matrices=body->matrices[p];count=body->counts[p];if(!g->base.body)goto fail;model=(nflstreet_model_view_source){g->base.body->bytes,g->base.body->size,g->base.body_token};}
  else{unsigned at=part-1;view=at<2?g->base.face.views[at]:g->hair.views[at-2];matrices=attachment->matrices[p][at];count=attachment->counts[p][at];model=at<2?(nflstreet_model_view_source){g->base.face.bytes[at],g->base.face.sizes[at],g->base.face.tokens[at]}:(nflstreet_model_view_source){g->hair.bytes[at-2],g->hair.sizes[at-2],g->hair.tokens[at-2]};}
  if(!count){if(!part||part<3||model.bytes||model.size||model.token)goto fail;continue;}if(!view||count>255||!model.bytes)goto fail;
  nflstreet_model_view_chunk clst;if(nflstreet_model_view_find(&model,0x436c7374u,&clst)!=1||clst.size<8||word(clst.bytes+4)!=count||word(view+12)!=clst.encoded_word)goto fail;
  nflstreet_owned_model_instance*d=&staged->parts[p][part];d->token=inputs[p].base+part*NFLSTREET_MATCH_INSTANCE_PART_BYTES;d->count=count;
  const uint32_t view_at=240,palette_at=292,matrix_at=304;
  memcpy(d->bytes+view_at,view,52);put(d->bytes+palette_at,d->token+matrix_at);put(d->bytes+palette_at+4,0);put(d->bytes+palette_at+8,count);
  for(unsigned i=0;i<count;++i)for(unsigned w=0;w<12;++w){uint32_t bits;if(!isfinite(matrices[i][w]))goto fail;memcpy(&bits,matrices[i]+w,4);put(d->bytes+matrix_at+i*48+w*4,bits);}
  if(!nflstreet_model_instance_initialize(d->bytes,d->token+view_at,d->token+palette_at,inputs[p].materials))goto fail;
 }
 *out=*staged;free(staged);return 1;
fail:free(staged);return 0;
}
uint8_t*nflstreet_match_model_instances_span(nflstreet_match_model_instances *owner,uint32_t token,uint32_t size){
 if(!owner||!token)return NULL;for(unsigned p=0;p<14;++p)for(unsigned part=0;part<6;++part){nflstreet_owned_model_instance*d=&owner->parts[p][part];
  if(d->token&&token>=d->token&&(uint64_t)token-d->token+size<=sizeof d->bytes)return d->bytes+token-d->token;}return NULL;
}
