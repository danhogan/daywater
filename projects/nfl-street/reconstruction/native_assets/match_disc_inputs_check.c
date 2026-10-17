#define main appearance_stage_main
#include "selected_appearance_assets_check.c"
#undef main
#include "selected_player_visual.h"
#include "player_record.h"
#include "game_roster.h"
#include "../menus/team_identity.h"
static unsigned uploads;
static int upload(void *opaque,uint32_t token,uint32_t size){(void)opaque;if(!token||!size)return 0;++uploads;return 1;}
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
#include "match_disc_inputs.h"
#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"comparison failed at line %u\n",__LINE__);return 4;}}while(0)
typedef struct {unsigned count,hash,reject;} flush_trace;
typedef struct {flush_trace *trace;unsigned player;} trace_context;
static int trace_flush(void *opaque,uint32_t token,uint32_t size){
 trace_context *c=opaque;flush_trace *t=c->trace;++t->count;t->hash=(t->hash^c->player)*16777619u;t->hash=(t->hash^token)*16777619u;t->hash=(t->hash^size)*16777619u;
 return !t->reject||t->count<t->reject;
}
static int equal_surface(const nflstreet_player_surface *a,const nflstreet_player_surface *b){
 if(a->textures.token!=b->textures.token||a->textures.scratch_capacity!=b->textures.scratch_capacity||a->textures.block_size!=b->textures.block_size||memcmp(a->textures.block,b->textures.block,a->textures.block_size)||
    a->instance_count!=b->instance_count||a->allocation_count!=b->allocation_count||a->material_used!=b->material_used)return 0;
 for(unsigned i=0;i<25;++i){const nflstreet_slot_texture *x=a->textures.slots+i,*y=b->textures.slots+i;
  if(x->token!=y->token||x->template_index!=y->template_index||x->template_size!=y->template_size||x->capacity!=y->capacity||
   x->offset!=y->offset||x->applied_index!=y->applied_index||x->applied_palette!=y->applied_palette)return 0;}
 for(unsigned i=0;i<a->instance_count;++i){const nflstreet_actor_material *x=a->instances+i,*y=b->instances+i;
  if(strcmp(x->material,y->material)||strcmp(x->texture,y->texture)||x->shader!=y->shader||x->stage_count!=y->stage_count||memcmp(x->stages,y->stages,sizeof x->stages))return 0;}
 for(unsigned i=0;i<25;++i){const nflstreet_slot_material *x=a->bindings+i,*y=b->bindings+i;
  if(strcmp(x->material,y->material)||x->entries!=y->entries||x->set!=y->set||x->destination!=y->destination)return 0;
  const nflstreet_material_block *xx[]={&x->texture_set.object,&x->texture_set.entries,&x->texture_set.textures,&x->texture_set.palettes,&x->texture_set.names};
  const nflstreet_material_block *yy[]={&y->texture_set.object,&y->texture_set.entries,&y->texture_set.textures,&y->texture_set.palettes,&y->texture_set.names};
  for(unsigned j=0;j<5;++j)if(xx[j]->token!=yy[j]->token||xx[j]->size!=yy[j]->size||(xx[j]->size&&memcmp(xx[j]->bytes,yy[j]->bytes,xx[j]->size)))return 0;}
 for(unsigned i=0;i<a->allocation_count;++i)if(a->allocations[i].token!=b->allocations[i].token||a->allocations[i].size!=b->allocations[i].size||memcmp(a->allocations[i].bytes,b->allocations[i].bytes,a->allocations[i].size))return 0;
 return 1;
}
static int equal_bound(const nflstreet_match_skeletons *a,const nflstreet_match_skeletons *b){
 if(a->count!=b->count||a->temporary_allocations!=b->temporary_allocations||a->temporary_releases!=b->temporary_releases||
    memcmp(a->face_view_tokens,b->face_view_tokens,sizeof a->face_view_tokens)||memcmp(a->hair_view_tokens,b->hair_view_tokens,sizeof a->hair_view_tokens))return 0;
 for(unsigned p=0;p<14;++p){if(memcmp(a->players[p].base_vectors,b->players[p].base_vectors,sizeof a->players[p].base_vectors))return 0;
  for(unsigned k=0;k<6;++k){const nflstreet_skeleton_resource *x=a->players[p].resources+k,*y=b->players[p].resources+k;
   if(x->size!=y->size||x->token!=y->token||x->archive_index!=y->archive_index||memcmp(x->bytes,y->bytes,x->size))return 0;}}
 return 1;
}
int main(int argc,char **argv){
 if(argc!=20||!initialize_assets(argv))return 2;
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
 REQUIRE(nflstreet_team_table_open_tag(session.stat_bytes,session.stat_size,0x52414547u,&gear_table));
 nflstreet_match_visual_input inputs[14];for(unsigned p=0;p<14;++p){uint32_t base=0x80000000u+p*0x04000000u;
  inputs[p]=(nflstreet_match_visual_input){c,base,0x04000000u,p/7};
  inputs[p].visual.face_identity=base;inputs[p].visual.hair_identity=base+0x01000000u;
  inputs[p].visual.surface.destination_token=base+0x02000000u;inputs[p].visual.surface.scratch_token=base+0x03000000u;
  inputs[p].visual.surface.material_token=base+0x03800000u;
 }

 nflstreet_match_skeleton_input binding[14];for(unsigned p=0;p<14;++p){uint32_t base=0x40000000u+p*0x01000000u;
  binding[p]=(nflstreet_match_skeleton_input){base,0x01000000u,base,base+0x00100000u,base+0x00100100u,base+0x00200000u,0x00100000u,0,5};}
 size_t startup_size;uint8_t *startup_bytes=input(argv[9],&startup_size);nflstreet_skeleton_startup_catalog startup_catalog;
 REQUIRE(startup_bytes&&nflstreet_skeleton_startup_catalog_open(startup_bytes,startup_size,&startup_catalog));
 size_t ram_size;uint8_t *ram=input(argv[16],&ram_size);REQUIRE(ram&&ram_size==24u*1024u*1024u);
 unsigned context=(unsigned)strtoul(argv[17],NULL,10),variants=(unsigned)strtoul(argv[18],NULL,10),guards=(unsigned)strtoul(argv[19],NULL,10);
 unsigned fill=ram[(context*4096u)%ram_size]^context;free(ram);
 unsigned queries=0,cheap=0,digest=2166136261u;
 for(unsigned v=0;v<variants;++v){
  flush_trace actual={0,2166136261u,0},reference={0,2166136261u,0};
  nflstreet_match_visual_input expected_inputs[14];trace_context ac[14],rc[14];
  for(unsigned p=0;p<14;++p){inputs[p].visual.body_variant=v%3;inputs[p].visual.surface.allocator_fill=(uint8_t)(fill+v);
   inputs[p].visual.surface.flush=trace_flush;ac[p]=(trace_context){&actual,p};rc[p]=(trace_context){&reference,p};inputs[p].visual.surface.flush_opaque=ac+p;
   expected_inputs[p]=inputs[p];expected_inputs[p].visual.surface.flush_opaque=rc+p;}
  nflstreet_match_visuals *want=calloc(1,sizeof *want);nflstreet_match_skeletons *want_bound=calloc(1,sizeof *want_bound);REQUIRE(want&&want_bound);
  REQUIRE(nflstreet_match_visuals_open(&session,expected_inputs,want));
  REQUIRE(nflstreet_match_skeletons_bind(want,&owned_bodies,&animations,&startup_catalog,binding,want_bound));
  /* None of these external catalog-derived tables may reach the adapter. */
  for(unsigned p=0;p<14;++p){inputs[p].visual.gear=NULL;inputs[p].visual.surface.gear=NULL;inputs[p].visual.surface.materials=NULL;
   inputs[p].visual.surface.colors=NULL;inputs[p].visual.surface.dynclut=NULL;}
  nflstreet_match_disc_inputs *owner=NULL;
  REQUIRE(nflstreet_match_disc_inputs_open(argv[15],&session,inputs,binding,&owner)&&owner);
  const nflstreet_match_visuals *got=nflstreet_match_disc_inputs_visuals(owner);
  const nflstreet_match_skeletons *bound=nflstreet_match_disc_inputs_skeletons(owner);
  REQUIRE(got&&bound&&got->count==14&&bound->count==14&&equal_bound(bound,want_bound));
  REQUIRE(actual.count==reference.count&&actual.hash==reference.hash);
  for(unsigned p=0;p<14;++p){REQUIRE(equal_geometry(&got->players[p].geometry,&want->players[p].geometry));
   for(unsigned h=0;h<3;++h)REQUIRE(!memcmp(got->players[p].geometry.hair.views[h],want->players[p].geometry.hair.views[h],52));
   REQUIRE(equal_surface(&got->players[p].surface,&want->players[p].surface));}
  /* Long-lived SMC1 names are read again after all constructors return. */
  const nflstreet_gear_texture_catalog *gc=nflstreet_match_disc_inputs_gear(owner);
  const nflstreet_slot_material_catalog *mc=nflstreet_match_disc_inputs_materials(owner);REQUIRE(gc&&mc);
  if(v==0)for(unsigned p=0;p<14;++p)for(unsigned slot=0;slot<25;++slot){
   REQUIRE(nflstreet_gear_texture_template(gc,slot)==nflstreet_gear_texture_template(&catalog,slot));
   REQUIRE(!strcmp(nflstreet_slot_material_name(mc,slot),nflstreet_slot_material_name(&materials,slot)));
   REQUIRE(!strcmp(got->players[p].surface.bindings[slot].material,nflstreet_slot_material_name(mc,slot)));++queries;}
  if(v==0){
   for(unsigned g=0;g<guards;++g){uint8_t a[112],b[112];memset(a,(uint8_t)(fill+g),112);memcpy(b,a,112);
    unsigned row=g%gear_table.used;
    REQUIRE(nflstreet_gear_record_from_table(&gear_table,row,gc,a));
    REQUIRE(nflstreet_gear_record_from_table(&gear_table,row,&catalog,b));
    REQUIRE(!memcmp(a,b,112));++cheap;}
   nflstreet_match_disc_inputs *occupied=owner;unsigned prior=actual.count;
   REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,inputs,binding,&occupied));
   REQUIRE(occupied==owner&&actual.count==prior);
  }
  digest=(digest^hash_geometry(got))*16777619u;digest=(digest^actual.hash)*16777619u;
  nflstreet_match_disc_inputs_close(owner);nflstreet_match_visuals_close(want);nflstreet_match_skeletons_close(want_bound);free(want);free(want_bound);
  /* Restore independent reference inputs for next body variant. */
  for(unsigned p=0;p<14;++p){inputs[p].visual.gear=&records;inputs[p].visual.surface.gear=&catalog;
   inputs[p].visual.surface.materials=&materials;inputs[p].visual.surface.colors=&color_table;inputs[p].visual.surface.dynclut=&dynclut;}
 }
 flush_trace failed={0,2166136261u,0};trace_context fc[14];for(unsigned p=0;p<14;++p){fc[p]=(trace_context){&failed,p};inputs[p].visual.surface.flush=trace_flush;inputs[p].visual.surface.flush_opaque=fc+p;}
 nflstreet_match_skeleton_input bad[14];memcpy(bad,binding,sizeof bad);bad[13].temporary_capacity=1;
 nflstreet_match_disc_inputs *rejected=NULL;
 nflstreet_match_visual_input bad_visual[14];memcpy(bad_visual,inputs,sizeof bad_visual);bad_visual[13].visual.resolver=NULL;
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,bad_visual,binding,&rejected));REQUIRE(!rejected&&!failed.count);
 memcpy(bad_visual,inputs,sizeof bad_visual);bad_visual[13].namespace_base=bad_visual[0].namespace_base;
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,bad_visual,binding,&rejected));REQUIRE(!rejected&&!failed.count);
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,inputs,bad,&rejected));REQUIRE(!rejected&&!failed.count);
 failed.reject=3;
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,inputs,binding,&rejected));REQUIRE(!rejected&&failed.count==3);
 unsigned prior=failed.count;
 REQUIRE(!nflstreet_match_disc_inputs_open(NULL,&session,inputs,binding,&rejected));
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],NULL,inputs,binding,&rejected));
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,NULL,binding,&rejected));
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,inputs,NULL,&rejected));
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,inputs,binding,NULL));
 nflstreet_match_visual_input mismatch[14];memcpy(mismatch,inputs,sizeof mismatch);mismatch[13].visual.dynamic_palettes=&textures;
 REQUIRE(!nflstreet_match_disc_inputs_open(argv[15],&session,mismatch,binding,&rejected));
 REQUIRE(!rejected&&failed.count==prior);
 REQUIRE(!nflstreet_match_disc_inputs_visuals(NULL)&&!nflstreet_match_disc_inputs_skeletons(NULL)&&
   !nflstreet_match_disc_inputs_gear(NULL)&&!nflstreet_match_disc_inputs_materials(NULL));
 nflstreet_match_disc_inputs_close(NULL);
 free(startup_bytes);nflstreet_quick_match_session_close(&session);nflstreet_dynclut_table_close(&dynclut);
 free(capture);for(unsigned i=0;i<6;++i)free(data[i]);close_assets();
 printf("Match disc inputs: %u full aggregates, %u players, %u bound resources; %u catalog queries, %u varied gear records, 1 occupied-output guard, 3 late rollbacks, 1 namespace guard; digest %08x.\n",variants,variants*14,variants*84,queries,cheap,digest);
 return 0;
}
