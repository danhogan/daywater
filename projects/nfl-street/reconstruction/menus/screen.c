#include "screen.h"
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint8_t*ptr(uint8_t*b,size_t n,uint32_t base,uint32_t v,size_t size){if(v<base||(size_t)(v-base)>n||size>n-(v-base))return NULL;return b+(v-base);}
static int add(uint8_t*p,uint32_t base){uint32_t v=be(p);if(v>UINT32_MAX-base)return 0;put(p,v+base);return 1;}
int nflstreet_ui_relocate(uint8_t*b,size_t n,uint32_t base,uint32_t*out){
 if(!b||!out||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base)return 0;
 if(be(b+4)>=base){*out=UINT32_MAX;return 1;}
 uint8_t*c=malloc(n);if(!c)return 0;memcpy(c,b,n);int ok=0;
 if(!add(c+4,base))goto done;unsigned nodes=be(c);uint8_t*rows=ptr(c,n,base,be(c+4),(size_t)nodes*20);if(!rows)goto done;
 for(unsigned i=0;i<nodes;++i){uint8_t*r=rows+i*20;
  if(!add(r,base)||!add(r+8,base))goto done;
  unsigned count=be(r+4);uint8_t*list=ptr(c,n,base,be(r+8),(size_t)count*4);if(!list)goto done;
  for(unsigned j=count;j-- >0;){uint8_t*p=list+j*4;if(!add(p,base))goto done;uint8_t*object=ptr(c,n,base,be(p),12);if(!object||!add(object,base)||!add(object+8,base))goto done;
   unsigned k=be(object+4);uint8_t*links=ptr(c,n,base,be(object+8),(size_t)k*8);if(!links)goto done;
   for(unsigned q=k;q-- >0;){uint8_t*link=links+q*8;if(sh(link)!=0xffff){link[2]=link[3]=0;if(!add(link+4,base))goto done;}}
  }
  if(!add(r+16,base))goto done;count=be(r+12);uint8_t*events=ptr(c,n,base,be(r+16),(size_t)count*8);if(!events)goto done;
  for(unsigned j=count;j-- >0;){uint8_t*p=events+j*8;if(sh(p+2)!=0xffff&&!add(p+4,base))goto done;}
 }
 if(!add(c+12,base))goto done;unsigned count=be(c+8);uint8_t*strings=ptr(c,n,base,be(c+12),(size_t)count*12);if(!strings)goto done;
 for(unsigned i=count;i-- >0;)if(!add(strings+i*12+8,base))goto done;
 if(!add(c+20,base))goto done;count=be(c+16);uint8_t*fixups=ptr(c,n,base,be(c+20),(size_t)count*4);if(!fixups)goto done;
 for(unsigned i=count;i-- >0;){uint32_t off=be(fixups+i*4);if(off>n||n-off<4)goto done;uint32_t index=be(c+off);put(c+off,index<be(c+8)?be(c+12)+index*12:0);}
 if(!add(c+28,base))goto done;count=be(c+24);uint8_t*globals=ptr(c,n,base,be(c+28),(size_t)count*8);if(!globals)goto done;
 for(unsigned i=count;i-- >0;){uint8_t*p=globals+i*8+4;uint32_t off=be(p);if(off){if(off>n||n-off<4)goto done;put(p,be(c+off)?base+off:0);}}
 memcpy(b,c,n);*out=1;ok=1;
done:free(c);return ok;
}
int nflstreet_ui_event(const uint8_t*b,size_t n,uint32_t base,unsigned node,unsigned kind,uint16_t event,uint16_t id,uint32_t*out,unsigned*found){
 if(!b||!out||!found||n<32||node>=be(b)||kind>2)return 0;
 const uint8_t*r=ptr((uint8_t*)b,n,base,be(b+4),(size_t)be(b)*20);if(!r)return 0;r+=node*20;
 unsigned count=be(r+12);const uint8_t*p=ptr((uint8_t*)b,n,base,be(r+16),(size_t)count*8);if(!p)return 0;
 for(unsigned i=0;i<count;++i){const uint8_t*t=p+i*8;unsigned flags=sh(t);if(sh(t+2)!=event)continue;
  if(kind==0?!(flags&0x8000):kind==1?!(flags&0x4000):((flags&0xc000)||(flags&0x2fff)!=id))continue;
  uint32_t v=be(t+4);if(!v){*found=0;*out=0;return 1;}if(v<base||v-base>=n)return 0;*found=1;*out=v-base;return 1;
 }*out=0;*found=0;return 1;
}
