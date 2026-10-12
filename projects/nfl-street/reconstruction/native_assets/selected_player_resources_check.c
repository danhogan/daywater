#include "selected_player_resources.h"
#include "../menus/team_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *load(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
 long n=ftell(f);if(n<16||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t *b=malloc((size_t)n);
 if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return b;
}
int main(int argc,char **argv){
 if(argc!=5)return 2;size_t fn=0,hn=0,bn=0;
 uint8_t *face=load(argv[1],&fn),*hair=load(argv[2],&hn),*body=load(argv[3],&bn);
 nflstreet_team_player_resolver resolver;nflstreet_body_resource_pool bodies={0};
 if(!face||!hair||!body||!nflstreet_team_player_resolver_open(face,fn,hair,hn,&resolver)||
  !nflstreet_body_resource_pool_open(body,bn,0x20000000u,&bodies)||!nflstreet_body_resource_pool_process(&bodies))return 3;
 unsigned rows=0,assemblies=0,models=0,guards=0;
 for(unsigned entry=1;entry<=32;++entry){
  char path[4096];size_t size=0;int n=snprintf(path,sizeof path,"%s/db_teams/%05u.bin",argv[4],entry);
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
    nflstreet_selected_player_resources saved={0};saved.manifest.player_id=0x12345678u;
    nflstreet_selected_player_resources out=saved;
    if(nflstreet_selected_player_resources_open(&resolver,db,size,team,UINT32_MAX,&bodies,0,0x10000000u,&out)||memcmp(&out,&saved,sizeof out))return 7;++guards;
    if(nflstreet_selected_player_resources_open(&resolver,db,size,team,player,&bodies,3,0x10000000u,&out)||memcmp(&out,&saved,sizeof out))return 7;++guards;
    if(nflstreet_selected_player_resources_open(&resolver,db,size,team,player,&bodies,0,0x20000000u,&out)||memcmp(&out,&saved,sizeof out))return 7;++guards;
   }
   for(unsigned variant=0;variant<3;++variant){nflstreet_selected_player_resources r={0};
    if(!nflstreet_selected_player_resources_open(&resolver,db,size,team,player,&bodies,variant,0x10000000u,&r)){
     fprintf(stderr,"Selected resources failed team%u player%u variant%u\n",team,player,variant);return 5;}
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
    ++assemblies;nflstreet_selected_player_resources_close(&r);
   }
   ++rows;
  }free(db);
 }
 nflstreet_body_resource_pool_close(&bodies);free(face);free(hair);free(body);
 if(rows!=402||assemblies!=1206||models!=2412||guards!=3)return 6;
 printf("Selected native resources: %u roster rows, %u explicit body-variant assemblies, %u processed face models, %u ownership/input guards; no snapshot/module input.\n",rows,assemblies,models,guards);
 return 0;
}
