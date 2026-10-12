#include "broadcast.h"
#include "screen.h"
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static const uint8_t*ptr(uint8_t*b,size_t n,unsigned base,unsigned a,size_t z){if(a<base||a-base>n||z>n-(a-base))return NULL;return b+a-base;}
typedef struct {uint8_t*b;size_t n;unsigned base,flags,property,argc;const unsigned*args;unsigned executed,budget;uint8_t visiting[1024];nflstreet_ui_script_service service;void*ctx;size_t ctxsize;} dispatch;
static int node(dispatch*d,unsigned index){
 if(index>=1024||index>=be(d->b)||!d->budget--||d->visiting[index])return 0;d->visiting[index]=1;
 const uint8_t*nodes=ptr(d->b,d->n,d->base,be(d->b+4),(size_t)be(d->b)*20);if(!nodes)return 0;const uint8_t*r=nodes+index*20;
 unsigned events=be(r+12);const uint8_t*list=ptr(d->b,d->n,d->base,be(r+16),(size_t)events*8);if(!list)return 0;
 for(unsigned i=0;i<events;++i)if(sh(list+i*8+2)==65535&&!node(d,be(list+i*8+4)))return 0;
 unsigned script,found;if(!nflstreet_ui_event(d->b,d->n,d->base,index,1,(uint16_t)d->property,0,&script,&found))return 0;
 if(found){nflstreet_ui_script vm={0};vm.flags=d->flags;vm.pc=d->base+script;vm.words[vm.sp++]=0;for(unsigned i=0;i<d->argc;++i)vm.words[vm.sp++]=d->args[i];vm.words[vm.sp++]=be(r);vm.words[vm.sp++]=0;
  if(!nflstreet_ui_script_services(d->b,d->n,d->base,0x70000000,&vm,10000,d->service,d->ctx,d->ctxsize))return 0;d->flags=vm.flags;++d->executed;
 }d->visiting[index]=0;return 1;
}
int nflstreet_ui_broadcast(uint8_t*b,size_t n,uint32_t base,uint32_t*flags,uint16_t property,unsigned argc,const uint32_t*args,nflstreet_ui_script_service service,void*ctx,size_t ctxsize,unsigned*executed){
 if(!b||!flags||!executed||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||!be(b)||be(b)>1024||argc>16||(argc&&!args)||ctxsize>8*1024*1024||(ctxsize&&!ctx)||(service&&(!ctx||!ctxsize)))return 0;
 uint8_t*copy=malloc(n);void*services=ctxsize?malloc(ctxsize):NULL;if(!copy||(ctxsize&&!services)){free(copy);free(services);return 0;}memcpy(copy,b,n);if(ctxsize)memcpy(services,ctx,ctxsize);
 dispatch d={0};d.b=copy;d.n=n;d.base=base;d.flags=*flags;d.property=property;d.argc=argc;d.args=args;d.budget=4096;d.service=service;d.ctx=services;d.ctxsize=ctxsize;
 int ok=node(&d,0);if(ok){memcpy(b,copy,n);if(ctxsize)memcpy(ctx,services,ctxsize);*flags=d.flags;*executed=d.executed;}free(copy);free(services);return ok;
}
