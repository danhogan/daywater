#include "selected_player_visual.h"
#include "player_record.h"
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

static int identity_number(const nflstreet_team_table *t,unsigned row,uint32_t tag,uint32_t *value){
 unsigned found=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(found++||!nflstreet_team_table_number(t,row,i,value))return 0;}}
 return found==1;
}
int nflstreet_selected_player_visual_from_match(const uint8_t *db,size_t size,const nflstreet_game_roster *match,unsigned row,
 unsigned side,
 const nflstreet_selected_player_visual_context *c,nflstreet_selected_player_visual *out){
 if(!match||match->source_play.bytes!=db||match->source_play.size!=size||!match->bytes||!match->source_rows||row>=match->table.used||!c||!out)return 0;
 nflstreet_team_table stat,teams,colors;uint32_t team,player;uint8_t record[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]={0};
 if(!nflstreet_team_table_open_tag(db,size,0x59414c50u,&stat)||
    stat.fields_at!=match->source_play.fields_at||stat.records_at!=match->source_play.records_at||
    stat.used!=match->source_play.used||stat.record_bytes!=match->source_play.record_bytes||
    stat.record_bits!=match->source_play.record_bits||stat.field_count!=match->source_play.field_count||
    !nflstreet_team_table_open_tag(db,size,0x4d414554u,&teams)||!nflstreet_team_table_open_tag(db,size,0x4c434750u,&colors)||
    match->source_rows[row]>=stat.used||
    !identity_number(&stat,match->source_rows[row],0x44494754u,&team)||
    !identity_number(&stat,match->source_rows[row],0x44494750u,&player)||
    !nflstreet_player_record_build(&match->table,row,c->gear,&teams,&colors,c->surface.gear,side,record))return 0;
 return nflstreet_selected_player_visual_open(db,size,team,player,record,c,out);
}
