#include "team_effects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {void*queue;uint32_t*count;size_t size;uint32_t capacity;} view;
static view lane(nflstreet_team_screen*s,unsigned i){
 switch(i){
 case 0:return(view){s->classes.shared.resources,&s->classes.shared.resource_count,sizeof s->classes.shared.resources[0],128};
 case 1:return(view){s->classes.shared.broadcasts,&s->classes.shared.broadcast_count,sizeof s->classes.shared.broadcasts[0],128};
 case 2:return(view){s->classes.shared.pending,&s->classes.shared.pending_count,sizeof s->classes.shared.pending[0],128};
 case 3:return(view){s->classes.shared.audio,&s->classes.shared.audio_count,sizeof s->classes.shared.audio[0],128};
 case 4:return(view){s->classes.effects,&s->classes.effect_count,sizeof s->classes.effects[0],128};
 case 5:return(view){s->actors.requests,&s->actors.request_count,sizeof s->actors.requests[0],NFLSTREET_TEAM_ACTOR_REQUEST_CAPACITY};
 case 6:return(view){s->activation.effects,&s->activation.effect_count,sizeof s->activation.effects[0],NFLSTREET_TEAM_ACTIVATION_EFFECTS};
 case 7:return(view){s->confirmation.effects,&s->confirmation.effect_count,sizeof s->confirmation.effects[0],NFLSTREET_TEAM_CONFIRM_EFFECTS};
 case 8:return(view){s->transition_requests,&s->transition_count,sizeof s->transition_requests[0],16};
 case 9:return(view){s->player_spawns,&s->player_spawn_count,sizeof s->player_spawns[0],64};
 default:return(view){s->player_resources,&s->player_resource_count,sizeof s->player_resources[0],1024};
 }
}
static void clear_queues(nflstreet_team_screen*s){for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i){view v=lane(s,i);memset(v.queue,0,(size_t)v.capacity*v.size);*v.count=0;}}
static int owners_equal(nflstreet_team_screen*a,nflstreet_team_screen*b){
 nflstreet_team_screen*ca=malloc(sizeof *ca),*cb=malloc(sizeof *cb);if(!ca||!cb){free(ca);free(cb);return 0;}
 memcpy(ca,a,sizeof *ca);memcpy(cb,b,sizeof *cb);clear_queues(ca);clear_queues(cb);
 int equal=memcmp(ca,cb,sizeof *ca)==0;free(ca);free(cb);return equal;
}
static int fill_queue(nflstreet_team_screen*s,unsigned i,unsigned count,uint8_t seed){view v=lane(s,i);if(count>v.capacity)return 0;*v.count=count;for(unsigned r=0;r<count;++r)memset((uint8_t*)v.queue+(size_t)r*v.size,(uint8_t)(seed+r*13u),v.size);return 1;}

int main(void){
 nflstreet_team_screen*s=calloc(1,sizeof *s),*before=malloc(sizeof *before),*owner_before=malloc(sizeof *owner_before);
 nflstreet_team_effect_batch*b=malloc(sizeof *b),*bad=malloc(sizeof *bad);
 if(!s||!before||!owner_before||!b||!bad)return 2;
 for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i)if(!fill_queue(s,i,3,(uint8_t)(0x21+i*7u)))return 3;
 s->game.startup.selected_id[0]=0x11112222;s->game.startup.selected_id[1]=0x33334444;
 s->selected_rosters[0].team_id=0x11112222;s->selected_rosters[0].count=2;s->selected_rosters[0].player_ids[0]=101;s->selected_rosters[0].player_ids[1]=102;
 s->selected_rosters[1].team_id=0x33334444;s->selected_rosters[1].count=1;s->selected_rosters[1].player_ids[0]=201;
 s->player_window.sides[0].count=2;s->player_window.sides[0].cursor=1;s->player_window.sides[0].rows[1]=(nflstreet_team_player_window_row){102,6,0xaabbccdd};
 s->player_window.sides[1].count=1;s->player_window.sides[1].cursor=0;s->player_window.sides[1].rows[0]=(nflstreet_team_player_window_row){201,0,0x12345678};
 s->activation.sides[0].count=7;s->activation.sides[0].roster[6]=(nflstreet_team_activation_row){107,6,0xabcdef01};
 s->activation.sides[1].count=7;s->activation.sides[1].roster[6]=(nflstreet_team_activation_row){207,6,0xabcdef02};
 s->actors.callback_registered=1;s->transition_object[0]=0x5a;s->confirmation.sides[0].count=7;s->confirmation.sides[0].rows[6]=(nflstreet_team_confirm_row){107,6,0x99999999};
 memcpy(before,s,sizeof *s);
 if(!nflstreet_team_effects_capture(s,b)||memcmp(before,s,sizeof *s))return 4;
 for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i)if(b->counts[i]!=3)return 5;
 /* Acknowledge prefixes after appending one fresh effect to every lane. */
 const uint32_t ack_counts[NFLSTREET_TEAM_EFFECT_LANES]={1,2,0,1,3,2,1,0,2,1,2};
 for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i){view v=lane(s,i);memset((uint8_t*)v.queue+(size_t)*v.count*v.size,(uint8_t)(0xe0+i),v.size);++*v.count;}
 memcpy(before,s,sizeof *s);memcpy(owner_before,s,sizeof *s);
 nflstreet_team_effect_ack ack={0};memcpy(ack.count,ack_counts,sizeof ack_counts);
 if(!nflstreet_team_effects_ack(s,b,&ack))return 6;
 for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i){view got=lane(s,i),old=lane(before,i);uint32_t remain=4-ack_counts[i];
  if(*got.count!=remain||memcmp(got.queue,(uint8_t*)old.queue+(size_t)ack_counts[i]*got.size,(size_t)remain*got.size))return 7;}
 if(!owners_equal(s,owner_before))return 8;
 /* A stale prefix in a later lane rejects all earlier requested drains. */
 if(!nflstreet_team_effects_capture(s,bad))return 9;
 view last=lane(s,NFLSTREET_TEAM_EFFECT_LANES-1);if(!*last.count)return 10;((uint8_t*)last.queue)[0]^=0x80;
 memcpy(before,s,sizeof *s);for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i)ack.count[i]=1;
 if(nflstreet_team_effects_ack(s,bad,&ack)||memcmp(before,s,sizeof *s))return 11;
 /* A modified capture and an excessive prefix count also fail atomically. */
 memcpy(s,before,sizeof *s);if(!nflstreet_team_effects_capture(s,bad))return 12;
 if(bad->counts[0])((uint8_t*)bad->resources)[0]^=1;
 memcpy(owner_before,s,sizeof *s);if(nflstreet_team_effects_ack(s,bad,&ack)||memcmp(owner_before,s,sizeof *s))return 13;
 if(!nflstreet_team_effects_capture(s,bad))return 14;memcpy(owner_before,s,sizeof *s);
 ack.count[0]=bad->counts[0]+1;
 if(nflstreet_team_effects_ack(s,bad,&ack)||memcmp(owner_before,s,sizeof *s))return 15;
 /* An invalid count blocks capture without changing caller output or source. */
 memcpy(s,owner_before,sizeof *s);view first=lane(s,0);*first.count=first.capacity+1;memcpy(before,s,sizeof *s);memset(b,0x5c,sizeof *b);
 if(nflstreet_team_effects_capture(s,b)||memcmp(before,s,sizeof *s))return 16;
 for(size_t i=0;i<sizeof *b;++i)if(((uint8_t*)b)[i]!=0x5c)return 17;
 puts("Team effect snapshot/ack: 11 ordered queue prefixes; partial and zero-count drains preserved appended suffixes; stale/mutated/oversized acknowledgements rejected atomically; selection, roster, cursor, and owner state preserved.");
 free(s);free(before);free(owner_before);free(b);free(bad);return 0;
}
