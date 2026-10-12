/* Exercise the joined screen router using the independent original E9D4
   fixture. Its DB getter and visibility services remain supplied boundaries. */
#define nflstreet_team_player_window_callback_memory routed_window
#define main standalone_fixture_main
#include "team_player_window_callback_check.c"
#undef main
#undef nflstreet_team_player_window_callback_memory
#include "team_screen.h"

int routed_window(void*ctx,uint8_t*b,size_t n,uint32_t base,
 const nflstreet_ui_service_request*q,uint32_t*out,uint32_t sb,uint32_t words[256]){
 nflstreet_team_screen*s=calloc(1,sizeof *s);if(!s)return 0;
 s->player_window=*(nflstreet_team_player_window*)ctx;
 s->supplied_window_mask=3;
 for(unsigned side=0;side<2;++side)s->supplied_window_team[side]=s->game.startup.selected_id[side]=side+1;
 int ok=nflstreet_team_screen_memory(s,b,n,base,q,out,sb,words);
 if(ok)*(nflstreet_team_player_window*)ctx=s->player_window;
 free(s);return ok;
}
static int team_guards(void){
 nflstreet_team_screen*s=calloc(1,sizeof *s),*old=malloc(sizeof *s);if(!s||!old)return 0;
 uint8_t b[512],saved[512];uint32_t words[256]={0},out=0xabcdef01;
 memset(b,0x55,sizeof b);put(b+32,0);memcpy(saved,b,sizeof b);
 nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000010;q.argc=4;q.args[0]=0;q.args[2]=1;q.args[3]=SCREEN+32;
 for(unsigned v=0;v<3;++v){s->supplied_window_mask=v==0?0:v==1?1:4;s->supplied_window_team[0]=2;s->game.startup.selected_id[0]=1;*old=*s;
  if(nflstreet_team_screen_memory(s,b,sizeof b,SCREEN,&q,&out,VM_STACK,words)||memcmp(s,old,sizeof *s)||memcmp(b,saved,sizeof b)||out!=0xabcdef01){free(s);free(old);return 0;}}
 free(s);free(old);return 1;
}
int main(int argc,char**argv){if(!team_guards())return 9;int result=standalone_fixture_main(argc,argv);
 if(!result)puts("Joined team-screen player-window routing exact; missing/stale/invalid supplied-team guards reject atomically.");return result;}
