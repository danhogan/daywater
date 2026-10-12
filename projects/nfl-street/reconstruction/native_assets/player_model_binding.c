#include "player_model_binding.h"
#include <string.h>
static uint32_t read(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned half(const uint8_t*p){return(unsigned)p[0]<<8|p[1];}
static void word(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void whalf(uint8_t*p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static int resolve(const nflstreet_player_model_binding_context*c,uint32_t token,size_t min,nflstreet_player_model_span*s){
 memset(s,0,sizeof *s);return token&&c->resolve(c->resolve_context,token,min,s)&&s->bytes&&s->size>=min;
}
static int call(const nflstreet_player_model_binding_context*c,uint8_t*a,size_t size,nflstreet_player_model_service_kind kind,
 uint32_t off,uint32_t x,uint32_t y,uint32_t z,uint32_t*out){nflstreet_player_model_request q={kind,off,{x,y,z}};return c->service(c->service_context,&q,a,size,out);}
static int allocate(const nflstreet_player_model_binding_context*c,uint32_t count,uint32_t stride,uint32_t*token,nflstreet_player_model_span*s){
 memset(s,0,sizeof *s);return c->allocate(c->allocate_context,count,stride,token,s)&&*token&&s->bytes&&(uint64_t)count*stride<=s->size;
}
static int commands(const nflstreet_player_model_binding_context*c,uint8_t*dest,uint32_t bytes){
 uint32_t aligned=(bytes+31u)&~31u,token=0;nflstreet_player_model_span s;
 if(bytes>UINT32_MAX-31u||!allocate(c,aligned,1,&token,&s))return 0;
 word(dest,token);word(dest+4,0);word(dest+8,aligned);word(dest+12,32);return 1;
}
static int model_copy(const nflstreet_player_model_binding_context*c,uint8_t*dest,uint32_t token){
 if(!token){memset(dest+4,0,48);return 1;}nflstreet_player_model_span s;if(!resolve(c,token,52,&s))return 0;
 memcpy(dest,s.bytes,52);word(dest+0x1c,0);return 1;
}
static int indices(const nflstreet_player_model_binding_context*c,uint8_t*model){
 uint32_t token=read(model+8);if(!token)return 1;nflstreet_player_model_span src,allocated;uint32_t output=0;
 if(!resolve(c,token,16,&src))return 0;uint32_t count=read(src.bytes+12);
 if(!allocate(c,count,4,&output,&allocated))return 0;word(model+0x1c,output);memset(allocated.bytes,0,(size_t)count*4);return 1;
}
static int remap(const nflstreet_player_model_binding_context*c,uint8_t*a,size_t size,uint8_t*model){
 nflstreet_player_model_span old,target,oldrecords,newrecords,temporary,mesh,entries;uint32_t temp_token=0,out=0;
 if(!resolve(c,read(model+0x18),8,&old)||!resolve(c,c->mapping_table_token,8,&target))return 0;
 uint32_t count=read(old.bytes),newcount=read(target.bytes);
 if(count>65535||newcount>65535||!resolve(c,read(old.bytes+4),(size_t)count*48,&oldrecords)||
  !resolve(c,read(target.bytes+4),(size_t)newcount*48,&newrecords)||!allocate(c,count,2,&temp_token,&temporary))return 0;
 for(uint32_t i=0;i<count;++i){uint32_t mapped=UINT32_MAX;
  for(uint32_t j=0;j<newcount;++j){if(!call(c,a,size,NFLSTREET_MODEL_NAMES_EQUAL,0,read(newrecords.bytes+j*48),read(oldrecords.bytes+i*48),0,&out))return 0;
   if(out){mapped=j;break;}}
  whalf(temporary.bytes+i*2,mapped);}
 if(!resolve(c,read(model+8),16,&mesh))return 0;uint32_t entrycount=read(mesh.bytes+4);
 if(entrycount){if(!resolve(c,read(mesh.bytes),(size_t)entrycount*24,&entries))return 0;
  for(uint32_t i=0;i<entrycount;++i){int index=(int16_t)half(entries.bytes+i*24+6);
   whalf(entries.bytes+i*24+6,index<0||(uint32_t)index>=count?0xffffu:half(temporary.bytes+2*(unsigned)index));}}
 if(!c->release(c->release_context,temp_token))return 0;word(model+0x18,c->mapping_table_token);return 1;
}
int nflstreet_player_model_bind(uint8_t*a,size_t size,const nflstreet_player_model_binding_context*c){
 if(!a||size<0x1a90||!c||!c->resolve||!c->allocate||!c->release||!c->service||!c->scratch||c->scratch_size<76||!c->shared||c->shared_size<68)return 0;
 unsigned marked=(unsigned)(a[0x1364]+2)*2;if(marked+2>c->scratch_size)return 0;uint32_t out=0;
#define CALL(K,O,X,Y,Z) do{if(!call(c,a,size,K,O,X,Y,Z,&out))return 0;}while(0)
 CALL(NFLSTREET_MODEL_PREPARE_KEY,0,read(a+0x400),0,0);
 CALL(NFLSTREET_MODEL_RESOURCE,0,4,a[0x1362],0);if(!commands(c,a+0x420,c->command_bytes)||!model_copy(c,a+0x430,out)||(out&&!indices(c,a+0x430)))return 0;
 CALL(NFLSTREET_MODEL_RESOURCE,0,4,a[0x1362],1);if(!commands(c,a+0x464,c->command_bytes>>1)||!model_copy(c,a+0x474,out)||(out&&!indices(c,a+0x474)))return 0;
 whalf(c->scratch+marked,0x1000);CALL(NFLSTREET_MODEL_RENDER_BLOCK,0x420,38,0,0);CALL(NFLSTREET_MODEL_RENDER_BLOCK,0x464,38,0,0);
 whalf(c->scratch+marked,0);whalf(c->scratch+0x10,0);uint32_t first=read(a+0x44c),second=read(a+0x490);
 CALL(NFLSTREET_MODEL_PREPARE_STYLE,0,a[0x1362],0,0);
 CALL(NFLSTREET_MODEL_RESOURCE,0,5,0,0);if(!out||!model_copy(c,a+0x430,out))return 0;
 CALL(NFLSTREET_MODEL_RESOURCE,0,5,0,1);if(!out||!model_copy(c,a+0x474,out))return 0;
 word(a+0x44c,first);word(a+0x490,second);if(!remap(c,a,size,a+0x430)||!remap(c,a,size,a+0x474))return 0;
 CALL(NFLSTREET_MODEL_RESET,0,0,0,0);CALL(NFLSTREET_MODEL_COMMIT_STYLE,0,a[0x1362],0,0);
 if(!read(c->shared)){memcpy(c->shared,a+0x430,52);word(c->shared+0x1c,0);if(!indices(c,c->shared)||!commands(c,c->shared+52,0x7404))return 0;}
 CALL(NFLSTREET_MODEL_BIND_WORDS,0x404,a[0x1362],0,0);CALL(NFLSTREET_MODEL_BIND_A,0,a[0x1362],read(a+0x410),0);
 CALL(NFLSTREET_MODEL_BIND_B,0,a[0x1362],read(a+0x414),0);CALL(NFLSTREET_MODEL_FINISH,0,0,0,0);
#undef CALL
 return 1;
}
