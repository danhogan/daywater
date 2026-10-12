#include "team_actor.h"
#include <string.h>

static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}

int nflstreet_team_actor_service(void*ctx,uint8_t*b,size_t n,uint32_t base,
                                const nflstreet_ui_service_request*q,uint32_t*out){
 nflstreet_team_actor*s=ctx;
 if(!s||!b||!q||!out||q->kind!=0||q->class_id!=3||!base||n>UINT32_MAX-base||
    s->callback_registered>1||s->request_count>NFLSTREET_TEAM_ACTOR_REQUEST_CAPACITY)return 0;
 if(q->property==0){
  if(q->argc!=1||q->target<base||q->target-base>n||n-(q->target-base)<4)return 0;
  put32(b+q->target-base,q->args[0]);return 1;
 }
 if(q->property==UINT32_MAX||q->property==0xfffffffcu)return 1;
 if(q->property!=0xfffffffeu||q->argc||q->target<base||q->target-base>n||
    n-(q->target-base)<20)return 0;
 if(!s->callback_registered)return 1;
 if(s->request_count>=NFLSTREET_TEAM_ACTOR_REQUEST_CAPACITY)return 0;
 const uint8_t*object=b+q->target-base;
 nflstreet_team_actor_request r={0};r.selected_index=be32(object);
 r.float_words[0]=be32(object+8);r.float_words[1]=be32(object+4);
 r.float_words[2]=be32(object+12);r.float_words[3]=be32(object+16);
 s->requests[s->request_count++]=r;return 1;
}
