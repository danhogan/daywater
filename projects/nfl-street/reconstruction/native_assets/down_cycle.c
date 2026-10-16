#include "down_cycle.h"
/* Independently compiled, source-addressed functions. No runtime decoder,
   original-module provider, fabricated events or native oracle hooks. */
#include "down_cycle_native.inc"
static uint32_t dc_word(nflstreet_down_cycle *c,uint32_t a){
 const uint8_t *p=c->native.ram.span(c->native.ram.memory,a,4);
 if(!p){c->native.ok=0;c->native.failure_address=a;return 0;}
 return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned dc_byte(nflstreet_down_cycle *c,uint32_t a){
 const uint8_t *p=c->native.ram.span(c->native.ram.memory,a,1);
 if(!p){c->native.ok=0;c->native.failure_address=a;return 0;}return *p;}
static float dc_real(nflstreet_down_cycle *c,uint32_t a){uint32_t u=dc_word(c,a);float f;memcpy(&f,&u,4);return f;}
static uint32_t dc_field(nflstreet_down_cycle *c){return dc_word(c,c->native.ram.r13-0x7078u);}
static uint32_t dc_phase(nflstreet_down_cycle *c){return dc_word(c,dc_word(c,c->native.ram.r13-0x7cf8u));}
void nflstreet_down_cycle_init(nflstreet_down_cycle *c,nflstreet_actor_ai ram){
 if(c){memset(c,0,sizeof *c);nflstreet_down_native_init(&c->native,ram);}}
int nflstreet_down_cycle_call(nflstreet_down_cycle *c,uint32_t pc,const uint32_t args[6],double weight,uint32_t *result){
 if(!c||!args||!result)return 0;nflstreet_ai_call call={pc,{0}};memcpy(call.args,args,sizeof call.args);
 return nflstreet_down_native_host_call(&c->native,&call,weight,result);}
static int dc_call(nflstreet_down_cycle *c,uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t *v){
 const uint32_t args[6]={a,b,d,0,0,0};return nflstreet_down_cycle_call(c,pc,args,1.0,v);}
static const char *dc_reason(nflstreet_down_cycle *c,uint32_t result){
 const unsigned score=dc_byte(c,result+0x1du);const uint32_t flags=dc_word(c,result+0x18u);
 /* Strings/flags in 80043104 and events in 80025028. Diagnostic labels only. */
 if(score==6)return "touchdown";if(score==2)return "conversion";if(score==254)return "safety";
 if(flags&0x4000u)return "incompletion";if(flags&0x20000u)return "out of bounds";
 if(flags&0x100u)return "touchback";return "down by contact";}
static void dc_observe(nflstreet_down_cycle *c,nflstreet_down_cycle_event *e){
 const uint32_t f=dc_field(c);e->phase=dc_phase(c);e->down=dc_word(c,f+0x30u);
 e->possession=dc_byte(c,f+0x38u);e->flags=dc_word(c,f+0x34u);
 e->spot=dc_real(c,f+0x10u);e->to_go=dc_real(c,f+8u)-e->spot;
 const uint8_t *p=c->native.ram.span(c->native.ram.memory,f+0x3cu,4);
 if(!p){c->native.ok=0;c->native.failure_address=f+0x3cu;return;}
 e->scores[0]=(unsigned)p[0]<<8|p[1];e->scores[1]=(unsigned)p[2]<<8|p[3];}
int nflstreet_down_cycle_update(nflstreet_down_cycle *c,nflstreet_down_cycle_event *e){
 if(!c||!e||!c->native.ram.span)return 0;memset(e,0,sizeof *e);c->native.ok=1;
 const uint32_t phase=dc_phase(c);uint32_t v=0;e->previous_phase=phase;e->reason="live";
 if(!c->native.ok)return 0;
 if(phase==3){
  if(!dc_call(c,0x800be3f0u,0,0,0,&v)||!dc_call(c,0x800f9c50u,v,0,0,&v))return 0;
 }else if(phase==4){
  const uint32_t whistle=dc_word(c,c->native.ram.r13-0x7b98u);
  if(!c->native.ok||!dc_call(c,0x8004ebf0u,whistle,0,0,&v))return 0;
  if(!dc_byte(c,whistle+0x100u)&&!dc_call(c,0x8002e45cu,5,0,0,&v))return 0;
 }else if(phase!=2&&phase!=5){c->native.ok=0;c->native.unsupported=phase;return 0;}
 dc_observe(c,e);if(!c->native.ok)return 0;
 e->whistle=phase==3&&e->phase==4;e->select_play=e->phase==5;
 if(e->whistle){++c->whistles;const uint32_t w=dc_word(c,c->native.ram.r13-0x7b98u);
  const uint32_t result=w+0x104u;e->reason=dc_reason(c,result);
  /* Result is already calculated at whistle entry, committed by its queued
     callback after 45 ticks. Report its proposed down/spot at the whistle. */
  e->down=dc_word(c,result+4u);e->spot=dc_real(c,result+0xcu);
  e->to_go=dc_real(c,dc_field(c)+8u)-e->spot;}
 ++c->updates;return c->native.ok;}
int nflstreet_down_cycle_lineup(nflstreet_down_cycle *c){
 if(!c||dc_phase(c)!=5)return 0;uint32_t v;
 /* Same position helpers as 80048DA8. These run the actual play geometry. */
 if(!dc_call(c,0x800f2a20u,0,0,0,&v)||!dc_call(c,0x800f2a20u,1,0,0,&v)||
    !dc_call(c,0x800f8ae8u,0,0,0,&v)||!dc_call(c,0x800f8ae8u,1,0,0,&v))return 0;
 return dc_call(c,0x8002e45cu,2,0,0,&v);}
int nflstreet_down_cycle_snap(nflstreet_down_cycle *c){
 if(!c||dc_phase(c)!=2)return 0;uint32_t v,play,actor;
 const uint32_t offense=dc_byte(c,dc_field(c)+0x38u);
 if(!c->native.ok||!dc_call(c,0x800f2e7cu,offense,0,0,&play))return 0;
 const unsigned slot=dc_byte(c,play+0x16u);
 if(!c->native.ok||!dc_call(c,0x8003a4a0u,offense,slot,0,&actor)||!actor)return 0;
 if(!dc_call(c,0x800f946cu,0,0,0,&v))return 0;
 return dc_call(c,0x800815b4u,actor,0,0,&v);}
