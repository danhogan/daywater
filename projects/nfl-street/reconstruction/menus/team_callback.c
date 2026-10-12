#include "team_callback.h"
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int range(size_t n,uint32_t base,uint32_t addr,size_t z){return addr>=base&&(size_t)(addr-base)<=n&&z<=n-(addr-base);}
static int word(uint8_t*b,size_t n,uint32_t base,uint32_t sb,uint32_t*w,uint32_t addr,uint32_t*v,int write){
 if(range(n,base,addr,4)){if(write)put(b+addr-base,*v);else *v=be(b+addr-base);return 1;}
 if(range(1024,sb,addr,4)&&!((addr-sb)&3)){if(write)w[(addr-sb)/4]=*v;else *v=w[(addr-sb)/4];return 1;}return 0;
}
static int array(uint8_t*b,size_t n,uint32_t base,uint32_t sb,uint32_t*w,uint32_t descriptor,uint32_t*dest){
 uint32_t dims;if(!word(b,n,base,sb,w,descriptor,&dims,0)||dims>8||descriptor>UINT32_MAX-(dims+1)*4)return 0;
 uint32_t addr=descriptor+(dims+1)*4,ignored;
 if(addr>UINT32_MAX-8)return 0;
 for(unsigned i=0;i<3;++i)if(!word(b,n,base,sb,w,addr+4*i,&ignored,0))return 0;
 *dest=addr;return 1;
}
int nflstreet_team_callback_memory(void*ctx,uint8_t*b,size_t n,uint32_t base,
 const nflstreet_ui_service_request*q,uint32_t*out,uint32_t sb,uint32_t words[256]){
 nflstreet_team_callback*s=ctx;
 if(!s||!b||!q||!out||!words||q->kind!=1||!base||!sb||n>UINT32_MAX-base||sb>UINT32_MAX-1024u)return 0;
 /* Ambiguous screen/stack ownership is unsupported. */
 if(base<sb+1024u&&sb<(uint64_t)base+n)return 0;
 if(q->property==8u){
  if(q->argc!=3)return 0;const nflstreet_team_record*r=nflstreet_team_catalog_find(&s->catalog,q->args[0]);uint32_t ignored;
  /* Supplied common mode6, actor gatefalse; regular team types only.
     Original8010BE60 initializes output0 and badgeFFFF before lookup. */
  if(r&&(r->resource_fields!=3||r->raw_type==5))return 0;
  if(!word(b,n,base,sb,words,q->args[1],&ignored,0)||!word(b,n,base,sb,words,q->args[2],&ignored,0)||q->args[1]==q->args[2])return 0;
  uint32_t logo=r?r->logo_entry:0,badge=UINT32_MAX;
  return word(b,n,base,sb,words,q->args[1],&logo,1)&&word(b,n,base,sb,words,q->args[2],&badge,1);
 }
 if(q->property==0x0fu){
  if(q->argc!=1)return 0;uint32_t dest;if(!array(b,n,base,sb,words,q->args[0],&dest)||dest>UINT32_MAX-12)return 0;uint32_t value;
  if(!word(b,n,base,sb,words,dest+12,&value,0))return 0;
  /* Original8010C078 with explicitly supplied empty device/profile iterator. */
  for(unsigned i=0;i<4;++i){value=UINT32_MAX;if(!word(b,n,base,sb,words,dest+4*i,&value,1))return 0;}return 1;
 }
 if(q->property==0x0du){if(q->argc!=1||q->args[0]!=UINT32_MAX)return 0;*out=0x3e;return 1;}
 if(q->property==0x80000001u){
  if(q->argc)return 0;nflstreet_team_start_state next=s->startup;
  if(!nflstreet_team_start_apply(&s->catalog,s->query_mode,&next))return 0;
  s->startup=next;*out=0;return 1;
 }
 if(q->property==0x80000003u){
  if(q->argc!=5)return 0;const nflstreet_team_record*r=nflstreet_team_catalog_find(&s->catalog,q->args[0]);
  if(!r||!r->name_bytes||r->name_bytes>=sizeof r->name||!range(n,base,q->args[1],12))return 0;
  uint8_t*d=b+q->args[1]-base;uint32_t cap=be(d+4),dest=be(d+8),ignored;
  if(!range(n,base,dest,cap))return 0;
  for(unsigned i=2;i<5;++i){if(!word(b,n,base,sb,words,q->args[i],&ignored,0))return 0;
   if(cap&&q->args[i]>=dest-((dest>=4)?4:0)&&(uint64_t)q->args[i]<(uint64_t)dest+cap)return 0;
  }
  unsigned copied=cap<r->name_bytes?cap:r->name_bytes;
  memcpy(b+dest-base,r->name,copied);if(cap>copied)memset(b+dest-base+copied,0,cap-copied);
  for(unsigned i=0;i<3;++i){uint32_t value=r->fields[i];if(!word(b,n,base,sb,words,q->args[i+2],&value,1))return 0;}
  return 1;
 }
 if(q->property==0x80000005u){
  if(q->argc!=1||q->args[0]>1)return 0;unsigned side=q->args[0];nflstreet_team_carousel next=s->startup.carousel;
  nflstreet_team_carousel_catalog*c=&next.catalogs[side];if(!c->count||c->count>NFLSTREET_TEAM_CAROUSEL_MAX_IDS||c->cursor>=c->count)return 0;
  /* Original CCAC random response0; CD34 goes back once then reads/advances3. */
  c->cursor=(c->cursor+c->count-1)%c->count;
  for(unsigned i=0;i<3;++i){next.windows[side][i]=c->entries[c->cursor].id;c->cursor=(c->cursor+1)%c->count;}
  s->startup.carousel=next;return 1;
 }
 if(q->property!=0x80000004u||q->argc!=4)return 0;
 uint32_t values,flags;
 if(!array(b,n,base,sb,words,q->args[2],&values)||!array(b,n,base,sb,words,q->args[3],&flags))return 0;
 if(values<flags+12u&&flags<values+12u)return 0; /* Output alias branches unverified. */
 nflstreet_team_carousel next=s->startup.carousel;nflstreet_team_carousel_output result;
 if(!nflstreet_team_carousel_apply(&next,q->args[0],q->args[1],&result))return 0;
 /* Original8011CE64 writes values first, then flags. */
 for(unsigned i=0;i<3;++i)if(!word(b,n,base,sb,words,values+4*i,&result.values[i],1))return 0;
 for(unsigned i=0;i<3;++i)if(!word(b,n,base,sb,words,flags+4*i,&result.flags[i],1))return 0;
 s->startup.carousel=next;return 1;
}
