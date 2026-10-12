/* Selected gear texture plans for every regular-roster PLAY row in both
   captured team contexts, the captured player records themselves, and
   controlled variants of those records. Variants set bytes +00/+01/+08/+09/+0A
   (8014F60C fields; +08/+0A also have original setters 800DE97C/800DE964) so
   800DDB10/800DDD2C/800DDBE8 branches reach 800E2E48; they are not roster
   claims. The snapshot is fixture input only; native planning reads no RAM. */
#include "gear_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PLAYER_RECORDS 0x802aaa60u
#define CAPTURED_PLAYERS 14u
static uint8_t *load_data(const char *path,size_t *size){FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return b;}
static int load_captured(const char *path,uint8_t out[CAPTURED_PLAYERS][NFLSTREET_GEAR_PLAYER_RECORD_BYTES]){
 FILE *f=fopen(path,"rb");if(!f)return 0;
 int ok=!fseek(f,(long)(PLAYER_RECORDS-0x80000000u),SEEK_SET)&&fread(out,NFLSTREET_GEAR_PLAYER_RECORD_BYTES,CAPTURED_PLAYERS,f)==CAPTURED_PLAYERS;
 fclose(f);return ok;
}
static uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
typedef struct {unsigned plans,assigned,palettes,fallbacks,colors,record_65,extra[3];uint8_t gear_used[NFLSTREET_GEAR_RECORDS_MAX],slot_used[NFLSTREET_GEAR_TEXTURE_SLOTS];uint32_t digest;} plan_stats;
static void account(plan_stats *s,const nflstreet_gear_texture_plan *p){
 ++s->plans;s->fallbacks+=p->fallback;s->colors+=p->color_count;s->record_65+=p->record_65;
 for(unsigned i=0;i<NFLSTREET_GEAR_TEXTURE_SLOTS;++i){if(p->indices[i]!=-1){++s->assigned;s->slot_used[i]=1;}if(p->palettes[i]!=UINT32_MAX)++s->palettes;}
 for(unsigned i=0;i<3;++i)s->extra[i]+=p->indices[16+i]!=-1;
 for(unsigned i=0;i<12;++i)s->gear_used[p->gear_rows[i]]=1;
 s->digest=fnv(s->digest,p->indices,sizeof p->indices);s->digest=fnv(s->digest,p->palettes,sizeof p->palettes);
 s->digest=fnv(s->digest,&p->fallback_index,sizeof p->fallback_index);s->digest=fnv(s->digest,&p->record_65,1);
 s->digest=fnv(s->digest,p->colors,p->color_count*sizeof p->colors[0]);
}
static nflstreet_gear_records records;
int main(int argc,char **argv){
#ifdef NFLSTREET_GEAR_ORACLE
 if(argc!=8)return 2;
#else
 if(argc!=7)return 2;
#endif
 size_t sizes[4];uint8_t *data[4];for(unsigned i=0;i<4;++i)if(!(data[i]=load_data(argv[1+i],sizes+i)))return 3;
 nflstreet_archive textures,dynamic;nflstreet_gear_texture_catalog catalog;nflstreet_team_table gear;
 uint8_t captured[CAPTURED_PLAYERS][NFLSTREET_GEAR_PLAYER_RECORD_BYTES];
 if(!nflstreet_archive_open(data[0],sizes[0],&textures)||!nflstreet_archive_open(data[1],sizes[1],&dynamic)||
    !nflstreet_gear_texture_catalog_open(data[2],sizes[2],&catalog)||!nflstreet_team_table_open_tag(data[3],sizes[3],0x52414547u,&gear)||
    !nflstreet_gear_records_build(&gear,&catalog,&records)||!load_captured(argv[6],captured))return 3;
#ifdef NFLSTREET_GEAR_ORACLE
 if(!initialize_gear_oracle(argv[7],argv[6])||!install_gear_hashes(&textures,&dynamic)||!original_sweeps())return 4;
 for(unsigned row=0;row<gear.used;++row)if(!original_gear_record(&gear,row,&catalog))return 4;
#endif
 /* Team/context bytes come from one captured record per side (+64 = 0 and 1). */
 const unsigned context_source[2]={0,7};if(captured[0][0x64]!=0||captured[7][0x64]!=1)return 5;
 plan_stats stats={0};stats.digest=2166136261u;unsigned rows=0;
 for(unsigned entry=1;entry<=32;++entry){char path[4096];size_t n=0;
  if(snprintf(path,sizeof path,"%s/db_teams/%05u.bin",argv[5],entry)>=(int)sizeof path)return 5;uint8_t *db=load_data(path,&n);nflstreet_team_table play;
  if(!db||!nflstreet_team_table_open_tag(db,n,0x59414c50u,&play))return 5;unsigned team_field=UINT32_MAX;
  for(unsigned i=0;i<play.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&play,i,&f))return 5;if(f.tag==0x44494754u)team_field=i;}
  for(unsigned row=0;row<play.used;++row){uint32_t team;if(!nflstreet_team_table_number(&play,row,team_field,&team))return 5;if(!team)continue;
   for(unsigned context=0;context<2;++context){uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];nflstreet_gear_texture_plan plan;
    memcpy(player,captured[context_source[context]],sizeof player);
    if(!nflstreet_gear_player_record_from_play(&play,row,&records,player)){fprintf(stderr,"player record failed block%u row%u\n",entry,row);return 6;}
#ifdef NFLSTREET_GEAR_ORACLE
    if(!context&&!original_player_fields(&play,row,&gear,&records,player))return 7;
#endif
    if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,player,&plan)){fprintf(stderr,"plan failed block%u row%u\n",entry,row);return 6;}
#ifdef NFLSTREET_GEAR_ORACLE
    if(!original_plan(&gear,&records,player,rows%CAPTURED_PLAYERS,&plan))return 7;
#endif
    account(&stats,&plan);
   }
   ++rows;
  }
  free(db);
 }
 unsigned captured_plans=0;
 for(unsigned i=0;i<CAPTURED_PLAYERS;++i){nflstreet_gear_texture_plan plan;
  if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,captured[i],&plan)){fprintf(stderr,"captured plan failed %u\n",i);return 6;}
#ifdef NFLSTREET_GEAR_ORACLE
  if(!original_plan(&gear,&records,captured[i],i,&plan))return 7;
#endif
  account(&stats,&plan);++captured_plans;
 }
 unsigned variants=0;
 for(unsigned i=0;i<CAPTURED_PLAYERS;++i)for(unsigned k=0;k<8;++k){uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];nflstreet_gear_texture_plan plan;
  memcpy(player,captured[i],sizeof player);player[0]=(uint8_t)(k&3u);player[1]=(uint8_t)(k>>1);
  player[8]=(uint8_t)k;player[9]=(uint8_t)((k+3u)&7u);player[0x0a]=(uint8_t)((k+5u)&7u);
  if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,player,&plan)){fprintf(stderr,"variant plan failed %u/%u\n",i,k);return 6;}
#ifdef NFLSTREET_GEAR_ORACLE
  if(!original_plan(&gear,&records,player,i,&plan))return 7;
#endif
  account(&stats,&plan);++variants;
 }
 unsigned gear_used=0,slots=0;for(unsigned i=0;i<NFLSTREET_GEAR_RECORDS_MAX;++i)gear_used+=stats.gear_used[i];
 for(unsigned i=0;i<NFLSTREET_GEAR_TEXTURE_SLOTS;++i)slots+=stats.slot_used[i];
 printf("Selected gear texture plans: %u roster rows x2 captured team contexts, %u captured records, %u controlled variants; %u plans, %u GEAR records (%u joined), %u slot indices across %u slots (slots16/17/18: %u/%u/%u), %u palettes, %u slot0 fallbacks, %u color requests, %u records +65 set; digest %08x.\n",
  rows,captured_plans,variants,stats.plans,records.count,gear_used,stats.assigned,slots,stats.extra[0],stats.extra[1],stats.extra[2],stats.palettes,stats.fallbacks,stats.colors,stats.record_65,stats.digest);
#ifdef NFLSTREET_GEAR_ORACLE
 printf("Original gear texture checks: %u GEAR records x2 fills, %u player-field rows (%u fields), %u selector kinds, %u fallback codes, %u complete 800DE3B8 plans (%u slot writes, %u color requests, %u queries).\n",
  gear_record_cases/2,player_rows,player_fields,kind_cases,fallback_cases,plan_cases,slot_writes,color_writes,gear_queries);
#endif
 for(unsigned i=0;i<4;++i)free(data[i]);return rows==402?0:8;
}
