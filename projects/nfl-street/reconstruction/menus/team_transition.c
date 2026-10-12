#include "team_transition.h"
#include <string.h>

static uint32_t get_be32(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put_be32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}

int nflstreet_team_transition_apply(uint8_t*object,size_t object_size,
 uint32_t delay,uint32_t clock_value,uint32_t audio_enabled,
 uint32_t broadcast_context,nflstreet_team_transition_effects*effects){
 if(!object||object_size<NFLSTREET_TEAM_TRANSITION_OBJECT_MIN_SIZE||!effects)return 0;
 nflstreet_team_transition_effects result={0};
 if(audio_enabled){result.audio_requested=1;result.audio_handle=get_be32(object);result.audio_delay=delay;}
 result.broadcast_context=broadcast_context;
 result.broadcast_property=0x80000003u;
 put_be32(object+0x0c,clock_value+delay);
 object[0x13]=3;
 *effects=result;return 1;
}

int nflstreet_team_transition_advance(uint8_t*object,size_t object_size,
 uint32_t clock_value,uint32_t audio_enabled,
 nflstreet_team_transition_completion*effects){
 if(!object||object_size<NFLSTREET_TEAM_TRANSITION_OBJECT_MIN_SIZE||!effects)return 0;
 nflstreet_team_transition_completion next={0};
 if(!object[0x10]&&object[0x13]==3&&get_be32(object+0x0c)<=clock_value){
  next.completed=1;
  if(audio_enabled){next.audio_stop_requested=1;next.audio_handle=get_be32(object);}
  object[0x10]=1;object[0x13]=1;
 }
 *effects=next;return 1;
}
