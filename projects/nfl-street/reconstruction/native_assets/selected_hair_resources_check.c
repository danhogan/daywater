#include "selected_player_resources.h"
#include "face_postattach.h"
#include "hair_resources.h"
#ifdef NFLSTREET_AUTOMATIC_HEADWEAR
#include "headwear_state.h"
#include "selected_player_geometry.h"
#ifdef NFLSTREET_BIND_HAIR
#include "hair_postattach.h"
#ifdef NFLSTREET_INITIALIZED_SKELETONS
#include "skeleton_startup.h"
#endif
#endif
#endif
#include "../menus/team_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *load(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
 long n=ftell(f);if(n<16||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t *b=malloc((size_t)n);
 if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return b;
}
typedef struct {void *bytes[9];uint32_t tokens[9];unsigned made,freed;} temporaries;
static int allocate_temporary(void *opaque,uint32_t count,uint32_t width,nflstreet_model_postattach_segment *out){
 temporaries *t=opaque;if(t->made>=9||(uint64_t)count*width>0x10000u)return 0;
 unsigned i=t->made++;size_t size=(size_t)count*width;t->bytes[i]=malloc(size?size:1);
 if(!t->bytes[i])return 0;t->tokens[i]=0x40000000u+i*0x10000u;
 *out=(nflstreet_model_postattach_segment){t->tokens[i],t->bytes[i],size};return 1;
}
static void release_temporary(void *opaque,uint32_t token){
 temporaries *t=opaque;for(unsigned i=0;i<t->made;++i)if(t->tokens[i]==token&&t->bytes[i]){
  free(t->bytes[i]);t->bytes[i]=NULL;++t->freed;return;}
}
int main(int argc,char **argv){
#ifdef NFLSTREET_AUTOMATIC_HEADWEAR
 #ifdef NFLSTREET_AUTOMATIC_HEADWEAR_ORACLE
 if(argc!=10)return 2;
#else
 #ifdef NFLSTREET_INITIALIZED_SKELETONS
 if(argc!=9)return 2;
#else
 if(argc!=8)return 2;
#endif
#endif
 size_t gn=0;uint8_t *gear_bytes=load(argv[7],&gn);nflstreet_headwear_catalog gear_catalog;
 if(!gear_bytes||!nflstreet_headwear_catalog_open(gear_bytes,gn,&gear_catalog))return 3;
#ifdef NFLSTREET_AUTOMATIC_HEADWEAR_ORACLE
 if(!initialize_headwear_oracle(argv[8],argv[9],&gear_catalog))return 3;
#endif
#else
 #ifdef NFLSTREET_HAIR_ORACLE
 if(argc!=9||!initialize_oracle(argv[7],argv[8]))return 2;
#else
 if(argc!=7)return 2;
#endif
#endif
 size_t ln=0;uint8_t *local_bytes=load(argv[6],&ln);nflstreet_archive local;
 if(!local_bytes||!nflstreet_archive_open(local_bytes,ln,&local))return 3;size_t fn=0,hn=0,bn=0,an=0;
 uint8_t *face=load(argv[1],&fn),*hair=load(argv[2],&hn),*body=load(argv[3],&bn);
 uint8_t *anim=load(argv[4],&an);nflstreet_archive animation;nflstreet_skeleton_resource target={0};
 if(!anim||!nflstreet_archive_open(anim,an,&animation))return 3;
#ifdef NFLSTREET_INITIALIZED_SKELETONS
 size_t startup_size=0;uint8_t *startup_bytes=load(argv[8],&startup_size);
 nflstreet_skeleton_startup_catalog startup_catalog;nflstreet_skeleton_startup startup={0};
 if(!startup_bytes||!nflstreet_skeleton_startup_catalog_open(startup_bytes,startup_size,&startup_catalog)||
    !nflstreet_skeleton_startup_open(&animation,&startup_catalog,0x30000000u,&startup))return 20;
 target=startup.resources[0];
#else
 if(!nflstreet_skeleton_resource_open(&animation,95,0x30000000u,&target))return 3;
#endif
 nflstreet_team_player_resolver resolver;nflstreet_body_resource_pool bodies={0};
 if(!face||!hair||!body||!nflstreet_team_player_resolver_open(face,fn,hair,hn,&resolver)||
  !nflstreet_body_resource_pool_open(body,bn,0x20000000u,&bodies)||!nflstreet_body_resource_pool_process(&bodies))return 3;
 #ifdef NFLSTREET_BIND_HAIR
 nflstreet_skeleton_resource hair_target={0};
 #ifdef NFLSTREET_INITIALIZED_SKELETONS
 hair_target=startup.resources[5];
#else
 if(!nflstreet_skeleton_resource_open(&animation,101,0x31000000u,&hair_target))return 16;
#endif
 unsigned bound_hair_models=0;
#endif
 unsigned rows=0,assemblies=0,models=0,guards=0,hair_models=0,hair_plans=0;
 for(unsigned entry=1;entry<=32;++entry){
  char path[4096];size_t size=0;int n=snprintf(path,sizeof path,"%s/db_teams/%05u.bin",argv[5],entry);
  if(n<0||n>=(int)sizeof path)return 4;uint8_t *db=load(path,&size);nflstreet_team_table table;
  if(!db||!nflstreet_team_table_open_tag(db,size,0x59414c50u,&table))return 4;
  unsigned team_field=UINT32_MAX,player_field=UINT32_MAX;
  for(unsigned j=0;j<table.field_count;++j){nflstreet_team_field f;if(!nflstreet_team_table_field(&table,j,&f))return 4;
   if(f.tag==0x44494754u)team_field=j;if(f.tag==0x44494750u)player_field=j;}
  if(team_field==UINT32_MAX||player_field==UINT32_MAX)return 4;
  for(unsigned row=0;row<table.used;++row){uint32_t team,player;
   if(!nflstreet_team_table_number(&table,row,team_field,&team)||!nflstreet_team_table_number(&table,row,player_field,&player))return 4;
   if(!team)continue;
   if(!rows){
#ifdef NFLSTREET_AUTOMATIC_HEADWEAR
    nflstreet_selected_player_geometry marker={0},rejected;
    marker.base.manifest.player_id=0x12345678u;
    for(unsigned collision=0;collision<2;++collision){rejected=marker;
      if(nflstreet_selected_player_geometry_open(&resolver,db,size,team,player,&bodies,0,
          &gear_catalog,&local,0x10000000u,collision?0x20000000u:0x10000000u,&rejected)||
          memcmp(&rejected,&marker,sizeof marker))return 15;}
#endif
    nflstreet_selected_player_resources saved={0};saved.manifest.player_id=0x12345678u;
    nflstreet_selected_player_resources out=saved;
    if(nflstreet_selected_player_resources_open(&resolver,db,size,team,UINT32_MAX,&bodies,0,0x10000000u,&out)||memcmp(&out,&saved,sizeof out))return 7;++guards;
    if(nflstreet_selected_player_resources_open(&resolver,db,size,team,player,&bodies,3,0x10000000u,&out)||memcmp(&out,&saved,sizeof out))return 7;++guards;
    if(nflstreet_selected_player_resources_open(&resolver,db,size,team,player,&bodies,0,0x20000000u,&out)||memcmp(&out,&saved,sizeof out))return 7;++guards;
   }
   for(unsigned variant=0;variant<3;++variant){nflstreet_selected_player_resources r={0};
#ifdef NFLSTREET_AUTOMATIC_HEADWEAR
    nflstreet_selected_player_geometry joined={0};
    if(!nflstreet_selected_player_geometry_open(&resolver,db,size,team,player,&bodies,variant,
        &gear_catalog,&local,0x10000000u,0x50000000u,&joined))return 14;
    r=joined.base;
#ifdef NFLSTREET_BIND_HAIR
 temporaries hair_temporary={0};
 if(!nflstreet_hair_resources_bind(&joined.hair,&hair_target,0x19000000u,allocate_temporary,release_temporary,&hair_temporary)||
    hair_temporary.made!=hair_temporary.freed)return 17;
 unsigned allocations_before=hair_temporary.made;
 if(!nflstreet_hair_resources_bind(&joined.hair,&hair_target,0x19000000u,allocate_temporary,release_temporary,&hair_temporary)||
    hair_temporary.made!=allocations_before||hair_temporary.freed!=allocations_before)return 18;
 for(unsigned slot=0;slot<3;++slot)if(joined.hair.bytes[slot])++bound_hair_models;
#ifdef NFLSTREET_AUTOMATIC_HEADWEAR_ORACLE
 if(!variant&&!original_hair_binding(&local,&resolver.hair,&animation,&joined.hair_plan))return 19;
#endif
#endif
#else
    if(!nflstreet_selected_player_resources_open(&resolver,db,size,team,player,&bodies,variant,0x10000000u,&r)){
     fprintf(stderr,"Selected resources failed team%u player%u variant%u\n",team,player,variant);return 5;}
#endif
    if(r.manifest.play_row!=row||r.manifest.team_id!=team||r.manifest.player_id!=player||r.body_variant!=variant||
     r.face.indices[0]!=(unsigned)r.manifest.head_runtime_index||r.face.indices[1]!=r.face.indices[0]+1||
     r.body->archive_index!=1+r.body->pool_key*3+variant)return 5;
    for(unsigned j=0;j<2;++j){
     nflstreet_model_view_source source={r.face.bytes[j],r.face.sizes[j],r.face.tokens[j]};
     nflstreet_model_view_chunk info,geom;
     if(r.face.bytes[j][15]!=1||nflstreet_model_view_find(&source,0x496e666fu,&info)!=1||
      nflstreet_model_view_find(&source,0x47656f6du,&geom)!=1||!info.size||geom.size<24)return 5;
     ++models;
    }
    temporaries temporary={0};
    if(!nflstreet_face_resource_pair_bind(&r.face,&target,0x18000000u,allocate_temporary,release_temporary,&temporary)||
       temporary.made!=6||temporary.freed!=6||
       !nflstreet_face_resource_pair_bind(&r.face,&target,0x18000000u,allocate_temporary,release_temporary,&temporary)||
       temporary.made!=6||temporary.freed!=6)return 8;
    ++assemblies;
    #ifdef NFLSTREET_AUTOMATIC_HEADWEAR
    nflstreet_headwear_selection selection=joined.headwear;
    if(selection.play_row!=row)return 12;
    #ifdef NFLSTREET_AUTOMATIC_HEADWEAR_ORACLE
    if(!variant&&!original_selection(db,size,&selection))return 13;
#endif
    if(!variant){unsigned state=selection.state;
#else
    if(!variant)for(unsigned state=0;state<2;++state){
#endif
      nflstreet_hair_plan plan;nflstreet_hair_resources hair_resources={0};
      if(!nflstreet_hair_plan_make(&resolver.hair,r.manifest.normalized_name,(uint16_t)r.manifest.plht,
          (uint16_t)(r.manifest.phar==63?12:r.manifest.phar),(uint16_t)state,&plan)||
         !nflstreet_hair_resources_open(&local,&resolver.hair,&plan,0x50000000u,&hair_resources)){
        fprintf(stderr,"hair failed team%u player%u state%u indices%u,%u,%u\n",team,player,state,plan.indices[0],plan.indices[1],plan.indices[2]);return 9;}
      #ifdef NFLSTREET_HAIR_ORACLE
      if(!original_plan(&resolver.hair,r.manifest.normalized_name,(uint16_t)r.manifest.plht,
           (uint16_t)(r.manifest.phar==63?12:r.manifest.phar),(uint16_t)state,&plan)||
         !original_resources(&local,&resolver.hair,&plan))return 10;
      if(!state)for(unsigned special=11;special<=12;++special){nflstreet_hair_plan extra;
        if(!nflstreet_hair_plan_make(&resolver.hair,r.manifest.normalized_name,(uint16_t)r.manifest.plht,
           (uint16_t)(r.manifest.phar==63?12:r.manifest.phar),(uint16_t)special,&extra)||
           !original_plan(&resolver.hair,r.manifest.normalized_name,(uint16_t)r.manifest.plht,
           (uint16_t)(r.manifest.phar==63?12:r.manifest.phar),(uint16_t)special,&extra))return 11;}
#endif
      ++hair_plans;for(unsigned k=0;k<3;++k)if(hair_resources.bytes[k])++hair_models;
      nflstreet_hair_resources_close(&hair_resources);
    }
    #ifdef NFLSTREET_AUTOMATIC_HEADWEAR
    nflstreet_selected_player_geometry_close(&joined);
#else
    nflstreet_selected_player_resources_close(&r);
#endif
   }
   ++rows;
  }free(db);
 }
 #ifndef NFLSTREET_INITIALIZED_SKELETONS
 nflstreet_skeleton_resource_close(&target);
#endif
 nflstreet_body_resource_pool_close(&bodies);free(face);free(hair);free(body);free(anim);
 if(rows!=402||assemblies!=1206||models!=2412||guards!=3)return 6;
 printf("Selected native resources: %u roster rows, %u explicit body-variant assemblies, %u bound face models, %u ownership/input guards; fresh target and repeated binding, no snapshot/module input.\n",rows,assemblies,models,guards);
 #ifdef NFLSTREET_AUTOMATIC_HEADWEAR
 printf("Automatic hair resources: %u derived plans, %u processed models across %u roster rows; rendering remains outside scope.\n",hair_plans,hair_models,rows);
#else
 printf("Hair resources: %u explicit-state plans, %u processed models across %u roster rows; state selection and rendering remain outside scope.\n",hair_plans,hair_models,rows);
#endif
 free(local_bytes);
#ifdef NFLSTREET_AUTOMATIC_HEADWEAR
 free(gear_bytes);
#ifdef NFLSTREET_BIND_HAIR
 printf("Hair binding: %u selected models and repeated no-ops; fresh entry101 target.\n",bound_hair_models);
 #ifdef NFLSTREET_INITIALIZED_SKELETONS
 nflstreet_skeleton_startup_close(&startup);free(startup_bytes);
 printf("Selected geometry uses initialized face/hair targets from six-resource startup; no DOL/module/snapshot runtime input.\n");
#else
 nflstreet_skeleton_resource_close(&hair_target);
#endif
#endif
#ifdef NFLSTREET_AUTOMATIC_HEADWEAR_ORACLE
 printf("Headwear original checks: %u extracted fields, %u selectors, %u mapped values; fresh GEAR query services supplied.\n",field_checks,selector_checks,map_checks);free(hw_ram);
#ifdef NFLSTREET_BIND_HAIR
 printf("Original hair binding: %u unique files/views and repeats, %u allocating models, %u exact allocations/storage/free order.\n",binding_models,binding_allocating,binding_allocations);
#endif
#endif
#endif
#ifdef NFLSTREET_HAIR_ORACLE
 printf("Original hair checks: %u plans, %u unique complete models/views; archive queries supplied.\n",original_plans,original_models);free(hair_ram);
#endif
 return 0;
}
