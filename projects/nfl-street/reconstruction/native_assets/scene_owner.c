#include "scene_owner.h"
#include <string.h>
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint64_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
static uint8_t *descriptor(const nflstreet_scene_owner_context *x,uint32_t type){
 if(!x||type||!x->alignment||(x->alignment&(x->alignment-1u))||(x->type_table&3u))return NULL;
 uint8_t *p=span(x->ram,x->type_table,48);
 if(!p||rd(p)!=0x80180c20u||rd(p+0x24)<32)return NULL;return p;}
int nflstreet_scene_list_layout(const nflstreet_scene_owner_context *x,uint32_t type,
 uint32_t capacity,uint32_t element_size,uint32_t *stride,uint32_t *total){
 uint8_t *d=descriptor(x,type);if(!d||!stride||!total)return 0;
 *stride=nflstreet_pool_align(element_size,x->alignment)+rd(d+0x28);
 uint32_t indices,data,pool;nflstreet_pool_layout(capacity,*stride,x->alignment,&indices,&data,&pool);
 *total=pool+nflstreet_pool_align(rd(d+0x24),x->alignment);return 1;}
int nflstreet_scene_list_create(const nflstreet_scene_owner_context *x,uint32_t owner,
 uint32_t type,uint32_t capacity,uint32_t element_size,uint32_t callback,uint32_t supplied,uint32_t *result){
 if(!result)return 0;*result=0;uint8_t *d=descriptor(x,type);
 if(!d||!x->services||!x->services->error||(!supplied&&!x->services->heap))return 0;
 uint32_t aligned=nflstreet_pool_align(element_size,x->alignment),stride,total;
 nflstreet_scene_list_layout(x,type,capacity,element_size,&stride,&total);
 uint32_t header=nflstreet_pool_align(rd(d+0x24),x->alignment);
 uint64_t index_bytes=24ull+capacity*(capacity<=255u?1ull:2ull);
 if((uint64_t)element_size+x->alignment-1u>UINT32_MAX||(uint64_t)aligned+rd(d+0x28)>UINT32_MAX||
 (uint64_t)rd(d+0x24)+x->alignment-1u>UINT32_MAX||index_bytes+x->alignment-1u>UINT32_MAX)return 0;
 if(capacity&&stride>UINT32_MAX/capacity)return 0;
 uint64_t need=header+((index_bytes+x->alignment-1u)&~((uint64_t)x->alignment-1u))+(uint64_t)capacity*stride;
 if(need>UINT32_MAX)return 0;
 uint32_t list=supplied?supplied:x->services->heap(x->services->user,owner,total,0);
 if(!list)return 1;
 uint8_t *p=span(x->ram,list,total);if(!p||(list&3u)||((list+header)&3u))return 0;
 wr(p+8,aligned);wr(p+12,owner);wr(p+4,0);wr(p+16,callback);wr(p,type);
 uint32_t pool;
 if(!nflstreet_pool_construct(x->ram,0,capacity,stride,list+header,x->alignment,x->services,&pool))return 0;
 wr(p+20,pool);
 /* 80180C20, decoded from pinned DOL bytes: zero+1C then+18. */
 wr(p+0x1c,0);wr(p+0x18,0);
 x->services->error(x->services->user,0);*result=list;return 1;}
int nflstreet_scene_owner_create(const nflstreet_scene_owner_context *x,uint32_t owner,
 uint32_t capacity,uint32_t type,uint32_t callback,uint32_t field0c,uint32_t multiplier,
 uint32_t field10,uint32_t field1c,uint32_t *result){
 if(!result)return 0;*result=0;
 if(!descriptor(x,type)||!x->services||!x->services->heap||!x->services->error)return 0;
 uint64_t product=(uint64_t)multiplier*x->registry_count;
 if(product>(UINT32_MAX-40u)/4u)return 0;uint64_t bytes=product*4u;
 uint32_t scene=x->services->heap(x->services->user,owner,(uint32_t)bytes+40u,0);
 if(!scene)return 1;uint8_t *p=span(x->ram,scene,bytes+40u);if(!p||(scene&3u))return 0;
 if(!callback)callback=0x80194f04u;
 uint32_t list;
 if(!nflstreet_scene_list_create(x,owner,type,capacity,4,callback,0,&list))return 0;
 wr(p,list);wr(p+4,type);wr(p+8,callback);wr(p+12,field0c);wr(p+16,field10);
 wr(p+20,multiplier);wr(p+24,owner);wr(p+28,field1c);wr(p+32,0);
 if(bytes)memset(p+36,0,(size_t)bytes);
 *result=scene;return 1;}
