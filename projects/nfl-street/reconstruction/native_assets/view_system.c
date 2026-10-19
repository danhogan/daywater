#include "view_system.h"
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint32_t n){
 if(!r.bytes||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;return r.bytes+(a-r.base);}
int nflstreet_view_system_type0(nflstreet_pool_ram r,uint32_t table,uint32_t alignment){
 uint8_t *p=span(r,table,48);if(!p||(table&3u))return 0;
 static const uint32_t callbacks[9]={0x80180c20u,0x80180c30u,0x80180d64u,0x80180c6cu,0x80180d8cu,0x80180e0cu,0x80180e6cu,0x80180f18u,0x80180facu};
 for(unsigned i=1;i<9;++i)wr(p+4*i,callbacks[i]);wr(p,callbacks[0]);
 wr(p+0x24,nflstreet_pool_align(32u,alignment));wr(p+0x28,nflstreet_pool_align(16u,alignment));p[0x2c]=1;return 1;}
int nflstreet_view_system_open(const nflstreet_view_system_context *x,uint32_t descriptor,uint32_t *code){
 if(!code)return 0;*code=0;
 if(!x||!x->owner.services||!x->owner.services->error||(x->r13&3u)||x->r13<0x65f4u)return 0;
 nflstreet_pool_ram r=x->owner.ram;uint8_t *flag=span(r,x->r13-0x6390u,1);if(!flag)return 0;
 uint32_t result=0;
 if(*flag)result=0x80001u;
 else{
  if((descriptor&3u)||!x->get_error||!x->matrix||x->matrix->ram.bytes!=r.bytes||x->matrix->ram.size!=r.size||x->matrix->ram.base!=r.base)return 0;
  uint8_t *d=span(r,descriptor,12),*view=span(r,x->r13-0x638cu,4),*callback=span(r,x->r13-0x6388u,4),*scene=span(r,x->r13-0x6380u,4),*active=span(r,x->r13-0x4eecu,4),*index=span(r,x->r13-0x4ee8u,1);
  if(!d||!view||!callback||!scene||!active||!index)return 0;
  uint32_t list;if(!nflstreet_scene_list_create(&x->owner,1,0,rd(d),0xf8,0x801def40u,0,&list))return 0;
  wr(view,list);
  if(!list)result=x->get_error(x->owner.services->user);
  else{
   if(!nflstreet_scene_list_create(&x->owner,1,0,rd(d+4),16,0x801dee3cu,0,&list))return 0;wr(callback,list);
   if(!list)result=x->get_error(x->owner.services->user);
   else{
    *index=d[10];if(!nflstreet_matrix_storage_allocate(x->matrix,d[10],d[11],&result))return 0;
    if(!result){uint32_t capacity=(uint32_t)d[8]<<8|d[9];
     if(capacity){uint32_t created;if(!nflstreet_scene_owner_create(&x->owner,1,capacity,0,0,0,1,3,1,&created))return 0;wr(scene,created);}
     /* Original801DF834 returns0 even when scene allocation returns0. */
    }
   }
  }
  wr(active,0);*flag=1;
 }
 x->owner.services->error(x->owner.services->user,result);*code=result;return 1;}
