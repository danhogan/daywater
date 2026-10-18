#include "matrix_storage.h"
#include <string.h>
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint64_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;
 return r.bytes+(a-r.base);}
static uint8_t *word(const nflstreet_matrix_storage_context *x,uint32_t a){return(a&3u)?NULL:span(x->ram,a,4);}
int nflstreet_matrix_storage_open(const nflstreet_matrix_storage_context *x,uint32_t requested,uint32_t *code){
 if(!code)return 0;*code=0;if(!x||!x->services||!x->services->error)return 0;
 uint8_t *flag=span(x->ram,x->flag_slot,1);if(!flag)return 0;
 uint32_t result=0;
 if(*flag)result=0x50001u;
 else{
  if(!x->services->heap||!x->services->get_error||requested>UINT32_MAX/12u-8u)return 0;
  uint8_t *table_slot=word(x,x->table_slot),*count_slot=word(x,x->count_slot);if(!table_slot||!count_slot)return 0;
  uint32_t count=requested+8u,table=x->services->heap(x->services->user,1,count*12u,0);
  wr(table_slot,table);
  if(!table)result=x->services->get_error(x->services->user);
  else{
   uint8_t *p=word(x,table);if(!p||!span(x->ram,table,(uint64_t)count*12u))return 0;
   memset(p,0,(size_t)count*12u);wr(count_slot,count);
   /* Original 80189664 is exactly blr: no hidden stack initialization. */
   *flag=1;
  }
 }
 x->services->error(x->services->user,result);*code=result;return 1;}
int nflstreet_matrix_storage_entry(const nflstreet_matrix_storage_context *x,uint32_t index,uint32_t *entry){
 if(!entry)return 0;*entry=0;if(!x)return 0;uint8_t *count=word(x,x->count_slot);if(!count)return 0;
 if(index>=rd(count))return 1;
 uint8_t *slot=word(x,x->table_slot);if(!slot)return 0;
 uint64_t address=(uint64_t)rd(slot)+(uint64_t)index*12u;
 if(address>UINT32_MAX)return 0;*entry=(uint32_t)address;return 1;}
int nflstreet_matrix_storage_allocate(const nflstreet_matrix_storage_context *x,uint32_t index,uint32_t depth,uint32_t *code){
 if(!code)return 0;*code=0;
 if(!x||!x->services||!x->services->heap||!x->services->get_error||!x->services->error||depth>UINT32_MAX/64u)return 0;
 uint32_t entry;if(!nflstreet_matrix_storage_entry(x,index,&entry)||!entry)return 0;
 uint8_t *p=word(x,entry);if(!p||!span(x->ram,entry,12))return 0;
 uint32_t slots=x->services->heap(x->services->user,1,depth*64u,0),result=0;
 if(!slots)result=x->services->get_error(x->services->user);
 else{
  uint8_t *m=word(x,slots),*zero=word(x,x->zero_slot),*one=word(x,x->one_slot);
  uint64_t bytes=depth?(uint64_t)depth*64u:64u;
  if(!m||!zero||!one||!span(x->ram,slots,bytes))return 0;
  wr(p,depth);wr(p+4,0);wr(p+8,slots);
  uint32_t z=rd(zero),o=rd(one);
  for(unsigned i=0;i<16;++i)wr(m+4*i,i%5u==0?o:z);
 }
 x->services->error(x->services->user,result);*code=result;return 1;}
int nflstreet_matrix_storage_bind(const nflstreet_matrix_storage_context *x,nflstreet_matrix_stacks *out){
 if(!x||!out)return 0;uint8_t *table=word(x,x->table_slot),*count=word(x,x->count_slot);
 if(!table||!count||!word(x,x->current_slot))return 0;
 out->table=rd(table);out->count=rd(count);out->current=x->current_slot;return 1;}
