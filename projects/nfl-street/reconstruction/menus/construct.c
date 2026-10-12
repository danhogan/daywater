#include "construct.h"
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void putsh(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
typedef struct {uint8_t*b;size_t n;unsigned base,property,mask,budget;uint8_t visiting[1024];nflstreet_ui_script_service service;void*ctx;} walk;
static uint8_t*ptr(walk*w,unsigned a,size_t n){if(a<w->base||a-w->base>w->n||n>w->n-(a-w->base))return NULL;return w->b+a-w->base;}
static int callback(walk*w,uint8_t*row){unsigned cls=sh(row);if(cls>=16||!(w->mask&(1u<<cls)))return 1;
 unsigned cell=be(row+4),target=0;if(cell){uint8_t*p=ptr(w,cell,4);if(!p||be(p)>UINT32_MAX-w->base)return 0;target=be(p)+w->base;if(!ptr(w,target,1))return 0;}
 nflstreet_ui_service_request q={0};q.kind=0;q.class_id=cls;q.target=target;q.property=w->property;unsigned ignored=0;return w->service(w->ctx,w->b,w->n,w->base,&q,&ignored);
}
static int node(walk*w,unsigned index){
 if(index>=1024||index>=be(w->b)||!w->budget--||w->visiting[index])return 0;w->visiting[index]=1;
 uint8_t*nodes=ptr(w,be(w->b+4),(size_t)be(w->b)*20);if(!nodes)return 0;uint8_t*r=nodes+index*20;
 unsigned count=be(w->b+24);uint8_t*globals=ptr(w,be(w->b+28),(size_t)count*8);if(!globals)return 0;
 for(unsigned i=0;i<count;++i){uint8_t*g=globals+i*8;if(!sh(g+2)){if(!callback(w,g))return 0;putsh(g+2,1);}}
 count=be(r+4);uint8_t*groups=ptr(w,be(r+8),(size_t)count*4);if(!groups)return 0;
 for(unsigned i=0;i<count;++i){uint8_t*group=ptr(w,be(groups+i*4),12);if(!group)return 0;unsigned links=be(group+4);uint8_t*rows=ptr(w,be(group+8),(size_t)links*8);if(!rows)return 0;
  for(unsigned j=0;j<links;++j){uint8_t*l=rows+j*8;unsigned cls=sh(l);if(cls==65535){if(!node(w,be(l+4)))return 0;}
   else if(cls<16&&(w->mask&(1u<<cls))){if(!callback(w,l))return 0;putsh(l+2,w->property==UINT32_MAX);}
  }
 }w->visiting[index]=0;return 1;
}
int nflstreet_ui_construct(uint8_t*b,size_t n,uint32_t base,unsigned root,uint32_t property,uint16_t mask,nflstreet_ui_script_service service,void*ctx,size_t ctxsize){
 if(!b||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||!be(b)||be(b)>1024||!service||!ctx||!ctxsize||ctxsize>8*1024*1024)return 0;
 uint8_t*copy=malloc(n);void*state=malloc(ctxsize);if(!copy||!state){free(copy);free(state);return 0;}memcpy(copy,b,n);memcpy(state,ctx,ctxsize);walk w={0};w.b=copy;w.n=n;w.base=base;w.property=property;w.mask=mask;w.budget=16384;w.service=service;w.ctx=state;
 int ok=node(&w,root);if(ok){memcpy(b,copy,n);memcpy(ctx,state,ctxsize);}free(copy);free(state);return ok;
}
