#include "team_player_resolve.h"
#include "team_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t*load(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<16||z>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}
int main(int argc,char**argv){
 if(argc!=5)return 2;size_t fn=0,hn=0;uint8_t*f=load(argv[1],&fn),*h=load(argv[2],&hn);nflstreet_team_player_resolver resolver;
 if(!f||!h||!nflstreet_team_player_resolver_open(f,fn,h,hn,&resolver))return 3;
 unsigned rows=0,custom=0,queries=0;
 for(unsigned entry=1;entry<=32;++entry){char p[4096];size_t n=0;
  int z=snprintf(p,sizeof p,"%s/db_teams/%05u.bin",argv[3],entry);if(z<0||z>=(int)sizeof p)return 4;
  uint8_t*b=load(p,&n);nflstreet_team_table t;if(!b||!nflstreet_team_table_open_tag(b,n,0x59414c50u,&t))return 4;
  unsigned ti=UINT32_MAX,pi=UINT32_MAX;for(unsigned i=0;i<t.field_count;++i){nflstreet_team_field field;if(!nflstreet_team_table_field(&t,i,&field))return 4;if(field.tag==0x44494754u)ti=i;if(field.tag==0x44494750u)pi=i;}
  if(ti==UINT32_MAX||pi==UINT32_MAX)return 4;
  for(unsigned row=0;row<t.used;++row){uint32_t team,player;
   if(!nflstreet_team_table_number(&t,row,ti,&team)||!nflstreet_team_table_number(&t,row,pi,&player))return 5;if(!team)continue;
   nflstreet_team_player_manifest m;if(!nflstreet_team_player_resolve(&resolver,b,n,team,player,&m)||m.team_id!=team||m.player_id!=player||m.play_row!=row||m.hair_variant_resolved)return 5;
   if(m.custom_name_branch){unsigned index;nflstreet_archive_entry source;uint8_t*decoded=NULL;size_t size=0;
    int found=nflstreet_archive_find(&resolver.face,m.head_key,&index);if(!found)index=(unsigned)m.head_runtime_index;
    if((found?m.head_candidate_index!=(int32_t)index:m.head_candidate_index!=-1)||
       !nflstreet_archive_get(&resolver.face,index,&source)||!nflstreet_archive_decode(&source,&decoded,&size)||size<16||memcmp(decoded,"TMdl",4)){fprintf(stderr,"Unsupported resolved head payload team%u player%u entry%u\n",team,player,index);return 6;}
    free(decoded);++custom;
   }++rows;
  }free(b);
 }
 FILE*q=fopen(argv[4],"rb");if(!q)return 7;char line[1024];
 while(fgets(line,sizeof line,q)){char*pool=line,*label=strchr(pool,'\t');if(!label)return 7;*label++=0;char*key=strchr(label,'\t');if(!key)return 7;*key++=0;char*value=strchr(key,'\t');if(!value)return 7;*value++=0;char*end;long expected=strtol(value,&end,10);if(*end!='\n'&&*end!='\r'&&*end!=0)return 7;
  const nflstreet_archive*a=!strcmp(pool,"face")?&resolver.face:!strcmp(pool,"hair")?&resolver.hair:NULL;if(!a)return 7;
  unsigned index;long actual=nflstreet_archive_find(a,key,&index)?(long)index:-1;if(actual!=expected)return 8;++queries;
 }if(ferror(q))return 7;fclose(q);free(f);free(h);
 if(rows!=402||queries!=2416)return 9;
 printf("Fresh native player resolution passes: %u actual PLAY rows, %u custom head TMdl payloads, %u original-oracle query results; hair choice and full appearance remain unresolved.\n",rows,custom,queries);return 0;
}
