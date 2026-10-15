#include "anim_stream.h"
#include "draw_engine.h"
#include <string.h>
#define R13 0x803a9a40u
static const uint8_t *at(nflstreet_draw_span s,void *m,uint32_t a,uint32_t n){return a?s(m,a,n):NULL;}
static int word(nflstreet_draw_span s,void *m,uint32_t a,uint32_t *v){const uint8_t *p=at(s,m,a,4);if(!p)return 0;*v=nflstreet_rd32(p);return 1;}
static uint32_t file32(const uint8_t *f,size_t n,size_t o){return o+4<=n?nflstreet_rd32(f+o):0xffffffffu;}
/* One bank's base: every saved frame word (an ARAM address, or a cache slot
   whose record holds the ARAM address) minus the file's frame offset. */
static int base_of(nflstreet_draw_span s,void *m,uint32_t header,uint32_t cache,unsigned index,const uint8_t *f,size_t fn,uint32_t *base,uint32_t *first){
 uint32_t table,records=0,slots=0,frames=nflstreet_rd16(f+4);int have=0;const size_t ft=file32(f,fn,16);*first=0xffffffffu;
 if(!word(s,m,header+0x10,&table)||ft==0xffffffffu||ft+4u*(size_t)frames>fn)return 0;
 if(cache){const uint8_t *c=at(s,m,cache,8);if(!c)return 0;slots=nflstreet_rd16(c);records=nflstreet_rd32(c+4);}
 for(uint32_t i=0;i<frames;++i){uint32_t w,aram;if(!word(s,m,table+4*i,&w))return 0;
  if((w&0xf0000000u)==0xf0000000u){const uint32_t k=w&0x0fffffffu;const uint8_t *r=k<slots?at(s,m,records+0x14u*k,0x14):NULL;
   if(!r)return 0;if(r[0xf]!=1||r[0xe]!=index||nflstreet_rd16(r+0xc)!=i)continue; /* a stale slot word (the slot now holds another frame) */
   aram=nflstreet_rd32(r+8);}
  else aram=w;
  const uint32_t o=nflstreet_rd32(f+ft+4*i),b=aram-o;if(o<*first)*first=o;if(have&&b!=*base)return 0;*base=b;have=1;}
 return have;}
int nflstreet_anim_aram_map(nflstreet_anim_aram *a,nflstreet_draw_span s,void *m,const uint8_t *const files[],const size_t sizes[],unsigned count){
 uint32_t n,table;if(!a||!s||!word(s,m,R13-0x4f70u,&n)||!word(s,m,R13-0x4f6cu,&table)||n>0x1000u)return -1;memset(a,0,sizeof *a);
 for(unsigned i=0;i<n;++i){uint32_t header,cache;const uint8_t *e=at(s,m,table+0x1cu*i,0x1c),*h;if(!e)return -1;
  header=nflstreet_rd32(e+0xc);cache=nflstreet_rd32(e+0x18);if(nflstreet_rd16(e+0x14)==0x8000u||!header||!(h=at(s,m,header,0x20)))continue;
  if(!(nflstreet_rd16(h+6)&0x200u))continue;
  int named=0,found=0;
  for(unsigned k=0;k<count&&!found;++k){const uint8_t *f=files[k];uint32_t base,first;
   if(!f||sizes[k]<0x20||memcmp(f,h,6))continue;named=1;if(!base_of(s,m,header,cache,i,f,sizes[k],&base,&first))continue;
   if(a->count>=NFLSTREET_ANIM_BANKS)return -1;a->bank[a->count++]=(nflstreet_anim_aram_bank){i,base,first,f,sizes[k]};found=1;}
  if(named&&!found)return -1;} /* entries no file names (free table slots) stay unmapped */
 return (int)a->count;}
void nflstreet_anim_aram_read(const nflstreet_anim_aram *a,uint32_t address,uint8_t *to,uint32_t n){
 memset(to,0,n);
 for(unsigned k=0;a&&k<a->count;++k){const nflstreet_anim_aram_bank *b=&a->bank[k];
  for(uint32_t i=0;i<n;++i){const uint32_t o=address+i-b->base;if(o>=b->first&&o<b->size)to[i]=b->data[o];}}} /* base may wrap below 0: the file's head is not in ARAM */
const uint8_t *nflstreet_anim_aram_frame(const nflstreet_anim_aram *a,unsigned index,unsigned frame,size_t *available){
 for(unsigned k=0;a&&k<a->count;++k){const nflstreet_anim_aram_bank *b=&a->bank[k];if(b->index!=index)continue;
  const size_t ft=file32(b->data,b->size,16);if(frame>=nflstreet_rd16(b->data+4)||ft+4u*(size_t)frame+4>b->size)return NULL;
  const uint32_t o=nflstreet_rd32(b->data+ft+4u*frame);if(o>=b->size)return NULL;if(available)*available=b->size-o;return b->data+o;}
 return NULL;}
