#include "team_effects.h"
#include <string.h>

typedef struct {
 void *queue;
 uint32_t *count;
 const void *captured;
 uint32_t captured_count,ack_count,capacity;
 size_t element_size;
} queue_view;

static int views(nflstreet_team_screen*s,const nflstreet_team_effect_batch*b,
 const nflstreet_team_effect_ack*a,queue_view q[NFLSTREET_TEAM_EFFECT_LANES]){
 if(!s||!b||!a)return 0;
 _Static_assert(sizeof s->classes.effects[0]==sizeof(nflstreet_team_options_effect),"options effect layout");
 _Static_assert(sizeof s->player_resources[0]==sizeof(nflstreet_team_player_resource_effect),"player resource layout");
 q[NFLSTREET_TEAM_EFFECT_RESOURCES]=(queue_view){s->classes.shared.resources,&s->classes.shared.resource_count,b->resources,b->counts[0],a->count[0],128,sizeof b->resources[0]};
 q[NFLSTREET_TEAM_EFFECT_BROADCASTS]=(queue_view){s->classes.shared.broadcasts,&s->classes.shared.broadcast_count,b->broadcasts,b->counts[1],a->count[1],128,sizeof b->broadcasts[0]};
 q[NFLSTREET_TEAM_EFFECT_PENDING]=(queue_view){s->classes.shared.pending,&s->classes.shared.pending_count,b->pending,b->counts[2],a->count[2],128,sizeof b->pending[0]};
 q[NFLSTREET_TEAM_EFFECT_AUDIO]=(queue_view){s->classes.shared.audio,&s->classes.shared.audio_count,b->audio,b->counts[3],a->count[3],128,sizeof b->audio[0]};
 q[NFLSTREET_TEAM_EFFECT_OPTIONS]=(queue_view){s->classes.effects,&s->classes.effect_count,b->options,b->counts[4],a->count[4],128,sizeof b->options[0]};
 q[NFLSTREET_TEAM_EFFECT_ACTORS]=(queue_view){s->actors.requests,&s->actors.request_count,b->actors,b->counts[5],a->count[5],NFLSTREET_TEAM_ACTOR_REQUEST_CAPACITY,sizeof b->actors[0]};
 q[NFLSTREET_TEAM_EFFECT_ACTIVATION]=(queue_view){s->activation.effects,&s->activation.effect_count,b->activation,b->counts[6],a->count[6],NFLSTREET_TEAM_ACTIVATION_EFFECTS,sizeof b->activation[0]};
 q[NFLSTREET_TEAM_EFFECT_CONFIRMATION]=(queue_view){s->confirmation.effects,&s->confirmation.effect_count,b->confirmation,b->counts[7],a->count[7],NFLSTREET_TEAM_CONFIRM_EFFECTS,sizeof b->confirmation[0]};
 q[NFLSTREET_TEAM_EFFECT_TRANSITIONS]=(queue_view){s->transition_requests,&s->transition_count,b->transitions,b->counts[8],a->count[8],16,sizeof b->transitions[0]};
 q[NFLSTREET_TEAM_EFFECT_PLAYER_SPAWNS]=(queue_view){s->player_spawns,&s->player_spawn_count,b->player_spawns,b->counts[9],a->count[9],64,sizeof b->player_spawns[0]};
 q[NFLSTREET_TEAM_EFFECT_PLAYER_RESOURCES]=(queue_view){s->player_resources,&s->player_resource_count,b->player_resources,b->counts[10],a->count[10],1024,sizeof b->player_resources[0]};
 return 1;
}

static int state_counts_valid(const nflstreet_team_effect_batch*b,
 const queue_view q[NFLSTREET_TEAM_EFFECT_LANES]){
 for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i)
  if(q[i].captured_count>q[i].capacity)return 0;
 (void)b;return 1;
}

int nflstreet_team_effects_capture(const nflstreet_team_screen*s,nflstreet_team_effect_batch*out){
 if(!s||!out)return 0;
 nflstreet_team_screen*mutable_screen=(nflstreet_team_screen*)(void*)s;
 nflstreet_team_effect_batch result={0};queue_view q[NFLSTREET_TEAM_EFFECT_LANES];
 nflstreet_team_effect_ack none={0};
 if(!views(mutable_screen,&result,&none,q))return 0;
 for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i){
  if(*q[i].count>q[i].capacity)return 0;
  result.counts[i]=*q[i].count;
  if(result.counts[i])memcpy((void*)q[i].captured,q[i].queue,
      (size_t)result.counts[i]*q[i].element_size);
 }
 *out=result;return 1;
}

int nflstreet_team_effects_ack(nflstreet_team_screen*s,const nflstreet_team_effect_batch*b,
 const nflstreet_team_effect_ack*a){
 if(!s||!b||!a)return 0;queue_view q[NFLSTREET_TEAM_EFFECT_LANES];
 if(!views(s,b,a,q)||!state_counts_valid(b,q))return 0;
 /* Validate every lane before changing the first count or byte. This also
    rejects stale/tampered batches while allowing new suffix appends. */
 for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i){
  if(*q[i].count>q[i].capacity||*q[i].count<q[i].captured_count||
     q[i].ack_count>q[i].captured_count)return 0;
  if(q[i].captured_count&&memcmp(q[i].queue,q[i].captured,
       (size_t)q[i].captured_count*q[i].element_size))return 0;
 }
 for(unsigned i=0;i<NFLSTREET_TEAM_EFFECT_LANES;++i){
  size_t dropped=(size_t)q[i].ack_count*q[i].element_size;
  size_t remaining=(size_t)(*q[i].count-q[i].ack_count)*q[i].element_size;
  if(dropped&&remaining)memmove(q[i].queue,(uint8_t*)q[i].queue+dropped,remaining);
  *q[i].count-=q[i].ack_count;
 }
 return 1;
}
