#include "object_pool.h"
#include <limits.h>
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint64_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
uint32_t nflstreet_pool_align(uint32_t n,uint32_t a){return(n-1u+a)&(0u-a);}
int nflstreet_pool_layout(uint32_t n,uint32_t stride,uint32_t a,uint32_t *indices,uint32_t *data,uint32_t *total){
 if(!indices||!data||!total)return 0;
 *indices=24;*data=nflstreet_pool_align(24u+n*(n<=255u?1u:2u),a);*total=*data+n*stride;return 1;}
static uint32_t width(const uint8_t *p){return rd(p)<=255u?1u:2u;}
static uint8_t *header(nflstreet_pool_ram r,uint32_t p){return(p&3u)?NULL:span(r,p,24);}
static uint8_t *slot(nflstreet_pool_ram r,const uint8_t *p,uint32_t count){
 uint32_t w=width(p);uint64_t a=(uint64_t)rd(p+16)+(uint64_t)count*w;
 if(a>UINT32_MAX||(w==2&&(a&1u)))return NULL;
 uint8_t *q=span(r,(uint32_t)a,w);if(q&&q<p+24&&q+w>p)return NULL;return q;}
int nflstreet_pool_push(nflstreet_pool_ram r,uint32_t pool,uint32_t index){
 uint8_t *p=header(r,pool);if(!p)return 0;uint32_t count=rd(p+8);uint8_t *q=slot(r,p,count);if(!q)return 0;
 if(width(p)==1)q[0]=(uint8_t)index;else {q[0]=(uint8_t)(index>>8);q[1]=(uint8_t)index;}
 wr(p+8,count+1u);return 1;}
int nflstreet_pool_reset(nflstreet_pool_ram r,uint32_t pool){
 uint8_t *p=header(r,pool);if(!p)return 0;uint32_t n=rd(p);
 /* Host preflight: prevent a partially reset invalid span, not an original guard. */
 if(n&&(!slot(r,p,0)||!slot(r,p,n-1u)))return 0;
 wr(p+8,0);for(uint32_t i=0;i<n;++i)if(!nflstreet_pool_push(r,pool,i))return 0;return 1;}
int nflstreet_pool_pop(nflstreet_pool_ram r,uint32_t pool,uint32_t *result){
 uint8_t *p=header(r,pool);if(!p||!result)return 0;uint32_t n=rd(p+8);
 if(!n){*result=UINT32_MAX;return 1;}uint8_t *q=slot(r,p,n-1u);if(!q)return 0;
 wr(p+8,n-1u);*result=width(p)==1?q[0]:((uint32_t)q[0]<<8|q[1]);return 1;}
int nflstreet_pool_construct(nflstreet_pool_ram r,uint32_t owner,uint32_t n,uint32_t stride,
 uint32_t supplied,uint32_t a,const nflstreet_pool_services *s,uint32_t *result){
 uint32_t indices,data,total;
 if(!result||!s||!s->error||(!supplied&&!s->heap)||!a||(a&(a-1u)))return 0;
 nflstreet_pool_layout(n,stride,a,&indices,&data,&total);
 if((uint64_t)24+n*(n<=255u?1ull:2ull)+a-1u>UINT32_MAX||(uint64_t)data+(uint64_t)n*stride>UINT32_MAX)return 0;
 uint32_t pool=supplied?supplied:s->heap(s->user,owner,total,0);
 if(!pool){*result=0;return 1;}
 if(!header(r,pool)||!span(r,pool,total)||total<24)return 0;
 uint8_t *p=header(r,pool);
 wr(p,n);wr(p+4,stride);wr(p+12,owner);wr(p+16,pool+indices);wr(p+20,pool+data);
 if(!nflstreet_pool_reset(r,pool))return 0;
 s->error(s->user,0);*result=pool;return 1;}
int nflstreet_pool_allocate(nflstreet_pool_ram r,uint32_t pool,const nflstreet_pool_services *s,uint32_t *result){
 if(!s||!s->error||!result)return 0;uint32_t index;
 if(!nflstreet_pool_pop(r,pool,&index))return 0;
 if(index==UINT32_MAX){s->error(s->user,0x30003u);*result=0;return 1;}
 uint8_t *p=header(r,pool);uint32_t address=rd(p+20)+index*rd(p+4);
 s->error(s->user,0);*result=address;return 1;}
