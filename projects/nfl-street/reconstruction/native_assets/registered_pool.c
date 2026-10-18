#include "registered_pool.h"
#include <string.h>
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint64_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
static uint8_t *record(const nflstreet_registered_context *x,uint32_t type){
 if(!x||type>=x->record_count||(x->registry_slot&3u))return NULL;
 uint8_t *root=span(x->ram,x->registry_slot,4);if(!root)return NULL;
 uint32_t table=rd(root);uint64_t address=(uint64_t)table+(uint64_t)type*20u;
 if(!table||(table&3u)||address>UINT32_MAX||x->record_count>UINT32_MAX/20u||
 !span(x->ram,table,(uint64_t)x->record_count*20u))return NULL;
 return span(x->ram,(uint32_t)address,20);}
int nflstreet_registered_register(const nflstreet_registered_context *x,uint32_t type,
 uint32_t size,uint32_t capacity,uint32_t ctor,uint32_t dtor,uint32_t *result){
 if(!result)return 0;*result=0;uint8_t *p=record(x,type);
 if(!p||!x->services||!x->services->pool.heap||!x->services->pool.error||!x->services->get_error||
 !x->alignment||(x->alignment&(x->alignment-1u)))return 0;
 uint32_t indices,data,total;
 nflstreet_pool_layout(capacity,size,x->alignment,&indices,&data,&total);
 if((uint64_t)24+capacity*(capacity<=255u?1ull:2ull)+x->alignment-1u>UINT32_MAX||
 (uint64_t)data+(uint64_t)capacity*size>UINT32_MAX)return 0;
 wr(p,size);wr(p+8,ctor);wr(p+12,dtor);wr(p+4,capacity);
 uint32_t pool;
 if(!nflstreet_pool_construct(x->ram,1,capacity,size,0,x->alignment,&x->services->pool,&pool))return 0;
 wr(p+16,pool);
 uint32_t code=pool?0:x->services->get_error(x->services->pool.user);
 x->services->pool.error(x->services->pool.user,code);*result=code;return 1;}
typedef struct {
 const nflstreet_registered_context *x;
 nflstreet_factory_record *host;
 uint32_t type,constructor;
 uint8_t *initial;
 int valid;
} join;
static int overlap(uint32_t a,uint64_t n,uint32_t b,uint64_t m){return (uint64_t)a<(uint64_t)b+m&&(uint64_t)b<(uint64_t)a+n;}
static void construct(void *user,nflstreet_factory_memory *memory,const void *input){
 join *j=user;j->x->services->construct(j->x->services->pool.user,j->constructor,memory->token,*(const uint32_t *)input);}
static int allocate(void *user,uint32_t owner,uint32_t minimum,nflstreet_factory_memory *memory){
 join *j=user;const nflstreet_registered_context *x=j->x;(void)minimum;uint32_t object;
 if(!nflstreet_pool_allocate(x->ram,owner,&x->services->pool,&object)){j->valid=0;return 0;}
 if(!object)return 0;
 /* Original reloads registry root and size after the allocation/error call.
    Refresh the exact scratch record actor_factory will next dereference. */
 uint8_t *p=record(x,j->type);if(!p){j->valid=0;return 0;}
 uint32_t size=rd(p),ctor=rd(p+8);uint8_t *bytes=span(x->ram,object,size);
 uint32_t initial=x->ram.base+(uint32_t)(j->initial-x->ram.bytes),current=x->ram.base+(uint32_t)(p-x->ram.bytes);
 if(size<0x12u||!bytes||(ctor&&!x->services->construct)||
 overlap(object,size,x->registry_slot,4)||overlap(object,size,initial,20)||overlap(object,size,current,20)){
  j->valid=0;return 0;}
 j->constructor=ctor;j->host->object_size=size;j->host->construct=ctor?construct:NULL;
 *memory=(nflstreet_factory_memory){object,bytes,size};return 1;}
int nflstreet_registered_create(const nflstreet_registered_context *x,nflstreet_factory_record *scratch,
 size_t scratch_count,uint32_t type,uint8_t header,uint32_t input,uint32_t *result){
 if(!result)return 0;*result=0;uint8_t *p=record(x,type);
 if(!p||!scratch||scratch_count<x->record_count||!x->services||!x->services->pool.error||rd(p)<0x12u)return 0;
 join j={x,scratch+type,type,rd(p+8),p,1};
 *j.host=(nflstreet_factory_record){rd(p),rd(p+16),NULL,&j};
 nflstreet_factory_memory memory;
 int ok=nflstreet_actor_create(scratch,x->record_count,type,header,&input,allocate,&j,&memory);
 /* Scratch callbacks point at this call's join; never expose dangling callbacks. */
 j.host->construct=NULL;j.host->construction_context=NULL;
 if(!j.valid)return 0;if(ok)*result=memory.token;return 1;}
