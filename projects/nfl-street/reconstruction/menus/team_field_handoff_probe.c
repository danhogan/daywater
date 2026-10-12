/* Actual native source completion; lower device/clock services are supplied. */
#include "team_input_probe.c"
#include "dispatcher.h"
static int loaded_start(uint8_t*,size_t,uint32_t,nflstreet_team_screen*);
static int complete_team(uint8_t*,size_t,nflstreet_team_screen*);
static const char*handoff_events="7,7,7,7";
#define DAYWATER_TEAM_STARTUP_HOST daywater_team_startup_host
#define DAYWATER_TEAM_STARTUP_CONTROLS complete_team
#define nflstreet_team_screen_start(b,n,base,s) loaded_start(b,n,base,s)
#define main team_probe_main
#include "team_screen_probe.c"
#undef main
#undef nflstreet_team_screen_start

int main(int argc,char**argv){
 if(argc!=5&&argc!=6)return 2;
 if(argc==6)handoff_events=argv[5];
 return team_probe_main(5,argv);
}

static int loaded_start(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s){
 const uint32_t args[2]={1,0x69};
 return nflstreet_team_screen_start_context(b,n,base,s,7,5,0x817c0000u,UINT32_MAX,2,args);
}
static int complete_team(uint8_t*b,size_t n,nflstreet_team_screen*s){
 const char*sequence=handoff_events;
 unsigned count=0;
 while(*sequence){if(*sequence<'2'||*sequence>'7'||++count>32)return 0;unsigned event=(unsigned)(*sequence++-'0');
  if(*sequence&&*sequence++!=',')return 0;
  if(!nflstreet_team_screen_input(b,n,0x81100000u,s,0,(uint16_t)event,1.0f)){
   fprintf(stderr,"Handoff event%u step%u rejected; retained teams%u/%u lineups%u/%u transitions%u\n",event,count,s->game.startup.selected_id[0],s->game.startup.selected_id[1],s->activation.sides[0].selected_count,s->activation.sides[1].selected_count,s->transition_count);return 0;}
  for(unsigned tick=0;tick<60;++tick)if(!nflstreet_team_screen_advance(b,n,0x81100000u,s,16))return 0;
  printf("Handoff step%u event%u teams%u/%u lineups%u/%u transitions%u broadcasts%u links%u\n",count,event,s->game.startup.selected_id[0],s->game.startup.selected_id[1],s->activation.sides[0].selected_count,s->activation.sides[1].selected_count,s->transition_count,s->classes.shared.broadcast_count,s->classes.shared.link_count);
 }
 if(s->activation.sides[0].selected_count!=7||s->activation.sides[1].selected_count!=7||s->classes.shared.link_count!=1||s->classes.shared.links[0]!=5)return 0;
 /* Retain the native 14-word formation inputs needed by the later field
    commit. This is native ownership evidence, not full original startup. */
 for(unsigned side=0;side<2;++side){
  const nflstreet_team_activation_side*a=&s->activation.sides[side];
  if(a->formation_count!=7)return 0;
  printf("Native handoff side%u team%u formation14",side,s->game.startup.selected_id[side]);
  for(unsigned slot=0;slot<14;++slot){
   uint32_t id=a->active_formation[slot];unsigned matches=0;
   for(unsigned player=0;player<7;++player)matches+=a->selected_ids[player]==id;
   if(matches!=1)return 0;
   printf(" %u",id);
  }
  putchar('\n');
 }
 size_t qn;uint8_t*q=load("W:\\Daywater\\research-temp\\nflstreet-original-menus-20261009\\nested\\uis_all\\00000\\00005.bin",&qn);unsigned relocated,flags=0;
 nflstreet_ui_dispatcher setup={0};
 if(!q||!nflstreet_ui_relocate(q,qn,0x81100000u,&relocated))return 0;
 /* Initialize coordinator5, then supply only the preceding controller
    completion. Team completion below must come from executed team scripts. */
 int ok=nflstreet_ui_quick_setup_event(q,qn,0x81100000u,&flags,&setup,0xfffe,0,0)&&
  nflstreet_ui_quick_setup_event(q,qn,0x81100000u,&flags,&setup,1,0x76,UINT32_MAX)&&
  nflstreet_ui_quick_setup_event(q,qn,0x81100000u,&flags,&setup,0xfffc,0,0);
 unsigned first=setup.count,forwarded=0;
 for(unsigned i=0;ok&&i<s->classes.shared.broadcast_count;++i){const nflstreet_ui_service_request*r=&s->classes.shared.broadcasts[i];
  if(r->property==1&&r->argc==2){ok=nflstreet_ui_quick_setup_event(q,qn,0x81100000u,&flags,&setup,1,r->args[0],r->args[1]);++forwarded;}}
 if(ok)ok=nflstreet_ui_quick_setup_event(q,qn,0x81100000u,&flags,&setup,0xfffc,0,0);
 unsigned loaded=0,activated=0;
 for(unsigned i=first;ok&&i<setup.count;++i){const nflstreet_ui_service_request*r=&setup.requests[i];if(r->target==8){loaded+=r->property==0;activated+=r->property==3;}}
 free(q);
 if(!ok||forwarded!=1||loaded!=1||activated!=1)return 0;
 puts("Actual native team source completion -> coordinator5 -> field8 load/activation requests pass; controller completion/device/clock supplied; field recipient and fresh match absent.");
 return 1;
}
