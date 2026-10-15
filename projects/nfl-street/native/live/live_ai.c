#include "live_ai.h"
#include <stdlib.h>
#include <string.h>
/* Host scratch token, deliberately OUTSIDE MEM1. It names these eight owned
   bytes through the AI-only span, not a discovered or borrowed game address. */
#define TARGET_TOKEN 0xfffffff0u
#define MEM1_BASE 0x80000000u
#define MEM1_SIZE (24u*1024u*1024u)
static uint32_t r32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void w32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *ai_span(void *user,uint32_t a,uint32_t n){
 nflstreet_live_ai *c=user;
 if(a>=TARGET_TOKEN&&(uint64_t)a-TARGET_TOKEN+n<=sizeof c->scratch)return c->scratch+(a-TARGET_TOKEN);
 return c->span(c->memory,a,n);}
static int sync_actor(nflstreet_live_ai *c,nflstreet_live_ai_actor *a){
 const uint8_t *p=c->span(c->memory,a->actor,0x14d4);if(!p)return 0;
 for(unsigned j=0;j<12;++j)a->movement.runtime.actor[j]=r32(p+0x200+4*j);
 for(unsigned j=0;j<21;++j)a->movement.runtime.state[j]=r32(p+0x1a8+4*j);
 a->movement.runtime.header=r32(p);
 a->movement.runtime.rating_bb4=(int16_t)((unsigned)p[0xbb4]<<8|p[0xbb5]);
 a->movement.runtime.rating_bbc=(int16_t)((unsigned)p[0xbbc]<<8|p[0xbbd]);
 a->movement.runtime.actor_408=r32(p+0x408);return 1;}
int nflstreet_live_ai_init(nflstreet_live_ai *c,nflstreet_draw_span span,void *memory,uint32_t r13,
 uint32_t roster,unsigned count,uint32_t controlled,FILE *log){
 if(!c||!span||count>14)return 0;
 memset(c,0,sizeof *c);c->span=span;c->memory=memory;c->log=log;
 c->dispatch.ram=(nflstreet_actor_ai){ai_span,c,r13};c->dispatch.scratch=TARGET_TOKEN;
 /* No original-backed test provider belongs in the live application. The
    dispatcher's missing-service failure identifies the exact remaining PC. */
 for(unsigned i=0;i<count;++i){const uint32_t g=roster+0x14d4u*i;const uint8_t *p=span(memory,g,0x14d4);
  if(!p)return 0;
  if(g==controlled||p[8]!=0xff)continue;
  nflstreet_live_ai_actor *a=c->actors+c->count++;a->actor=g;
  if(!sync_actor(c,a))return 0;
  nflstreet_actor_runtime r=a->movement.runtime;
  if(!nflstreet_live_movement_init(&a->movement,r.header,r.rating_bb4,r.rating_bbc,r.actor_408,r.actor,r.state))return 0;
  /* Same first-update reset as seed_movement for the controlled actor. */
  a->movement.seed.completed=a->movement.runtime.completed=1;
 }
 if(!span(memory,MEM1_BASE,MEM1_SIZE))return 0;
 c->rollback=malloc(MEM1_SIZE);if(!c->rollback)return 0;
 if(log){fprintf(log,"AI roster: %u non-controlled actors seeded\n",c->count);fflush(log);}return 1;}
static void failure(nflstreet_live_ai *c,unsigned frame,uint32_t g,const char *phase,uint32_t pc){
 if(c->log){fprintf(c->log,"frame %u: AI %s unsupported, actor %08x, pc %08x; update stopped\n",frame,phase,g,pc);fflush(c->log);}}
/* Host containment of unmodeled services. The native API intentionally keeps
   writes before failure; restoring ALL MEM1 prevents that partial result from
   entering the live frame. With no AI service provider there are no external
   policy effects to undo. Never use this around the clip/event providers. */
static int service_status(nflstreet_live_ai *c,nflstreet_live_ai_actor *a,uint8_t *ram,
 int ok,unsigned frame,const char *phase){
 if(ok)return 1;
 memcpy(ram,c->rollback,MEM1_SIZE);
 if(!c->dispatch.unsupported){failure(c,frame,a->actor,phase,0);return 0;}
 a->held=1;
 if(c->log){fprintf(c->log,"frame %u: AI %s missing service %08x, actor %08x; RAM restored, actor held\n",
  frame,phase,c->dispatch.unsupported,a->actor);fflush(c->log);}return 1;}
/* The movement wrapper's documented tail clears both transient status bytes
   and publishes the resulting heading to *(actor+31C)+8. Work on a copy so
   rejected movement cannot partly advance either RAM or the retained runtime. */
static int committed_movement(nflstreet_live_ai *c,nflstreet_live_ai_actor *a){
 if(!sync_actor(c,a))return 0;
 uint8_t *p=c->span(c->memory,a->actor,0x14d4);if(!p)return 0;
 /* 800F86BC's flag-100 tail calls 800371E4, outside this runtime. */
 if(r32(p+0xc)&0x100u)return 0;
 const uint32_t root=r32(p+0x31c);if(root>UINT32_MAX-8)return 0;
 uint8_t *heading=c->span(c->memory,root+8,4);if(!heading)return 0;
 nflstreet_actor_runtime next=a->movement.runtime;
 const unsigned status=(next.actor[7]>>8)&255u;
 if(status==0){if(!nflstreet_actor_runtime_brake(&next))return 0;}
 else if(status==1){uint32_t inputs[12];memcpy(inputs,next.actor,sizeof inputs);
  if(!nflstreet_actor_runtime_step(&next,inputs))return 0;}
 else return 0;
 next.actor[3]&=0xffff00ffu;next.actor[7]&=0xffff00ffu;
 a->movement.runtime=next;
 for(unsigned j=0;j<12;++j)w32(p+0x200+4*j,next.actor[j]);
 for(unsigned j=0;j<21;++j)w32(p+0x1a8+4*j,next.state[j]);
 w32(heading,next.state[6]);return 1;}
int nflstreet_live_ai_step(nflstreet_live_ai *c,nflstreet_locomotion_context *selector,unsigned frame){
 c->last_moved=0;uint8_t *ram=c->span(c->memory,MEM1_BASE,MEM1_SIZE);if(!ram||!c->rollback)return 0;
 for(unsigned i=0;i<c->count;++i){nflstreet_live_ai_actor *a=c->actors+i;if(a->held)continue;
  memcpy(c->rollback,ram,MEM1_SIZE);
  const int ok=nflstreet_ai_dispatch_step(&c->dispatch,0,a->actor+0xbe4,a->actor,0);
  if(!service_status(c,a,ram,ok,frame,"dispatch"))return 0;}
 for(unsigned i=0;i<c->count;++i){if(c->actors[i].held)continue;const uint32_t g=c->actors[i].actor;
  if(nflstreet_ai_commit(selector,g)!=1){failure(c,frame,g,"commit",selector->unsupported);return 0;}}
 for(unsigned i=0;i<c->count;++i){nflstreet_live_ai_actor *a=c->actors+i;if(a->held)continue;
  memcpy(c->rollback,ram,MEM1_SIZE);
  const int ok=nflstreet_ai_input_finish(&c->dispatch,a->actor);
  if(!service_status(c,a,ram,ok,frame,"input finish"))return 0;}
 for(unsigned i=0;i<c->count;++i){nflstreet_live_ai_actor *a=c->actors+i;if(a->held)continue;
  const uint8_t *p=c->span(c->memory,a->actor+0x1a8,8);if(!p)return 0;
  const uint32_t x=r32(p),y=r32(p+4);
  if(!committed_movement(c,a)){
   if(!a->rejected&&c->log){fprintf(c->log,"frame %u: AI movement takes an unreconstructed branch, actor %08x; held\n",frame,a->actor);fflush(c->log);}
   a->rejected=1;continue;}
  if(x!=r32(p)||y!=r32(p+4)){a->moved=1;++c->last_moved;}}
 ++c->updates;return 1;}
int nflstreet_live_ai_sync(nflstreet_live_ai *c){
 for(unsigned i=0;i<c->count;++i)if(!sync_actor(c,c->actors+i))return 0;
 return 1;}
void nflstreet_live_ai_report(const nflstreet_live_ai *c){
 unsigned moved=0,rejected=0,held=0;for(unsigned i=0;i<c->count;++i){moved+=c->actors[i].moved!=0;
  rejected+=c->actors[i].rejected!=0;held+=c->actors[i].held!=0;}
 if(c->log){fprintf(c->log,"AI movement: %u/%u actors changed XY, %u in last update, %u rejected movement, %u held for missing services; %u completed AI updates\n",
  moved,c->count,c->last_moved,rejected,held,c->updates);fflush(c->log);}}
void nflstreet_live_ai_close(nflstreet_live_ai *c){if(c){free(c->rollback);c->rollback=NULL;}}
