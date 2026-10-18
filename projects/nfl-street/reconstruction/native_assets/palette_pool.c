#include "palette_pool.h"
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint64_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
static uint8_t *globals(const nflstreet_palette_pool_context *x){
 if(!x||(x->globals&3u))return NULL;return span(x->ram,x->globals,20);}
static uint8_t *descriptor(const nflstreet_palette_pool_context *x,uint32_t palette){
 if(!x||(palette&3u))return NULL;return span(x->ram,palette,12);}
int nflstreet_palette_pool_flip(const nflstreet_palette_pool_context *x){
 uint8_t *g=globals(x);if(!g)return 0;uint32_t previous=rd(g+8);
 wr(g+16,0);wr(g+8,previous==0);return 1;}
int nflstreet_palette_pool_create(const nflstreet_palette_pool_context *x,uint32_t capacity,uint32_t multiplier){
 uint8_t *g=globals(x);if(!g||!x->services||!x->services->heap)return 0;
 uint64_t matrices=(uint64_t)capacity*multiplier;if(matrices>UINT32_MAX/48u)return 0;
 uint32_t base=x->services->heap(x->services->user,0,(uint32_t)matrices*48u,0);
 wr(g,base);wr(g+4,base);
 if(multiplier==2)wr(g+4,base+capacity*48u);
 wr(g+12,capacity);return nflstreet_palette_pool_flip(x);}
int nflstreet_palette_pool_reserve(const nflstreet_palette_pool_context *x,uint32_t palette,uint32_t requested){
 uint8_t *g=globals(x),*p=descriptor(x,palette);if(!g||!p)return 0;
 uint32_t active=rd(g+8);if(active>1u)return 0;
 uint32_t cursor=rd(g+16),rounded=(requested+1u)&0xfffffffeu,base=rd(g+4u*active);
 uint32_t data=base+cursor*48u;
 wr(g+16,cursor+rounded);wr(p+8,rounded);wr(p,data);return 1;}
uint8_t *nflstreet_palette_pool_span(const nflstreet_palette_pool_context *x,uint32_t palette,size_t *size){
 if(!size)return NULL;*size=0;uint8_t *p=descriptor(x,palette);if(!p)return NULL;
 uint64_t bytes=(uint64_t)rd(p+8)*48u;uint8_t *data=span(x->ram,rd(p),bytes);if(!data)return NULL;
 *size=(size_t)bytes;return data;}
