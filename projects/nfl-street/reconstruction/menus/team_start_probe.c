#include "team_start.h"
#include "team_callback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
 if(argc!=2)return 2;nflstreet_team_catalog cat={0};
 for(unsigned i=1;i<=32;++i){char path[4096];int z=snprintf(path,sizeof path,"%s/%05u.bin",argv[1],i);if(z<0||z>=(int)sizeof path)return 3;
  FILE*f=fopen(path,"rb");if(!f)return 3;uint8_t b[65536];size_t n=fread(b,1,sizeof b,f);int extra=fgetc(f);fclose(f);nflstreet_team_table t;
  if(extra!=EOF||!nflstreet_team_table_open(b,n,&t)||!nflstreet_team_catalog_append(&cat,&t))return 4;
 }
 nflstreet_team_start_state s={0};for(unsigned i=0;i<2;++i)s.prior_profile[i]=s.selected_id[i]=UINT32_MAX;
 if(!nflstreet_team_start_apply(&cat,1,&s)||s.effect_count!=12)return 5;
 for(unsigned side=0;side<2;++side){uint32_t center=s.carousel.windows[side][1];if(center!=cat.records[0].id||s.carousel.catalogs[side].cursor!=2)return 6;
  nflstreet_team_carousel_output out;if(!nflstreet_team_carousel_apply(&s.carousel,side,side?3:1,&out))return 7;
  const nflstreet_team_record *r=nflstreet_team_catalog_find(&cat,out.values[1]);if(!r)return 7;
  printf("Native startup side%u: source fallback ID%u; operation%u selects ID%u %s; eligibility count%u.\n",side,center,side?3:1,r->id,r->name,s.eligibility_count[side]);
 }
 nflstreet_team_start_state before=s;if(nflstreet_team_start_apply(&cat,2,&s)||memcmp(&s,&before,sizeof s))return 8;
 nflstreet_team_catalog invalid=cat;invalid.records[0].sivt=2;if(nflstreet_team_start_apply(&invalid,1,&s)||memcmp(&s,&before,sizeof s))return 8;
 nflstreet_team_callback bridge={0};bridge.catalog=cat;bridge.query_mode=1;
 for(unsigned i=0;i<2;++i)bridge.startup.prior_profile[i]=bridge.startup.selected_id[i]=UINT32_MAX;
 uint8_t screen[64]={0};uint32_t stack[256]={0},result=0xabcddcbau;
 const uint32_t base=0x81100000,sb=0x817c0000;
 nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000001u;
 if(!nflstreet_team_callback_memory(&bridge,screen,sizeof screen,base,&q,&result,sb,stack)||result)return 9;
 q.property=0x80000004u;q.argc=4;q.args[0]=0;q.args[1]=1;q.args[2]=sb+40;q.args[3]=base;stack[10]=2;result=0xabcddcbau;
 if(!nflstreet_team_callback_memory(&bridge,screen,sizeof screen,base,&q,&result,sb,stack)||stack[14]!=cat.records[1].id||result!=0xabcddcbau)return 10;
 /* Reverse the two memory owners and use a different array dimension. */
 q.args[0]=1;q.args[1]=3;q.args[2]=base;q.args[3]=sb+240;stack[60]=1;
 if(!nflstreet_team_callback_memory(&bridge,screen,sizeof screen,base,&q,&result,sb,stack))return 10;
 uint32_t center=(uint32_t)screen[8]<<24|(uint32_t)screen[9]<<16|(uint32_t)screen[10]<<8|screen[11];
 if(center!=cat.records[cat.count-1].id||result!=0xabcddcbau)return 10;
 nflstreet_team_callback prior=bridge;uint8_t old_screen[64];uint32_t old_stack[256];memcpy(old_screen,screen,64);memcpy(old_stack,stack,sizeof stack);
 for(unsigned guard=0;guard<6;++guard){nflstreet_ui_service_request bad=q;
  if(guard==0)bad.argc=3;else if(guard==1)bad.args[0]=2;else if(guard==2)bad.args[3]=sb+1020;
  else if(guard==3)bad.args[3]=bad.args[2];else if(guard==4)bad.property=0x80000006u;else bad.kind=0;
  if(nflstreet_team_callback_memory(&bridge,screen,sizeof screen,base,&bad,&result,sb,stack)||memcmp(&bridge,&prior,sizeof bridge)||memcmp(screen,old_screen,64)||memcmp(stack,old_stack,sizeof stack)||result!=0xabcddcbau)return 11;
 }
 puts("Native team callback bridge passes: actual startup, mixed screen/stack carousel outputs in both directions, dimensions0/1/2 and six atomic rejection guards.");
 puts("Independent native original team-start/carousel join passes: real32-team records, source fallback initialization, both-side changes and atomic rejection; resource effects retained, no original module or source-screen activation.");return 0;
}
