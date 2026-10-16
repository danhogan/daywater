#ifndef NFLSTREET_AI_POLICY_HOST_H
#define NFLSTREET_AI_POLICY_HOST_H
#include "actor_ai_dispatch.h"
#include <string.h>
/* GNNE69 r0, host execution choice; candidate pending original comparisons.
   The saved RAM alone does not supply the host's OS caller identity. In
   context 15, 800000E4 names a READY record while another active record is
   RUNNING and owns the mixer mutex. Borrow the sole RUNNING identity, or
   the first registered identity during idle captures, for synchronous calls.
   This does not resume a captured thread, steal/reinitialize any mutex, or
   change its queue/ownership/recursion count. Restore the saved identity.

   Source evidence: 801F4290 reads the caller at 800000E4; 801F4128 owns
   the active-list head at 800000DC and +2FC next links. 801F45D0 writes
   state 2 at +2C8 (801F47B0) BEFORE publishing that thread at 800000E4
   (801F47C8). 801F2C3C's recursive path compares its mutex +8 to E4.
   The 256-record traversal limit is a host validation bound, not a game
   enum or discovered capacity. Ambiguous/cyclic lists fail, never guess. */
typedef struct {
 nflstreet_actor_ai ram;
 uint32_t previous,selected,bad_address;
 int active;
} nflstreet_ai_policy_thread_scope;
static inline uint32_t nflstreet_ai_host_be32(const uint8_t *p){
 return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static inline void nflstreet_ai_host_put32(uint8_t *p,uint32_t v){
 p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static inline int nflstreet_ai_policy_thread_begin(nflstreet_ai_policy_thread_scope *s,nflstreet_actor_ai ram){
 memset(s,0,sizeof *s);s->ram=ram;s->bad_address=0x800000dcu;
 if(!ram.span)return 0;
 const uint8_t *p=ram.span(ram.memory,0x800000dcu,4);if(!p)return 0;
 const uint32_t head=nflstreet_ai_host_be32(p);if(!head)return 0;
 uint32_t visited[256],running=0,thread=head;unsigned count=0;
 while(thread){
  s->bad_address=thread;
  if((uint64_t)thread+0x300u>0x100000000ull)return 0;
  if(count==256)return 0;
  for(unsigned i=0;i<count;++i)if(visited[i]==thread)return 0;
  visited[count++]=thread;
  p=ram.span(ram.memory,thread+0x2c8u,0x38);if(!p)return 0;
  if(((unsigned)p[0]<<8|p[1])==2){if(running)return 0;running=thread;}
  thread=nflstreet_ai_host_be32(p+0x34);
 }
 s->selected=running?running:head;s->bad_address=0x800000e4u;
 uint8_t *current=ram.span(ram.memory,0x800000e4u,4);if(!current)return 0;
 s->previous=nflstreet_ai_host_be32(current);nflstreet_ai_host_put32(current,s->selected);
 s->active=1;s->bad_address=0;return 1;}
static inline int nflstreet_ai_policy_thread_end(nflstreet_ai_policy_thread_scope *s){
 if(!s->active)return 1;
 uint8_t *p=s->ram.span(s->ram.memory,0x800000e4u,4);
 if(!p){s->bad_address=0x800000e4u;return 0;}
 nflstreet_ai_host_put32(p,s->previous);s->active=0;return 1;}
#endif
