#include "registry_init.h"
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint64_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
int nflstreet_registry_init(const nflstreet_registry_init_context *x,uint32_t count,uint32_t *result){
 if(!result)return 0;*result=0;
 if(!x||!x->services||!x->services->heap||count>UINT32_MAX/20u||
 (x->count_slot&3u)||(x->root_slot&3u))return 0;
 uint8_t *count_word=span(x->ram,x->count_slot,4),*root_word=span(x->ram,x->root_slot,4);
 if(!count_word||!root_word)return 0;
 uint32_t bytes=count*20u,table=x->services->heap(x->services->user,1,bytes,0);
 if(!table)return 1;
 uint8_t *p=span(x->ram,table,bytes);if(!p||(count&&(table&3u)))return 0;
 /* Original signed comparison only iterates for positive counts; the host
    nonwrapping count*20 contract restricts supported counts below INT32_MAX. */
 for(uint32_t i=0;i<count;++i)wr(p+20u*i,0);
 wr(count_word,count);wr(root_word,table);*result=1;return 1;}
int nflstreet_registry_open(const nflstreet_registry_init_context *x,uint32_t count,uint32_t *result){
 if(!result)return 0;*result=0;if(!x)return 0;
 uint8_t *flag=span(x->ram,x->flag_slot,1);if(!flag)return 0;
 if(*flag){*result=0x90001u;return 1;}
 if(!x->services||!x->services->get_error)return 0;
 uint32_t initialized;if(!nflstreet_registry_init(x,count,&initialized))return 0;
 if(initialized){*flag=1;*result=0;}
 else *result=x->services->get_error(x->services->user);
 return 1;}
