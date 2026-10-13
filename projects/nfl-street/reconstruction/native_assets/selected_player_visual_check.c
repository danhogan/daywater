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
int main(int argc,char **argv){
 if(argc!=14||!initialize_assets(argv))return 2;
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
 unsigned rows=0,builds=0,guards=0,fallbacks=0;
 uint32_t context_teams[2]={0};unsigned team_field=UINT32_MAX;
 for(unsigned i=0;i<teams.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&teams,i,&f))return 4;if(f.tag==0x44494754u)team_field=i;}
 if(!nflstreet_team_table_number(&teams,0,team_field,context_teams)||!nflstreet_team_table_number(&teams,1,team_field,context_teams+1))return 4;
 nflstreet_game_roster context_match={0};uint8_t color_record[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]={0};
 if(!nflstreet_game_roster_build(&stat_play,&thcd,&teams,&game_schema,context_teams,2,NULL,0,&context_match)||
    !nflstreet_player_record_build(&context_match.table,0,&records,&teams,&player_colors,&catalog,0,color_record))return 5;
 nflstreet_game_roster_close(&context_match);
 for(unsigned entry=1;entry<=32;++entry){char path[4096];size_t size;
  if(snprintf(path,sizeof path,"%s/db_teams/%05u.bin",argv[4],entry)>=(int)sizeof path)return 4;
  uint8_t *db=input(path,&size);nflstreet_team_table play;if(!db||!nflstreet_team_table_open_tag(db,size,0x59414c50u,&play))return 4;
  unsigned ti=UINT32_MAX,pi=UINT32_MAX;for(unsigned i=0;i<play.field_count;++i){nflstreet_team_field f;
   if(!nflstreet_team_table_field(&play,i,&f))return 4;if(f.tag==0x44494754u)ti=i;if(f.tag==0x44494750u)pi=i;}
  for(unsigned row=0;row<play.used;++row){uint32_t team,player;uint8_t record[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]={0};
   if(!nflstreet_team_table_number(&play,row,ti,&team)||!nflstreet_team_table_number(&play,row,pi,&player))return 4;if(!team)continue;
   /* Controlled color context built wholly natively; selected gear fields
      come from this disc roster row. This does not assert selected team colors. */
   memcpy(record,color_record,sizeof record);
   if(!nflstreet_gear_player_record_from_play(&play,row,&records,record))return 5;
   for(unsigned variant=0;variant<3;++variant){c.body_variant=variant;nflstreet_selected_player_visual v={0};
    if(!nflstreet_selected_player_visual_open(db,size,team,player,record,&c,&v))return 6;
    nflstreet_selected_player_geometry g={0};nflstreet_player_surface surface={0};
    if(!nflstreet_selected_player_geometry_open(c.resolver,db,size,team,player,c.bodies,variant,c.headwear,c.local_hair,c.face_identity,c.hair_identity,&g)||
       !equal_geometry(&v.geometry,&g)||!nflstreet_player_surface_from_player(&c.surface,&records,&dynamic,record,g.base.manifest.normalized_name,&surface)||
       surface.textures.block_size!=v.surface.textures.block_size||memcmp(surface.textures.block,v.surface.textures.block,surface.textures.block_size)||
       surface.instance_count!=v.surface.instance_count||memcmp(surface.instances,v.surface.instances,surface.instance_count*sizeof *surface.instances))return 7;
    nflstreet_face_texture_plan face;if(!nflstreet_face_texture_plan_make(&textures,g.base.manifest.normalized_name,&face))return 7;
    fallbacks+=face.fallback;nflstreet_selected_player_geometry_close(&g);nflstreet_player_surface_close(&surface);
    if(!builds){uint32_t collision[]={c.face_identity,c.hair_identity,c.bodies->tokens[0]};
     for(unsigned span=0;span<3;++span)for(unsigned resource=0;resource<3;++resource){
      nflstreet_selected_player_visual_context bad=c;nflstreet_selected_player_visual out={0};out.geometry.base.manifest.player_id=123;
      nflstreet_selected_player_visual before=out;unsigned prior=uploads;
      if(span==0)bad.surface.destination_token=collision[resource];if(span==1)bad.surface.scratch_token=collision[resource];if(span==2)bad.surface.material_token=collision[resource];
      if(nflstreet_selected_player_visual_open(db,size,team,player,record,&bad,&out)||memcmp(&out,&before,sizeof out)||uploads!=prior)return 8;++guards;
     }
     nflstreet_selected_player_visual out={0},before=out;nflstreet_selected_player_visual_context bad=c;bad.surface.flush=rejected_upload;
     if(nflstreet_selected_player_visual_open(db,size,team,player,record,&bad,&out)||memcmp(&out,&before,sizeof out))return 8;++guards;
    }
    nflstreet_selected_player_visual before=v;if(nflstreet_selected_player_visual_open(db,size,team,player,record,&c,&v)||memcmp(&v,&before,sizeof v))return 8;
    nflstreet_selected_player_visual_close(&v);nflstreet_selected_player_visual_close(&v);++builds;
   }++rows;
  }free(db);
 }
 nflstreet_dynclut_table_close(&dynclut);free(schema);free(template_db);for(unsigned i=0;i<6;++i)free(data[i]);close_assets();
 printf("Selected player visuals: %u roster rows x3 bodies = %u geometry/surface assemblies; %u named-face fallbacks; %u pre-upload collision/failure guards; occupied outputs reject and close repeats. Controlled native color context + disc gear fields, no runtime DOL/module/snapshot.\n",rows,builds,fallbacks,guards);return 0;
}
