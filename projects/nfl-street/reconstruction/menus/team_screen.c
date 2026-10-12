#include "team_screen.h"
#include "team_format.h"
#include "team_player_remove.h"
#include "team_player_labels_callback.h"
#include "events.h"
#include "construct.h"
#include "text_construct.h"
#include "image_construct.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void ps(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int nflstreet_team_screen_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*out){
 nflstreet_team_screen*s=ctx;if(!s||!b||!q||!out)return 0;
 if(q->kind==1&&q->property==0x135&&!q->argc)return 1; /* 8010CB80 accepts this query without writing its result. */
 if(q->kind==1&&!q->argc&&(q->property==0xcb||q->property==0x104||q->property==0x105)){*out=0;return 1;} /* Same supplied common-query false branch as controller startup. */
 if(q->kind==1&&q->property==0xe0&&!q->argc){*out=s->supplied_common_mode>=3&&s->supplied_common_mode<=5?1:0;return 1;}
 if(q->kind==1&&q->property==1&&!q->argc){*out=2;return 1;} /* Original8010BE48 constant. */
 if(q->kind==10)return nflstreet_team_format(b,n,base,q);
 if(q->kind==0&&q->class_id==3)return nflstreet_team_actor_service(&s->actors,b,n,base,q,out);
 if(q->kind==3||q->kind==4||q->kind==5)return nflstreet_ui_rate_request(&s->classes.shared.rates,b,n,base,7,q);
 if(q->kind==6)return nflstreet_ui_class_activation(b,n,base,q,nflstreet_team_screen_service,s);
 if(q->kind==0&&q->class_id==0&&q->property==7&&q->argc==1){
  if(q->target<base||q->target-base>n||n-(q->target-base)<12)return 0;uint8_t*o=b+q->target-base;unsigned archive=q->args[0]>>16,entry=q->args[0]&65535;
  if((sh(o)&0x10)||(archive!=65535&&entry!=65535)){ps(o+8,entry);ps(o+10,archive);}ps(o,sh(o)|0x4000);return 1;
 }
 if(q->kind==0&&q->class_id==0&&q->property==5&&q->argc==1){
  if(q->target<base||q->target-base>n||n-(q->target-base)<36)return 0;uint8_t*o=b+q->target-base;
  unsigned archive=q->args[0]>>16,entry=q->args[0]&65535,oldarchive=sh(o+6),oldentry=sh(o+4);
  if(!archive||archive==65535)archive=oldarchive;
  if((archive!=65535&&(archive>=32||!(s->supplied_static_archives&(1u<<archive))))||(oldarchive!=65535&&(oldarchive>=32||!(s->supplied_static_archives&(1u<<oldarchive)))))return 0;
  if(archive!=oldarchive||entry!=oldentry){unsigned count=(oldarchive!=65535&&oldentry!=65535)+(archive!=65535&&entry!=65535);if(count>128-s->classes.shared.resource_count)return 0;
   if(oldarchive!=65535&&oldentry!=65535)s->classes.shared.resources[s->classes.shared.resource_count++]=(nflstreet_ui_button_effect){0,oldarchive,oldentry};
   if(archive!=65535&&entry!=65535)s->classes.shared.resources[s->classes.shared.resource_count++]=(nflstreet_ui_button_effect){1,archive,entry};ps(o+4,entry);ps(o+6,archive);
  }
  ps(o+12,65535);ps(o+14,65535);ps(o,sh(o)|0x4000);return 1;
 }
 if(q->kind==0&&q->class_id==0&&q->property==0xfffffffc&&q->argc==1){
  if(q->target<base||q->target-base>n||n-(q->target-base)<36)return 0;uint8_t*o=b+q->target-base;unsigned archive=(unsigned)o[6]<<8|o[7];
  if((o[1]&2)&&(archive>=32||!(s->supplied_static_archives&(1u<<archive))))return 0;o[0]|=0x40;return 1;
 }
 if(q->kind==1&&q->property>=0x80000000u)return 0; /* Team game requests require stack-aware bridge. */
 if(q->kind==0&&q->class_id==8&&q->property==3&&q->argc==1){
  if(q->target<base||q->target-base>n||n-(q->target-base)<40||q->args[0]<base||q->args[0]-base>n||n-(q->args[0]-base)<12)return 0;
  uint8_t*o=b+q->target-base;uint32_t desc=be(o+32);if(desc<base||desc-base>n||n-(desc-base)<12)return 0;
  uint32_t source=be(b+q->args[0]-base+8),dest=be(b+desc-base+8),cap=be(b+desc-base+4);
  if(source<base||source-base>=n||dest<base||dest-base>n||cap>=n-(dest-base))return 0;
  uint8_t*last=memchr(b+source-base,0,n-(source-base));if(!last)return 0;size_t length=last-(b+source-base);if(length>cap)return 0;
  memmove(b+dest-base,b+source-base,length+1);return nflstreet_team_classes_prepare_text(&s->objects,b,n,base,q->target);
 }
 if(q->kind==0&&q->class_id==8&&q->target>=base&&q->target-base<=n&&n-(q->target-base)>=40&&b[q->target-base+2]==3)return nflstreet_ui_main_service(&s->classes.shared,b,n,base,q,out);
 if(q->kind==0||q->kind==10)return nflstreet_ui_options_screen_service(&s->classes,b,n,base,q,out);
 return nflstreet_ui_main_service(&s->classes.shared,b,n,base,q,out);
}
static int owned_word(uint8_t*b,size_t n,uint32_t base,uint32_t sb,uint32_t words[256],uint32_t address,uint32_t*value,int write){
 if(address>=base&&address-base<=n&&n-(address-base)>=4){if(write){uint8_t*p=b+address-base;p[0]=*value>>24;p[1]=*value>>16;p[2]=*value>>8;p[3]=*value;}else *value=be(b+address-base);return 1;}
 if(address>=sb&&address-sb<=1020&&!((address-sb)&3)){if(write)words[(address-sb)/4]=*value;else *value=words[(address-sb)/4];return 1;}return 0;
}
int nflstreet_team_screen_memory(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*out,uint32_t sb,uint32_t words[256]){
 nflstreet_team_screen*s=ctx;
 if(!s||!b||!q||!out||!words||!base||!sb||n>UINT32_MAX-base||sb>UINT32_MAX-1024u||(base<sb+1024u&&sb<(uint64_t)base+n))return 0;
 if(q->kind==1&&q->property==0x80000007u){
  if(q->argc!=1)return 0;
  /* 8011E9AC treats its raw argument as a boolean side selector and writes
     only the corresponding dirty/rebuild flags. */
  unsigned side=q->args[0]!=0;
  s->activation.sides[side].dirty=1;
  s->activation.sides[side].rebuild=1;
  return 1;
 }
 if(q->kind==1&&q->property==0xfu&&s->supplied_controller_sides){
  uint32_t dims,at,old[4];if(q->argc!=1||s->supplied_controller_sides!=1||!owned_word(b,n,base,sb,words,q->args[0],&dims,0)||dims>8||q->args[0]>UINT32_MAX-(dims+5u)*4u)return 0;
  at=q->args[0]+(dims+1u)*4u;for(unsigned i=0;i<4;++i)if(!owned_word(b,n,base,sb,words,at+i*4,&old[i],0)||(s->controller_sides[i]!=UINT32_MAX&&s->controller_sides[i]>1))return 0;
  for(unsigned i=0;i<4;++i){uint32_t value=s->controller_sides[i];if(!owned_word(b,n,base,sb,words,at+i*4,&value,1))return 0;}return 1;
 }
 if(q->kind==1&&q->property==0xau){
  uint32_t port;if(q->argc!=1||s->supplied_port_mapping_mask>1023||!owned_word(b,n,base,sb,words,q->args[0],&port,0)||port>=10||!(s->supplied_port_mapping_mask&(1u<<port)))return 0;
  uint32_t mapping=s->port_mapping[port];return owned_word(b,n,base,sb,words,q->args[0],&mapping,1);
 }
 if(q->kind==1&&q->property==0xdu&&s->bootstrap&&q->argc==1&&q->args[0]!=UINT32_MAX){
  /* 8010C154 opens the player row then calls 8014EAC4. The fresh host
     already retains that verified selector's result beside each PLAY ID.
     Original database registration/I/O remains outside this owned view. */
  if(s->supplied_roster_count>32)return 0;unsigned found=0;uint32_t asset=0;
  for(unsigned team=0;team<s->supplied_roster_count;++team){
   if(s->supplied_rosters[team].count>20)return 0;
   for(unsigned i=0;i<s->supplied_rosters[team].count;++i)if(s->supplied_rosters[team].players[i]==q->args[0]){
    uint32_t value=s->supplied_rosters[team].assets[i];if(found&&value!=asset)return 0;asset=value;++found;
   }
  }
  if(!found)return 0;*out=asset;return 1;
 }
 if(q->kind==1&&q->property==0xe&&s->bootstrap){
  if(q->argc!=2||q->args[1]>=14||s->player_spawn_count>=64)return 0;
  unsigned side=q->args[1]/7,found=0;for(unsigned i=0;i<s->activation.sides[side].count;++i)if(s->activation.sides[side].roster[i].id==q->args[0])++found;if(found!=1)return 0;
  unsigned dbindex=32;for(unsigned i=0;i<s->supplied_roster_count;++i)if(s->supplied_rosters[i].id==s->game.startup.selected_id[side])dbindex=i;
  nflstreet_team_player_spawn_request request;
  if(dbindex==32||!nflstreet_team_player_spawn_from_db(s->roster_databases[dbindex],s->roster_database_sizes[dbindex],s->game.startup.selected_id[side],q->args[0],q->args[1],0,1,s->bootstrap->constants->actor_labels[q->args[1]],&request))return 0;
  s->player_spawns[s->player_spawn_count++]=request;return 1; /* Explicit renderer request boundary. */
 }
 if(q->kind==1&&q->property==0x80000011u&&s->bootstrap){
  const char*roles[14];for(unsigned i=0;i<14;++i)roles[i]=s->bootstrap->constants->role_names[i];
  nflstreet_team_player_labels_bridge bridge={0};bridge.role_texts=roles;bridge.role_text_count=14;bridge.supplied_mask=s->supplied_window_mask;
  for(unsigned side=0;side<2;++side){bridge.rows[side]=s->label_rows[side];bridge.count[side]=s->player_window.sides[side].count;bridge.cursor[side]=s->player_window.sides[side].cursor;bridge.mode[side]=s->player_window.sides[side].mode;bridge.selected_team_id[side]=s->game.startup.selected_id[side];bridge.supplied_team_id[side]=s->supplied_window_team[side];}
  return nflstreet_team_player_labels_callback_memory(&bridge,b,n,base,q,out,sb,words);
 }
 if(q->kind==1&&(q->property==0x8000000fu||q->property==0x80000010u)){
  if(!q->argc||q->args[0]>1||s->supplied_window_mask>3)return 0;unsigned side=q->args[0];
  if(!(s->supplied_window_mask&(1u<<side))||s->supplied_window_team[side]!=s->game.startup.selected_id[side])return 0;
  return nflstreet_team_player_window_callback_memory(&s->player_window,b,n,base,q,out,sb,words);
 }
 if(q->kind==1&&q->property==0x80000012u){
  if(q->argc!=2||q->args[0]>1||s->supplied_activation_mask>3)return 0;unsigned side=q->args[0];
  if(!(s->supplied_activation_mask&(1u<<side))||s->supplied_activation_team[side]!=s->game.startup.selected_id[side])return 0;
  uint32_t dims,address=q->args[1],output[7];if(!owned_word(b,n,base,sb,words,address,&dims,0)||dims>8||address>UINT32_MAX-(dims+8u)*4u)return 0;
  address+=(dims+1u)*4u;for(unsigned i=0;i<7;++i)if(!owned_word(b,n,base,sb,words,address+i*4,&output[i],0))return 0;
  nflstreet_team_activation next=s->activation;next.effect_count=0;
  if(!nflstreet_team_activation_apply(&next,side,output,7))return 0;
  for(unsigned i=0;i<next.sides[side].selected_count;++i)if(!owned_word(b,n,base,sb,words,address+i*4,&output[i],1))return 0;
  s->activation=next;
  if(s->bootstrap){nflstreet_team_player_window_side*w=&s->player_window.sides[side];
   w->count=next.sides[side].count;for(unsigned i=0;i<w->count;++i){w->rows[i].id=next.sides[side].roster[i].id;w->rows[i].slot=next.sides[side].roster[i].selected;w->rows[i].auxiliary=next.sides[side].roster[i].auxiliary;}}
  return 1;
 }
 if(q->kind==1&&q->property==0x8000000du&&s->bootstrap){
  if(q->argc!=2||q->args[0]>1)return 0;unsigned side=q->args[0];uint32_t slot=0;
  if(!owned_word(b,n,base,sb,words,q->args[1],&slot,0))return 0;
  nflstreet_team_activation a=s->activation;nflstreet_team_player_window w=s->player_window;a.effect_count=0;
  if(!nflstreet_team_player_remove(&a,&w,side,&s->bootstrap->constants->reset_poses[side],&slot)||!owned_word(b,n,base,sb,words,q->args[1],&slot,1))return 0;
  s->activation=a;s->player_window=w;return 1;
 }
 if(q->kind==1&&q->property==0x80000008u&&s->bootstrap){
  if(q->argc!=3||q->args[0]>1)return 0;unsigned side=q->args[0];uint32_t cursor,result;
  if(!(s->supplied_activation_mask&(1u<<side))||!(s->supplied_window_mask&(1u<<side))||s->supplied_activation_team[side]!=s->game.startup.selected_id[side]||s->supplied_window_team[side]!=s->game.startup.selected_id[side]||s->player_window.sides[side].count!=s->activation.sides[side].count)return 0;
  if(!owned_word(b,n,base,sb,words,q->args[1],&cursor,0))return 0;
  /* The source remove path sets dirty=1, so DF88 skips the pose-index
     tables. Clean confirmation needs those tables and remains unsupported. */
  if(q->args[2]!=UINT32_MAX&&!s->activation.sides[side].dirty)return 0;
  nflstreet_team_confirm next=s->confirmation;
  for(unsigned j=0;j<2;++j){const nflstreet_team_activation_side*a=&s->activation.sides[j];
   if(a->count>20)return 0;next.sides[j].count=a->count;next.sides[j].source_flag_6e34=a->dirty;next.sides[j].source_flag_6e30=a->rebuild;next.sides[j].last_confirmed_id=a->last_seventh_player_id;
   for(unsigned i=0;i<a->count;++i)next.sides[j].rows[i]=(nflstreet_team_confirm_row){a->roster[i].id,a->roster[i].selected,a->roster[i].auxiliary};
  }
  if(!nflstreet_team_confirm_apply(&next,side,q->args[2],&cursor,&result,NULL,NULL)||!owned_word(b,n,base,sb,words,q->args[1],&cursor,1))return 0;
  nflstreet_team_activation_side*a=&s->activation.sides[side];nflstreet_team_player_window_side*w=&s->player_window.sides[side];
  a->dirty=next.sides[side].source_flag_6e34;a->rebuild=next.sides[side].source_flag_6e30;a->last_seventh_player_id=next.sides[side].last_confirmed_id;
  for(unsigned i=0;i<a->count;++i){const nflstreet_team_confirm_row*r=&next.sides[side].rows[i];a->roster[i]=(nflstreet_team_activation_row){r->player_id,r->slot,r->auxiliary};w->rows[i]=(nflstreet_team_player_window_row){r->player_id,r->slot,r->auxiliary};}
  s->confirmation=next;*out=result;return 1;
 }
 if(q->kind==1&&q->property==0x80000013u&&s->bootstrap){
  if(q->argc!=1)return 0;
  /* E9D4 clears the callback, then mode6/argument67 calls 80137B84(500).
     Clock/audio/broadcast ownership is explicit and never guessed. */
  if(q->args[0]==0x67&&s->supplied_common_mode==6){
   if(s->supplied_transition!=1||s->transition_count>=16)return 0;
   if(!nflstreet_team_transition_apply(s->transition_object,sizeof s->transition_object,500,s->transition_clock,s->transition_audio,s->transition_context,&s->transition_requests[s->transition_count]))return 0;
   ++s->transition_count;
  }
  s->player_callback=0;s->actors.callback_registered=0;return 1;
 }
 if(q->kind==1&&q->property==0x80000006u){
  if(q->argc!=2||q->args[0]>1||s->supplied_roster_count>32||s->player_resource_count>1024)return 0;
  unsigned side=q->args[0],found=32,old=32;uint32_t prior=s->game.startup.selected_id[side];
  for(unsigned i=0;i<s->supplied_roster_count;++i){if(s->supplied_rosters[i].id==q->args[1])found=i;if(s->supplied_rosters[i].id==prior)old=i;}
  if(found==32||(prior!=UINT32_MAX&&old==32))return 0;
  unsigned count=s->supplied_rosters[found].count,removed=old==32?0:s->supplied_rosters[old].count;
  if(count>20||removed>20||count+removed>1024-s->player_resource_count)return 0;
  nflstreet_team_preview_roster roster;
  if(!nflstreet_team_preview_roster_select(&roster,side,q->args[1],s->supplied_rosters[found].players,count))return 0;
  if(s->bootstrap){
   if(!nflstreet_team_bootstrap_prepare(s->bootstrap,s->roster_databases[found],s->roster_database_sizes[found],q->args[1],side,&s->activation.sides[side],&s->player_window.sides[side],s->label_rows[side]))return 0;
   s->supplied_activation_mask|=1u<<side;s->supplied_window_mask|=1u<<side;
   s->supplied_activation_team[side]=s->supplied_window_team[side]=q->args[1];
  }
  /* Supplied STAT asset rows and GAME roster query, actor gatefalse.
     These effects request I/O; they do not load or render player assets. */
  for(unsigned i=0;i<removed;++i){unsigned k=s->player_resource_count++;s->player_resources[k].kind=0;s->player_resources[k].side=side;s->player_resources[k].archive=13;s->player_resources[k].entry=s->supplied_rosters[old].assets[i];}
  for(unsigned i=0;i<count;++i){unsigned k=s->player_resource_count++;s->player_resources[k].kind=1;s->player_resources[k].side=side;s->player_resources[k].archive=13;s->player_resources[k].entry=s->supplied_rosters[found].assets[i];}
  s->game.startup.selected_id[side]=q->args[1];s->selected_rosters[side]=roster;return 1;
 }
 int ok=q->kind==1&&(q->property>=0x80000000u||q->property==0xf||q->property==0xd||q->property==8)?nflstreet_team_callback_memory(&s->game,b,n,base,q,out,sb,words):nflstreet_team_screen_service(ctx,b,n,base,q,out);
 if(!ok){fprintf(stderr,"Unsupported team dependency kind%u prop%x class%u argc%u target%x",q->kind,q->property,q->class_id,q->argc,q->target);for(unsigned i=0;i<q->argc;++i)fprintf(stderr," arg%u=%x",i,q->args[i]);if(q->kind==0&&q->target>=base&&q->target-base<=n&&n-(q->target-base)>=40){uint8_t*o=b+q->target-base;fprintf(stderr," flags%x font%u archive%u lines%u descriptor%x",sh(o),o[2],o[3],o[15],be(o+32));}fputc('\n',stderr);}return ok;
}
int nflstreet_team_screen_construct(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s){
 if(!s||!b)return 0;uint8_t*copy=malloc(n);nflstreet_team_screen*next=malloc(sizeof *next);if(!copy||!next){free(copy);free(next);return 0;}memcpy(copy,b,n);*next=*s;
 int ok=nflstreet_team_classes_construct(copy,n,base,&next->objects)&&nflstreet_ui_construct(copy,n,base,0,UINT32_MAX,8,nflstreet_team_screen_service,next,sizeof *next);
 if(ok){memcpy(b,copy,n);*s=*next;}free(copy);free(next);return ok;
}
int nflstreet_team_screen_start_context(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s,
 uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base,uint32_t load_argument,
 unsigned loaded_argc,const uint32_t*loaded_args){
 if(!b||!s||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||!stack_base||
    loaded_argc>16||(loaded_argc&&!loaded_args))return 0;
 uint8_t*copy=malloc(n);nflstreet_team_screen*next=malloc(sizeof *next);if(!copy||!next){free(copy);free(next);return 0;}memcpy(copy,b,n);*next=*s;unsigned executed;
 int ok=nflstreet_ui_events_memory_context(copy,n,base,current_screen,linked_screen,stack_base,
  &next->classes.shared.entry_flags,0,0xfffe,load_argument,loaded_argc,loaded_args,
  nflstreet_team_screen_service,next,sizeof *next,&executed,NULL,nflstreet_team_screen_memory);
 uint32_t rows=be(copy+4);if(rows<base||rows-base>n||n-(rows-base)<20)ok=0;uint32_t root=ok?be(copy+rows-base):0;if(root<base||root-base>n||n-(root-base)<8)ok=0;
 if(ok){memset(copy+root-base+4,0,4);copy[root-base+7]=1;ok=nflstreet_ui_events_memory_context(copy,n,base,current_screen,linked_screen,stack_base,
  &next->classes.shared.entry_flags,0,0xfffc,0,0,NULL,
  nflstreet_team_screen_service,next,sizeof *next,&executed,NULL,nflstreet_team_screen_memory);}
 if(ok){memcpy(b,copy,n);*s=*next;}free(copy);free(next);return ok;
}
int nflstreet_team_screen_start(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s){
 return nflstreet_team_screen_start_context(b,n,base,s,7,5,0x817c0000u,0,0,NULL);
}
static int tick(void*ctx,nflstreet_ui_rates*r,uint8_t*b,size_t n,uint32_t base,uint32_t id,uint32_t root,uint32_t pc,unsigned argc,const uint32_t*args,float*out){
 nflstreet_team_screen*s=ctx;if(id!=7||argc>16)return 0;s->classes.shared.rates=*r;nflstreet_ui_script vm={0};vm.current_screen=7;vm.linked_screen=5;vm.root=root;vm.pc=pc;vm.flags=s->classes.shared.entry_flags;memcpy(vm.words+vm.sp++,out,4);
 for(unsigned i=0;i<argc;++i)vm.words[vm.sp++]=args[i];vm.words[vm.sp++]=root;vm.words[vm.sp++]=0;
 if(!nflstreet_ui_script_services_memory(b,n,base,0x817c0000,&vm,100000,nflstreet_team_screen_service,s,sizeof *s,NULL,nflstreet_team_screen_memory))return 0;
 s->classes.shared.entry_flags=vm.flags;*r=s->classes.shared.rates;memcpy(out,vm.words,4);return 1;
}
int nflstreet_team_screen_advance(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s,uint32_t elapsed){
 if(!b||!s||n<32||n>8*1024*1024)return 0;uint8_t*copy=malloc(n);nflstreet_team_screen*next=malloc(sizeof *next);if(!copy||!next){free(copy);free(next);return 0;}memcpy(copy,b,n);*next=*s;
 int ok=nflstreet_ui_rate_update(&next->classes.shared.rates,copy,n,base,7,elapsed,0,tick,next);if(ok){memcpy(b,copy,n);*s=*next;}free(copy);free(next);return ok;
}

static int drain_team_focus(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s,unsigned first,
 uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base){
 if(!b||!s||first>s->classes.shared.pending_count||s->classes.shared.pending_count>128||!base)return 0;
 unsigned nodes_count=be(b);if(!nodes_count||nodes_count>1024)return 0;uint64_t nodes_address=be(b+4);
 if(nodes_address<base||nodes_address-base>n||(size_t)nodes_count*20>n-(size_t)(nodes_address-base))return 0;
 const uint8_t*nodes=b+(size_t)(nodes_address-base);
 /* A focus script may queue a later focus request. Process the newly appended
    suffix in insertion order, matching main/options navigation. Retain the
    request records for the host's separate effect acknowledgement. */
 for(unsigned i=first;i<s->classes.shared.pending_count;++i){
  nflstreet_ui_service_request q=s->classes.shared.pending[i];unsigned target=nodes_count;
  for(unsigned j=0;j<nodes_count;++j)if(be(nodes+j*20)==q.target){target=j;break;}
  if(target==nodes_count)return 0;
  nflstreet_ui_focus_effects effects={0};
  if(!nflstreet_ui_focus_indexed_execute_context(b,n,base,target,1,
      (uint32_t)(int32_t)(int16_t)q.property,&effects,&s->classes.shared.entry_flags,
      current_screen,linked_screen,stack_base,nflstreet_team_screen_service,s,sizeof *s,q.args[0],
      nflstreet_team_screen_memory))return 0;
 }
 return 1;
}

int nflstreet_team_screen_focus_context(uint8_t*b,size_t n,uint32_t base,
 nflstreet_team_screen*s,unsigned node,unsigned selected,uint32_t argument,
 uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base,
 uint32_t list,nflstreet_ui_focus_effects*out){
 if(!b||!s||!out||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||selected>1)return 0;
 nflstreet_ui_focus_effects effects={0};
 if(!nflstreet_ui_focus_indexed_execute_context(b,n,base,node,selected,argument,
      &effects,&s->classes.shared.entry_flags,current_screen,linked_screen,stack_base,
      nflstreet_team_screen_service,s,sizeof *s,list,nflstreet_team_screen_memory))return 0;
 *out=effects;return 1;
}

int nflstreet_team_screen_input_context(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s,uint32_t port,uint16_t event,float value,uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base){
 if(!b||!s||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||port>=10||event<2||event>7||!(value>=0&&value<=1)||s->classes.shared.pending_count>128)return 0;
 uint8_t*copy=malloc(n);nflstreet_team_screen*state=malloc(sizeof *state);if(!copy||!state){free(copy);free(state);return 0;}
 memcpy(copy,b,n);*state=*s;unsigned first=state->classes.shared.pending_count,executed;uint32_t word;memcpy(&word,&value,4);
 int ok=nflstreet_ui_events_memory_context(copy,n,base,current_screen,linked_screen,stack_base,&state->classes.shared.entry_flags,0,event,port,1,&word,
    nflstreet_team_screen_service,state,sizeof *state,&executed,NULL,nflstreet_team_screen_memory);
 if(ok)ok=drain_team_focus(copy,n,base,state,first,current_screen,linked_screen,stack_base);
 if(ok){memcpy(b,copy,n);*s=*state;}free(state);free(copy);return ok;
}

int nflstreet_team_screen_input(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s,uint32_t port,uint16_t event,float value){
 return nflstreet_team_screen_input_context(b,n,base,s,port,event,value,7,5,0x817c0000u);
}

int nflstreet_team_screen_devices(nflstreet_team_screen*s,const nflstreet_team_device_record*records,size_t count,const uint32_t mapping[10]){
 if(!s||!mapping)return 0;uint32_t sides[4];
 if(!nflstreet_team_devices_project(records,count,mapping,sides))return 0;
 memcpy(s->port_mapping,mapping,sizeof s->port_mapping);memcpy(s->controller_sides,sides,sizeof sides);
 s->supplied_port_mapping_mask=1023;s->supplied_controller_sides=1;return 1;
}
