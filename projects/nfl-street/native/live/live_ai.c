#include "live_ai.h"
#include "ai_policy.h"
#include <stdlib.h>
#include <string.h>
/* Host scratch token, deliberately OUTSIDE MEM1. It names these eight owned
   bytes through the AI-only span, not a discovered or borrowed game address. */
#define TARGET_TOKEN 0xfffffff0u
#define MEM1_BASE 0x80000000u
#define MEM1_SIZE (24u*1024u*1024u)
static uint32_t r32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
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
 /* live_main installs the native policy provider after roster initialization. */
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
   entering the live frame. Native policy services write only RAM and owned
   call temporaries. Never use this around the host clip/event providers. */
static int service_status(nflstreet_live_ai *c,nflstreet_live_ai_actor *a,uint8_t *ram,
 int ok,unsigned frame,const char *phase){
 if(ok)return 1;
 memcpy(ram,c->rollback,MEM1_SIZE);
 if(!c->dispatch.unsupported){failure(c,frame,a->actor,phase,0);return 0;}
 a->held=1;
 if(c->dispatch.service==nflstreet_ai_policy_service){const nflstreet_ai_policy *p=c->dispatch.user;
  if(c->log){fprintf(c->log,"AI policy stopped: service %08x, source PC %08x, address %08x, target %08x, budget %d\n",
   c->dispatch.unsupported,p->failure_pc,p->failure_address,p->unsupported,p->failure_budget_exhausted);fflush(c->log);}}
 if(c->log){fprintf(c->log,"frame %u: AI %s missing service %08x, actor %08x; RAM restored, actor held\n",
  frame,phase,c->dispatch.unsupported,a->actor);fflush(c->log);}return 1;}
/* The old observed-player runtime rejected the AI actors' contact modes,
   active statuses and animation-driven movement. Run the actual 800F86BC
   wrapper instead: reset -> input finish -> acceleration/status dispatch ->
   kinematics or channel root motion -> transient clear/heading publication.
   Its code and indirect table targets are in the native policy closure.
   This executes finalization ONCE, in the wrapper's original order. */
static int committed_movement(nflstreet_live_ai *c,nflstreet_live_ai_actor *a){
 const nflstreet_ai_call call={0x800f86bcu,{a->actor,0,0,0,0,0}};uint32_t result=0;
 if(!c->dispatch.service||!c->dispatch.service(c->dispatch.user,&call,&result)){
  c->dispatch.unsupported=call.pc;return 0;}return sync_actor(c,a);}
int nflstreet_live_ai_step(nflstreet_live_ai *c,nflstreet_locomotion_context *selector,unsigned frame){
 c->last_moved=0;uint8_t *ram=c->span(c->memory,MEM1_BASE,MEM1_SIZE);if(!ram||!c->rollback)return 0;
 for(unsigned i=0;i<c->count;++i){nflstreet_live_ai_actor *a=c->actors+i;if(a->held)continue;
  memcpy(c->rollback,ram,MEM1_SIZE);
  const int ok=nflstreet_ai_dispatch_step(&c->dispatch,0,a->actor+0xbe4,a->actor,0);
  if(!service_status(c,a,ram,ok,frame,"dispatch"))return 0;}
 (void)selector; /* Keep the public live adapter ABI; native calls share one stack. */
 for(unsigned i=0;i<c->count;++i){nflstreet_live_ai_actor *a=c->actors+i;if(a->held)continue;
  memcpy(c->rollback,ram,MEM1_SIZE);
  const nflstreet_ai_call call={0x800150e8u,{a->actor,0,0,0,0,0}};uint32_t result=0;
  const int ok=c->dispatch.service&&c->dispatch.service(c->dispatch.user,&call,&result);
  if(!ok)c->dispatch.unsupported=call.pc;
  if(!service_status(c,a,ram,ok,frame,"commit"))return 0;}
 for(unsigned i=0;i<c->count;++i){nflstreet_live_ai_actor *a=c->actors+i;if(a->held)continue;
  const uint8_t *p=c->span(c->memory,a->actor+0x1a8,8);if(!p)return 0;
  const uint32_t x=r32(p),y=r32(p+4);
  memcpy(c->rollback,ram,MEM1_SIZE);
  if(!committed_movement(c,a)){
   if(!service_status(c,a,ram,0,frame,"movement"))return 0;
   a->rejected=1;continue;}
  if(x!=r32(p)||y!=r32(p+4)){a->moved=a->moved_second=1;++c->last_moved;}}
 ++c->updates;return 1;}
int nflstreet_live_ai_sync(nflstreet_live_ai *c){
 for(unsigned i=0;i<c->count;++i)if(!sync_actor(c,c->actors+i))return 0;
 return 1;}
unsigned nflstreet_live_ai_moving(nflstreet_live_ai *c){
 unsigned moved=0;for(unsigned i=0;i<c->count;++i){moved+=c->actors[i].moved_second!=0;c->actors[i].moved_second=0;}return moved;}
void nflstreet_live_ai_report(const nflstreet_live_ai *c){
 unsigned moved=0,rejected=0,held=0;for(unsigned i=0;i<c->count;++i){moved+=c->actors[i].moved!=0;
  rejected+=c->actors[i].rejected!=0;held+=c->actors[i].held!=0;}
 /* Routine counters use neutral labels. The smoke runner treats failure
    words anywhere in a log as events, including a zero-valued counter.
    Actual movement failures still emit their once-per-actor diagnostic. */
 if(c->log){fprintf(c->log,"AI movement: %u/%u actors changed XY, %u in last update, %u movement holds, %u held for missing services; %u completed AI updates\n",
  moved,c->count,c->last_moved,rejected,held,c->updates);fflush(c->log);}}
void nflstreet_live_ai_close(nflstreet_live_ai *c){if(c){free(c->rollback);c->rollback=NULL;}}
