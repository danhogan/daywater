#include "scene_remove.h"
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint64_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
static uint8_t *node(nflstreet_pool_ram r,uint32_t a){return(a&3u)?NULL:span(r,a,20);}
static uint8_t *list_header(const nflstreet_scene_insert_context *x,uint32_t list){
 if(!x||(list&3u)||(x->type_table&3u))return NULL;uint8_t *p=span(x->ram,list,32),*d=span(x->ram,x->type_table,48);
 if(!p||!d||rd(p)||rd(d+0x10)!=0x80180d8cu||rd(d+0x14)!=0x80180e0cu||rd(d+0x18)!=0x80180e6cu||rd(d+0x28)!=16)return NULL;
 return p;}
int nflstreet_scene_match(nflstreet_pool_ram r,uint32_t payload,uint32_t *target,uint32_t *result){
 if(!result)return 0;*result=0;if(!target||(payload&3u))return 0;
 uint8_t *p=span(r,payload,4);if(!p)return 0;
 if(rd(p)==*target){*target=payload;*result=0;}else *result=1;return 1;}
int nflstreet_scene_find(const nflstreet_scene_insert_context *x,uint32_t list,uint32_t object,uint32_t *target,uint32_t *result){
 if(!target||!result)return 0;*target=0;*result=0;uint8_t *p=list_header(x,list);if(!p)return 0;
 *target=object;uint32_t current=rd(p+24);if(!current){*result=1;return 1;}
 for(size_t steps=0;current;++steps){if(steps>x->ram.size/16u)return 0;
  uint8_t *n=node(x->ram,current);if(!n)return 0;uint32_t next=rd(n+12),matched;
  if(!nflstreet_scene_match(x->ram,current+16u,target,&matched))return 0;
  if(!matched){*result=4;return 1;}current=next;
 }
 *result=0;return 1;}
int nflstreet_scene_pool_release(nflstreet_pool_ram r,uint32_t pool,uint32_t address){
 if(pool&3u)return 0;uint8_t *p=span(r,pool,24);if(!p||!rd(p+4))return 0;
 uint32_t index=(address-rd(p+20))/rd(p+4);
 return nflstreet_pool_push(r,pool,index);}
int nflstreet_scene_node_release(const nflstreet_scene_insert_context *x,uint32_t list,uint32_t payload){
 uint8_t *p=list_header(x,list);if(!p||payload<16)return 0;uint32_t base=payload-16;uint8_t *n=node(x->ram,base);if(!n)return 0;
 if(rd(n)&0x80000000u){
  uint32_t next=rd(n+12),prev=rd(n+8);uint8_t *after=next?node(x->ram,next):NULL,*before=prev?node(x->ram,prev):NULL;
  if((next&&!after)||(prev&&!before))return 0;
  /* Exact80180E0C order. Freed node's own links are intentionally untouched. */
  if(rd(p+24)==base)wr(p+24,next);
  if(rd(p+28)==base)wr(p+28,prev);
  if(next)wr(after+8,prev);
  if(prev)wr(before+12,next);
  wr(p+4,rd(p+4)-1u);
  wr(n,rd(n)&0x7fffffffu);
 }
 return nflstreet_scene_pool_release(x->ram,rd(p+20),base);}
int nflstreet_scene_remove(const nflstreet_scene_insert_context *x,uint32_t scene,uint32_t object){
 if(!x||(scene&3u))return 0;uint8_t *s=span(x->ram,scene,16),*o=span(x->ram,object,3);
 if(!s||!o||rd(s+12))return 0;
 uint32_t target,status;if(!nflstreet_scene_find(x,rd(s),object,&target,&status)||status!=4)return 0;
 if(!nflstreet_scene_node_release(x,rd(s),target))return 0;
 o[2]=(uint8_t)(o[2]-1u);return 1;}
int nflstreet_scene_reorder(const nflstreet_scene_insert_context *x,uint32_t scene,uint32_t object,uint32_t order){
 if(!x||(uint64_t)object+18u>0x100000000ull)return 0;uint8_t *p=span(x->ram,object,18);if(!p||(object&1u))return 0;
 if(!nflstreet_scene_remove(x,scene,object))return 0;
 p[16]=(uint8_t)(order>>8);p[17]=(uint8_t)order;
 uint32_t ignored;return nflstreet_scene_insert(x,scene,object,&ignored);}
