#ifndef NFLSTREET_AI_POLICY_TRANSACTION_H
#define NFLSTREET_AI_POLICY_TRANSACTION_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* Host containment, not a game structure or gameplay policy. Capture each
   MEM1 page before the first mutable span exposes it during an operation.
   Reads also capture pages: the span API intentionally does not distinguish
   reads from writes. Thus retained pointers and cross-page writes are safe.
   All mutations in the native services and existing dispatcher use this span.
   Call temporaries and caller-owned scratch are outside the MEM1 transaction.
   Single-threaded: do not mutate MEM1 concurrently with an active journal. */
#define NFLSTREET_AI_MEM1_BASE 0x80000000u
#define NFLSTREET_AI_MEM1_BYTES (24u*1024u*1024u)
#define NFLSTREET_AI_PAGE_BYTES 4096u
#define NFLSTREET_AI_PAGE_COUNT (NFLSTREET_AI_MEM1_BYTES/NFLSTREET_AI_PAGE_BYTES)
typedef uint8_t *(*nflstreet_ai_memory_span)(void *,uint32_t,uint32_t);
typedef struct {
 nflstreet_ai_memory_span span;void *memory;uint8_t *before;
 uint16_t pages[NFLSTREET_AI_PAGE_COUNT];uint8_t seen[NFLSTREET_AI_PAGE_COUNT/8u];
 unsigned count;int active;
} nflstreet_ai_transaction;
static inline int nflstreet_ai_transaction_init(nflstreet_ai_transaction *t,nflstreet_ai_memory_span span,void *memory){
 memset(t,0,sizeof *t);t->span=span;t->memory=memory;
 if(!span||!span(memory,NFLSTREET_AI_MEM1_BASE,NFLSTREET_AI_MEM1_BYTES))return 0;
 t->before=malloc(NFLSTREET_AI_MEM1_BYTES);return t->before!=NULL;}
static inline void nflstreet_ai_transaction_begin(nflstreet_ai_transaction *t){
 memset(t->seen,0,sizeof t->seen);t->count=0;t->active=1;}
static inline uint8_t *nflstreet_ai_transaction_span(nflstreet_ai_transaction *t,uint32_t a,uint32_t n){
 uint8_t *p=t->span(t->memory,a,n);if(!p||!t->active||!n)return p;
 if(a<NFLSTREET_AI_MEM1_BASE||(uint64_t)a-NFLSTREET_AI_MEM1_BASE+n>NFLSTREET_AI_MEM1_BYTES)return NULL;
 const unsigned first=(a-NFLSTREET_AI_MEM1_BASE)/NFLSTREET_AI_PAGE_BYTES;
 const unsigned last=(a-NFLSTREET_AI_MEM1_BASE+n-1u)/NFLSTREET_AI_PAGE_BYTES;
 for(unsigned page=first;page<=last;++page){const unsigned mask=1u<<(page&7u);
  if(t->seen[page>>3]&mask)continue;
  const unsigned offset=page*NFLSTREET_AI_PAGE_BYTES;
  const uint8_t *original=t->span(t->memory,NFLSTREET_AI_MEM1_BASE+offset,NFLSTREET_AI_PAGE_BYTES);
  if(!original)return NULL;
  memcpy(t->before+offset,original,NFLSTREET_AI_PAGE_BYTES);
  t->seen[page>>3]|=(uint8_t)mask;t->pages[t->count++]=(uint16_t)page;}
 return p;}
static inline int nflstreet_ai_transaction_end(nflstreet_ai_transaction *t,int commit){
 t->active=0;if(commit)return 1;
 for(unsigned i=0;i<t->count;++i){const unsigned offset=(unsigned)t->pages[i]*NFLSTREET_AI_PAGE_BYTES;
  uint8_t *p=t->span(t->memory,NFLSTREET_AI_MEM1_BASE+offset,NFLSTREET_AI_PAGE_BYTES);
  if(!p)return 0;memcpy(p,t->before+offset,NFLSTREET_AI_PAGE_BYTES);}return 1;}
static inline void nflstreet_ai_transaction_close(nflstreet_ai_transaction *t){free(t->before);t->before=NULL;t->active=0;}
#endif
