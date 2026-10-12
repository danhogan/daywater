#include "team_selection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 if(argc!=2)return 2;nflstreet_team_catalog catalog={0};
 for(unsigned i=1;i<=32;++i){char path[4096];if(snprintf(path,sizeof path,"%s/%05u.bin",argv[1],i)>=(int)sizeof path)return 3;
  FILE*f=fopen(path,"rb");if(!f)return 3;uint8_t b[65536];size_t n=fread(b,1,sizeof b,f);int extra=fgetc(f);fclose(f);nflstreet_team_table t;
  if(extra!=EOF||!nflstreet_team_table_open(b,n,&t)||!nflstreet_team_catalog_append(&catalog,&t))return 4;
 }
 if(catalog.count!=32)return 4;uint32_t centers[2]={catalog.records[0].id,catalog.records[1].id};nflstreet_team_carousel c;
 if(!nflstreet_team_selection_seed(&catalog,centers,&c))return 5;
 for(unsigned side=0;side<2;++side){uint32_t initial[3];memcpy(initial,c.windows[side],sizeof initial);unsigned steps=0;
  for(unsigned direction=0;direction<2;++direction)for(unsigned i=0;i<catalog.count;++i){nflstreet_team_carousel_output out;
   if(!nflstreet_team_carousel_apply(&c,side,direction?3:1,&out))return 6;
   for(unsigned j=0;j<3;++j){const nflstreet_team_record *record=nflstreet_team_catalog_find(&catalog,out.values[j]);if(!record||out.flags[j]!=(record->sivt!=0))return 7;}
   ++steps;
   if(i==0){const nflstreet_team_record *selected=nflstreet_team_catalog_find(&catalog,out.values[1]);printf("Local side%u direction%u center:%u %s raw FORT/EDRT/VORT:%u/%u/%u\n",side,direction,selected->id,selected->name,selected->fields[0],selected->fields[1],selected->fields[2]);}
   if(i==catalog.count-1&&memcmp(initial,c.windows[side],sizeof initial))return 8;
  }
  printf("Local side%u: %u forward/backward steps through all32 private records; complete wraps preserve window.\n",side,steps);
 }
 nflstreet_team_catalog before=catalog;nflstreet_team_table t={0};if(nflstreet_team_catalog_append(&catalog,&t)||memcmp(&catalog,&before,sizeof before))return 9;
 puts("Independent native team selection data/carousel pass; supplied starting IDs and no-profile eligibility branch. No original menu activation, confirmation or selected-match creation claimed.");return 0;
}
