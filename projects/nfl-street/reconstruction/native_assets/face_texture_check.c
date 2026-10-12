#include "face_texture.h"
#include "../menus/team_player_resolve.h"
#include "../menus/team_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *load_data(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");if(!f||fseek(f,0,SEEK_END))return NULL;long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET))return NULL;
 uint8_t *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n)return NULL;fclose(f);*size=(size_t)n;return b;
}
int main(int argc,char **argv){
#ifdef NFLSTREET_FACE_TEXTURE_ORACLE
 if(argc!=7||!initialize_texture_oracle(argv[5],argv[6]))return 2;
#elif defined(NFLSTREET_SELECTED_ASSETS)
 if(argc!=10||!initialize_assets(argv))return 2;
#else
 if(argc!=5)return 2;
#endif
 size_t fn=0,hn=0,tn=0;uint8_t *f=load_data(argv[1],&fn),*h=load_data(argv[2],&hn),*t=load_data(argv[3],&tn);
 nflstreet_team_player_resolver resolver;nflstreet_archive textures;
 if(!f||!h||!t||!nflstreet_team_player_resolver_open(f,fn,h,hn,&resolver)||!nflstreet_archive_open(t,tn,&textures))return 3;
 unsigned rows=0,fallbacks=0;uint64_t pixels=0;
#ifdef NFLSTREET_FACE_TEXTURE_ORACLE
 if(!install_texture_hashes(&textures))return 3;
#endif
 for(unsigned entry=1;entry<=32;++entry){char path[4096];size_t size=0;
  if(snprintf(path,sizeof path,"%s/db_teams/%05u.bin",argv[4],entry)>=(int)sizeof path)return 4;
  uint8_t *db=load_data(path,&size);nflstreet_team_table table;
  if(!db||!nflstreet_team_table_open_tag(db,size,0x59414c50u,&table))return 4;
  unsigned ti=UINT32_MAX,pi=UINT32_MAX;for(unsigned i=0;i<table.field_count;++i){nflstreet_team_field field;if(!nflstreet_team_table_field(&table,i,&field))return 4;
   if(field.tag==0x44494754u)ti=i;if(field.tag==0x44494750u)pi=i;}
  if(ti==UINT32_MAX||pi==UINT32_MAX)return 4;
  for(unsigned i=0;i<table.used;++i){uint32_t team,player;
   if(!nflstreet_team_table_number(&table,i,ti,&team)||!nflstreet_team_table_number(&table,i,pi,&player))return 4;if(!team)continue;
   nflstreet_team_player_manifest manifest;nflstreet_face_texture_resource resource={0};
   if(!nflstreet_team_player_resolve(&resolver,db,size,team,player,&manifest)||!manifest.custom_name_branch||
      !nflstreet_face_texture_resource_open(&textures,manifest.normalized_name,0x81000000u,&resource))return 5;
#ifdef NFLSTREET_FACE_TEXTURE_ORACLE
   if(!original_face_texture(manifest.normalized_name,&resource))return 6;
#endif
   #ifdef NFLSTREET_SELECTED_ASSETS
   if(!check_joined_assets(&resolver,&textures,db,size,team,player))return 9;
#endif
   #ifdef NFLSTREET_FACE_APPLY
   if(!check_texture_application(&textures,&resource))return 10;
#endif
   ++rows;fallbacks+=resource.plan.fallback;pixels+=(uint64_t)resource.inspection_plane.width*resource.inspection_plane.height;
   nflstreet_face_texture_resource_close(&resource);
  }free(db);
 }
 printf("Selected face textures: %u roster rows, %u template fallbacks, %llu inspection pixels; owned raw/processed MMAP, no original draw claim.\n",rows,fallbacks,(unsigned long long)pixels);
#ifdef NFLSTREET_FACE_TEXTURE_ORACLE
 if(!original_texture_guards(&textures))return 7;
 printf("Original face texture checks: %u selections/cache cases, %u whole MMAP relocations/repeats; actual HSH1 lookup, synchronization services supplied.\n",texture_cases,texture_models);free(texture_ram);
#endif
 #ifdef NFLSTREET_SELECTED_ASSETS
 printf("Joined selected assets: %u geometry/face-texture assemblies, %u texture namespace guards; prepared skeleton binding and request writes, no runtime DOL/module/snapshot.\n",asset_assemblies,asset_guards);close_assets();
#ifdef NFLSTREET_APPLIED_ASSETS
 printf("Joined face application: %u owned destinations, %u ordered flush requests, %u replacement guards.\n",applied_assets,applied_flushes,application_guards);
#endif
#endif
 #ifdef NFLSTREET_FACE_APPLY
 printf("Face texture application: %u whole destination files/reapplies, %u ordered flush requests; original GPU effects excluded.\n",application_cases,application_flushes);
#endif
 free(f);free(h);free(t);return rows==402?0:8;
}
