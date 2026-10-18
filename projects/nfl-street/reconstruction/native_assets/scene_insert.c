#include "scene_insert.h"
#include <string.h>
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint64_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
static uint8_t *node(nflstreet_pool_ram r,uint32_t a){return(a&3u)?NULL:span(r,a,20);}
static uint8_t *field(nflstreet_pool_ram r,uint32_t a,uint32_t offset,uint32_t size){
 if((uint64_t)a+offset>UINT32_MAX)return NULL;return span(r,a+offset,size);}
int nflstreet_scene_default_compare(nflstreet_pool_ram r,uint32_t left,uint32_t right,int32_t *result){
 if(!result)return 0;*result=0;if((left&3u)||(right&3u))return 0;
 uint8_t *lp=span(r,left,4),*rp=span(r,right,4);if(!lp||!rp)return 0;
 uint32_t l=rd(lp),q=rd(rp);if((l&1u)||(q&1u))return 0;
 lp=field(r,l,16,2);rp=field(r,q,16,2);if(!lp||!rp)return 0;
 uint32_t a=(uint32_t)lp[0]<<8|lp[1],b=(uint32_t)rp[0]<<8|rp[1];
 *result=a<b?-1:a>b?1:0;return 1;}
int nflstreet_scene_insert(const nflstreet_scene_insert_context *x,uint32_t scene,uint32_t object,uint32_t *result){
 if(!result)return 0;*result=0;
 if(!x||!x->pool||!x->pool->error||!x->get_error||(scene&3u)||(x->type_table&3u))return 0;
 uint8_t *s=span(x->ram,scene,4),*obj=span(x->ram,object,3),*d=span(x->ram,x->type_table,48);
 if(!s||!obj||!d||rd(d+8)!=0x80180d64u||rd(d+12)!=0x80180c6cu||rd(d+0x28)!=16)return 0;
 uint32_t list=rd(s);if(list&3u)return 0;uint8_t *p=span(x->ram,list,32);
 if(!p||rd(p)!=0)return 0;uint32_t comparator=rd(p+16);
 if(comparator&&comparator!=0x80194f04u&&!x->compare)return 0;
 uint32_t base;
 if(!nflstreet_pool_allocate(x->ram,rd(p+20),x->pool,&base))return 0;
 if(!base){*result=x->get_error(x->pool->user);return 1;}
 uint8_t *n=node(x->ram,base);if(!n)return 0;
 memset(n,0,16);wr(n,rd(n)&0x7fffffffu);
 wr(n+16,object);
 uint32_t head=rd(p+24),position=head;
 for(size_t steps=0;comparator&&position;++steps){
  if(steps>x->ram.size/16u)return 0;uint8_t *at=node(x->ram,position);if(!at)return 0;
  int32_t order;
  if(comparator==0x80194f04u){if(!nflstreet_scene_default_compare(x->ram,position+16u,base+16u,&order))return 0;}
  else order=x->compare(x->pool->user,comparator,position+16u,base+16u);
  if(order>=0)break;position=rd(at+12);
 }
 if(!head){wr(p+28,base);wr(p+24,base);wr(n+12,0);wr(n+8,0);}
 else if(position==head){uint8_t *h=node(x->ram,head);if(!h)return 0;
  wr(p+24,base);wr(n+12,head);wr(n+8,0);wr(h+8,base);}
 else if(!position){uint32_t tail=rd(p+28);uint8_t *t=node(x->ram,tail);if(!t)return 0;
  wr(p+28,base);wr(n+12,0);wr(n+8,tail);wr(t+12,base);}
 else {uint8_t *next=node(x->ram,position);if(!next)return 0;uint32_t prev=rd(next+8);uint8_t *before=node(x->ram,prev);if(!before)return 0;
  wr(n+8,prev);wr(before+12,base);wr(n+12,position);wr(next+8,base);}
 wr(p+4,rd(p+4)+1u);
 wr(n,rd(n)|0x80000000u);
 obj[2]=(uint8_t)(obj[2]+1u);*result=0;return 1;}
int nflstreet_scene_create_insert(const nflstreet_scene_insert_context *x,const nflstreet_registered_context *registered,
 nflstreet_factory_record *scratch,size_t scratch_count,uint32_t scene,uint32_t type,uint8_t header,uint32_t input,uint32_t *result){
 if(!result)return 0;*result=0;
 if(!x||!registered||x->ram.bytes!=registered->ram.bytes||x->ram.size!=registered->ram.size||x->ram.base!=registered->ram.base)return 0;
 uint32_t object;
 if(!nflstreet_registered_create(registered,scratch,scratch_count,type,header,input,&object))return 0;
 if(object){uint32_t ignored;if(!nflstreet_scene_insert(x,scene,object,&ignored))return 0;}
 *result=object;return 1;}
