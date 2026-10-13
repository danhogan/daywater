#define main appearance_stage_main
#include "selected_appearance_assets_check.c"
#undef main
#include "selected_player_visual.h"
#include "player_record.h"
#include "game_roster.h"
#include "../menus/team_identity.h"
static unsigned uploads;
static int upload(void *opaque,uint32_t token,uint32_t size){(void)opaque;if(!token||!size)return 0;++uploads;return 1;}
static int rejected_upload(void *opaque,uint32_t token,uint32_t size){(void)opaque;(void)token;(void)size;return 0;}
static int equal_geometry(const nflstreet_selected_player_geometry *a,const nflstreet_selected_player_geometry *b){
 if(memcmp(&a->base.manifest,&b->base.manifest,sizeof a->base.manifest)||a->base.body_token!=b->base.body_token||a->base.body_view!=b->base.body_view||
    memcmp(&a->headwear,&b->headwear,sizeof a->headwear)||memcmp(&a->hair_plan,&b->hair_plan,sizeof a->hair_plan))return 0;
 for(unsigned i=0;i<2;++i)if(a->base.face.sizes[i]!=b->base.face.sizes[i]||a->base.face.tokens[i]!=b->base.face.tokens[i]||
    memcmp(a->base.face.bytes[i],b->base.face.bytes[i],a->base.face.sizes[i])||memcmp(a->base.face.views[i],b->base.face.views[i],NFLSTREET_MODEL_VIEW_BYTES))return 0;
 for(unsigned i=0;i<3;++i)if(a->hair.sizes[i]!=b->hair.sizes[i]||a->hair.tokens[i]!=b->hair.tokens[i]||
    (a->hair.bytes[i]&&memcmp(a->hair.bytes[i],b->hair.bytes[i],a->hair.sizes[i])))return 0;return 1;
}
#include "match_visuals.h"
#include "match_skeletons.h"
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t hash_geometry(const nflstreet_match_visuals *v){uint32_t hash=2166136261u;
 for(unsigned p=0;p<14;++p){const nflstreet_selected_player_geometry *g=&v->players[p].geometry;
  const uint8_t *bytes[11]={g->base.face.bytes[0],g->base.face.bytes[1],g->base.face.views[0],g->base.face.views[1],
   g->hair.bytes[0],g->hair.bytes[1],g->hair.bytes[2],g->hair.views[0],g->hair.views[1],g->hair.views[2],g->base.body->bytes};
  size_t sizes[11]={g->base.face.sizes[0],g->base.face.sizes[1],52,52,g->hair.sizes[0],g->hair.sizes[1],g->hair.sizes[2],52,52,52,g->base.body->size};
  for(unsigned b=0;b<11;++b)if(bytes[b])for(size_t n=0;n<sizes[b];++n)hash=(hash^bytes[b][n])*16777619u;
 }return hash;
}
int main(int argc,char **argv){
 if(argc!=15||!initialize_assets(argv))return 2;
 size_t sizes[6];uint8_t *data[6];unsigned args[]={1,2,3,10,11,12};
 for(unsigned i=0;i<6;++i)if(!(data[i]=input(argv[args[i]],sizes+i)))return 3;
 nflstreet_team_player_resolver resolver;nflstreet_archive textures,dynamic;nflstreet_gear_texture_catalog catalog;
 nflstreet_slot_material_catalog materials;nflstreet_gear_records records;nflstreet_color_table color_table;nflstreet_dynclut_table dynclut;
 nflstreet_team_table gear_table,teams,player_colors,stat_play,thcd,game_schema;
 size_t schema_size;uint8_t *schema=input(argv[13],&schema_size);
 size_t template_size;uint8_t *template_db=input(argv[8],&template_size);
 if(!schema||!nflstreet_db_schema_open(schema,schema_size,0x59414c50u,&game_schema)||!template_db||!nflstreet_team_player_resolver_open(data[0],sizes[0],data[1],sizes[1],&resolver)||
    !nflstreet_archive_open(data[2],sizes[2],&textures)||!nflstreet_archive_open(data[5],sizes[5],&dynamic)||
    !nflstreet_gear_texture_catalog_open(data[3],sizes[3],&catalog)||!nflstreet_slot_material_catalog_open(data[4],sizes[4],&materials)||
    !nflstreet_team_table_open_tag(template_db,template_size,0x52414547u,&gear_table)||
    !nflstreet_team_table_open_tag(template_db,template_size,0x59414c50u,&stat_play)||
    !nflstreet_team_table_open_tag(template_db,template_size,0x54484344u,&thcd)||
    !nflstreet_team_table_open_tag(template_db,template_size,0x4d414554u,&teams)||
    !nflstreet_team_table_open_tag(template_db,template_size,0x4c434750u,&player_colors)||
    !nflstreet_gear_records_build(&gear_table,&catalog,&records)||!nflstreet_color_table_build(template_db,template_size,&catalog,&color_table)||
    !nflstreet_dynclut_table_open(&dynamic,&catalog,&dynclut))return 3;
 nflstreet_selected_player_visual_context c={&resolver,&owned_bodies,&gear,&local_hair,&dynamic,&records,0,0x10000000u,0x50000000u,
  {&catalog,&materials,&textures,&dynclut,&color_table,0x60000000u,0x62000000u,0x63000000u,0x10000u,0xa5,upload,NULL}};

 size_t capture_size;uint8_t *capture=input(argv[14],&capture_size);
 if(!capture||capture_size!=192||memcmp(capture,"QMS1",4)||rd32(capture+4)!=1)return 3;
 nflstreet_quick_match_selection selection={0};unsigned at=8;
 for(unsigned i=0;i<2;++i,at+=4)selection.team_ids[i]=rd32(capture+at);
 for(unsigned u=0;u<2;++u)for(unsigned p=0;p<7;++p,at+=4)selection.selected_players[u][p]=rd32(capture+at);
 for(unsigned u=0;u<2;++u)for(unsigned p=0;p<14;++p,at+=4)selection.formation_players[u][p]=rd32(capture+at);
 selection.field_slot=rd32(capture+at);at+=4;uint32_t key=rd32(capture+at);if(key>255)return 3;selection.field_key=(uint8_t)key;
 nflstreet_quick_match_session session={0};
 if(!nflstreet_quick_match_session_open(template_db,template_size,schema,schema_size,&selection,&session))return 4;
 free(schema);free(template_db);schema=NULL;template_db=NULL;
 nflstreet_match_visual_input inputs[14];for(unsigned p=0;p<14;++p){uint32_t base=0x80000000u+p*0x04000000u;
  inputs[p]=(nflstreet_match_visual_input){c,base,0x04000000u,p/7};
  inputs[p].visual.face_identity=base;inputs[p].visual.hair_identity=base+0x01000000u;
  inputs[p].visual.surface.destination_token=base+0x02000000u;inputs[p].visual.surface.scratch_token=base+0x03000000u;
  inputs[p].visual.surface.material_token=base+0x03800000u;
 }
 nflstreet_match_visuals *out=calloc(1,sizeof *out),*saved=calloc(1,sizeof *saved);if(!out||!saved)return 4;
 if(!nflstreet_match_visuals_open(&session,inputs,out)||out->count!=14)return 5;
 unsigned uploaded=uploads,slots=0,instances=0;
 for(unsigned p=0;p<14;++p){nflstreet_selected_player_visual expected={0};const nflstreet_match_visual_input *i=inputs+p;
  if(!nflstreet_selected_player_visual_from_match(session.stat_bytes,session.stat_size,&session.roster.game,
     session.roster.selected_rows[p/7][p%7],i->record_side,&i->visual,&expected)||
     !equal_geometry(&out->players[p].geometry,&expected.geometry)||
     out->players[p].surface.textures.block_size!=expected.surface.textures.block_size||
     memcmp(out->players[p].surface.textures.block,expected.surface.textures.block,expected.surface.textures.block_size)||
     memcmp(out->players[p].surface.instances,expected.surface.instances,expected.surface.instance_count*sizeof *expected.surface.instances))return 6;
  if(out->players[p].surface.allocation_count!=expected.surface.allocation_count)return 6;
  for(unsigned slot=0;slot<25;++slot){const nflstreet_slot_material *actual=out->players[p].surface.bindings+slot,*want=expected.surface.bindings+slot;
   if(strcmp(actual->material,want->material)||actual->entries!=want->entries||actual->set!=want->set||actual->destination!=want->destination)return 6;
   const nflstreet_material_block *a[]={&actual->texture_set.object,&actual->texture_set.entries,&actual->texture_set.textures,&actual->texture_set.palettes,&actual->texture_set.names};
   const nflstreet_material_block *b[]={&want->texture_set.object,&want->texture_set.entries,&want->texture_set.textures,&want->texture_set.palettes,&want->texture_set.names};
   for(unsigned part=0;part<5;++part)if(a[part]->token!=b[part]->token||a[part]->size!=b[part]->size||
      (a[part]->size&&memcmp(a[part]->bytes,b[part]->bytes,b[part]->size)))return 6;
  }
  for(unsigned a=0;a<expected.surface.allocation_count;++a){const nflstreet_material_block *actual=out->players[p].surface.allocations+a,*want=expected.surface.allocations+a;
   if(actual->token!=want->token||actual->size!=want->size||memcmp(actual->bytes,want->bytes,want->size))return 6;}
  slots+=25;instances+=out->players[p].surface.instance_count;nflstreet_selected_player_visual_close(&expected);
 }
 size_t startup_size;uint8_t *startup_bytes=input(argv[9],&startup_size);nflstreet_skeleton_startup_catalog startup_catalog;
 if(!startup_bytes||!nflstreet_skeleton_startup_catalog_open(startup_bytes,startup_size,&startup_catalog))return 8;
 nflstreet_match_skeleton_input skeleton_inputs[14];for(unsigned p=0;p<14;++p){uint32_t base=0x40000000u+p*0x01000000u;
  skeleton_inputs[p]=(nflstreet_match_skeleton_input){base,0x01000000u,base,base+0x00100000u,base+0x00100100u,base+0x00200000u,0x00100000u,0,5};}
 nflstreet_match_skeletons bound={0},empty={0};nflstreet_match_skeleton_input bad_skeletons[14];
 uint32_t original_hash=hash_geometry(out);*saved=*out;
 for(unsigned fault=0;fault<4;++fault){memcpy(bad_skeletons,skeleton_inputs,sizeof bad_skeletons);
  if(fault==0)bad_skeletons[13]=bad_skeletons[0];
  if(fault==1)bad_skeletons[13].temporary_capacity=1;
  if(fault==2)bad_skeletons[13].face_target=6;
  if(fault==3)bad_skeletons[13].skeleton_token=bad_skeletons[13].namespace_base+bad_skeletons[13].namespace_size-32u;
  if(nflstreet_match_skeletons_bind(out,&owned_bodies,&animations,&startup_catalog,bad_skeletons,&bound)||
     memcmp(&bound,&empty,sizeof bound)||memcmp(out,saved,sizeof *out)||hash_geometry(out)!=original_hash)return 8;
 }
 if(!nflstreet_match_skeletons_bind(out,&owned_bodies,&animations,&startup_catalog,skeleton_inputs,&bound)||bound.count!=14||
    !bound.temporary_allocations||bound.temporary_allocations!=bound.temporary_releases)return 8;
 unsigned temporary_allocations=bound.temporary_allocations;
 for(unsigned p=0;p<14;++p){nflstreet_selected_player_visual expected={0};nflstreet_skeleton_startup skeleton={0};temporary t={0};
  const nflstreet_match_skeleton_input *i=skeleton_inputs+p;
  if(!nflstreet_selected_player_visual_from_match(session.stat_bytes,session.stat_size,&session.roster.game,
      session.roster.selected_rows[p/7][p%7],inputs[p].record_side,&inputs[p].visual,&expected)||
     !nflstreet_skeleton_startup_open(&animations,&startup_catalog,i->skeleton_token,&skeleton)||
     !nflstreet_face_resource_pair_bind(&expected.geometry.base.face,skeleton.resources+i->face_target,i->face_view_token,allocate_asset,release_asset,&t)||
     !nflstreet_hair_resources_bind(&expected.geometry.hair,skeleton.resources+i->hair_target,i->hair_view_token,allocate_asset,release_asset,&t)||t.count!=t.frees||
     !equal_geometry(&out->players[p].geometry,&expected.geometry))return 9;
  for(unsigned k=0;k<6;++k)if(bound.players[p].resources[k].size!=skeleton.resources[k].size||
      bound.players[p].resources[k].token!=skeleton.resources[k].token||
      memcmp(bound.players[p].resources[k].bytes,skeleton.resources[k].bytes,skeleton.resources[k].size))return 9;
  if(memcmp(bound.players[p].base_vectors,skeleton.base_vectors,sizeof skeleton.base_vectors)||
     bound.face_view_tokens[p]!=i->face_view_token||bound.hair_view_tokens[p]!=i->hair_view_token)return 9;
  nflstreet_selected_player_visual_close(&expected);nflstreet_skeleton_startup_close(&skeleton);
 }
 nflstreet_match_skeletons bound_before=bound;*saved=*out;original_hash=hash_geometry(out);
 if(nflstreet_match_skeletons_bind(out,&owned_bodies,&animations,&startup_catalog,skeleton_inputs,&bound)||
    memcmp(&bound,&bound_before,sizeof bound)||memcmp(out,saved,sizeof *out)||hash_geometry(out)!=original_hash)return 9;
 free(startup_bytes);
 *saved=*out;unsigned prior=uploads;if(nflstreet_match_visuals_open(&session,inputs,out)||memcmp(out,saved,sizeof *out)||uploads!=prior)return 7;
 nflstreet_match_visuals_close(out);nflstreet_match_visuals_close(out);nflstreet_match_skeletons_close(&bound);nflstreet_match_skeletons_close(&bound);memset(saved,0,sizeof *saved);
 nflstreet_match_visual_input bad[14];memcpy(bad,inputs,sizeof bad);bad[13]=bad[0];
 prior=uploads;if(nflstreet_match_visuals_open(&session,bad,out)||memcmp(out,saved,sizeof *out)||uploads!=prior)return 7;
 memcpy(bad,inputs,sizeof bad);bad[13].visual.face_identity=inputs[0].visual.face_identity;
 if(nflstreet_match_visuals_open(&session,bad,out)||memcmp(out,saved,sizeof *out)||uploads!=prior)return 7;
 memcpy(bad,inputs,sizeof bad);bad[13].visual.face_identity=bad[13].namespace_base+bad[13].namespace_size-32u;
 if(nflstreet_match_visuals_open(&session,bad,out)||memcmp(out,saved,sizeof *out)||uploads!=prior)return 7;
 memcpy(bad,inputs,sizeof bad);bad[13].visual.resolver=NULL;
 if(nflstreet_match_visuals_open(&session,bad,out)||memcmp(out,saved,sizeof *out)||uploads!=prior)return 7;
 memcpy(bad,inputs,sizeof bad);bad[13].visual.surface.flush=rejected_upload;
 if(nflstreet_match_visuals_open(&session,bad,out)||memcmp(out,saved,sizeof *out)||uploads<=prior)return 7;
 nflstreet_match_visuals_close(out);free(out);free(saved);nflstreet_quick_match_session_close(&session);
 nflstreet_dynclut_table_close(&dynclut);free(capture);for(unsigned i=0;i<6;++i)free(data[i]);close_assets();
 printf("Selected match skeletons: 14 players, 84 fresh resources, 28 face views and 42 hair views; %u temporary allocations released; 5 transactional guards; matches independent binding stages; body skinning and actors remain separate.\n",temporary_allocations);
 printf("Selected match visuals: 14 simultaneous players, %u owned slots, %u material instances; %u deferred flushes; source menu buffers released; standalone owners byte-equal. 6 ownership/failure guards; no actors or drawing.\n",slots,instances,uploaded);return 0;
}
