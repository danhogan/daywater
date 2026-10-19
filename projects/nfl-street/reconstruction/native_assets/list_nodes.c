#include "list_nodes.h"
#include <string.h>
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint32_t n){
 if(!r.bytes||(a&3u)||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
static uint8_t *list(const nflstreet_list_nodes_context *x,uint32_t a){
 if(!x)return NULL;uint8_t *p=span(x->ram,a,32),*d=span(x->ram,x->type_table,48);
 if(!p||!d||rd(p)||rd(d+8)!=0x80180d64u||rd(d+12)!=0x80180c6cu||rd(d+0x28)!=16)return NULL;return p;}
int nflstreet_list_node_allocate(const nflstreet_list_nodes_context *x,uint32_t a,uint32_t *result){
 if(!result)return 0;*result=0;uint8_t *p=list(x,a);
 if(!p||!x->services||!x->services->error)return 0;uint32_t node;
 if(!nflstreet_pool_allocate(x->ram,rd(p+20),x->services,&node))return 0;
 if(!node)return 1;uint8_t *n=span(x->ram,node,16);if(!n||node>UINT32_MAX-16u)return 0;
 memset(n,0,16);wr(n,rd(n)&0x7fffffffu);*result=node+16u;return 1;}
int nflstreet_list_node_publish(const nflstreet_list_nodes_context *x,uint32_t a,uint32_t payload,uint32_t *result){
 if(!result)return 0;*result=0;uint8_t *p=list(x,a);
 if(!p||payload<16)return 0;uint32_t node=payload-16u;uint8_t *n=span(x->ram,node,16);if(!n)return 0;
 uint32_t callback=rd(p+16),head=rd(p+24),position=head;
 if(callback&&!x->compare)return 0;
 for(size_t steps=0;callback&&position;++steps){
  if(steps>=x->ram.size/16u||position>UINT32_MAX-16u)return 0;
  uint8_t *at=span(x->ram,position,16);if(!at)return 0;int32_t order;
  if(!x->compare(x->compare_user,callback,position+16u,payload,&order))return 0;
  if(order>=0)break;position=rd(at+12);
 }
 if(!head){wr(p+28,node);wr(p+24,node);wr(n+12,0);wr(n+8,0);}
 else if(position==head){uint8_t *h=span(x->ram,head,16);if(!h)return 0;
  wr(p+24,node);wr(n+12,head);wr(n+8,0);wr(h+8,node);}
 else if(!position){uint32_t tail=rd(p+28);uint8_t *t=span(x->ram,tail,16);if(!t)return 0;
  wr(p+28,node);wr(n+12,0);wr(n+8,tail);wr(t+12,node);}
 else{uint8_t *next=span(x->ram,position,16);if(!next)return 0;uint32_t previous=rd(next+8);uint8_t *before=span(x->ram,previous,16);if(!before)return 0;
  wr(n+8,previous);wr(before+12,node);wr(n+12,position);wr(next+8,node);}
 wr(p+4,rd(p+4)+1u);wr(n,rd(n)|0x80000000u);*result=payload;return 1;}
