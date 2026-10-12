#include "options.h"
static int append(nflstreet_ui_options*s,uint32_t fn,unsigned argc,uint32_t a,uint32_t b){
 if(s->count>=64)return 0;s->requests[s->count++]=(nflstreet_ui_options_request){fn,argc,{a,b}};return 1;
}
int nflstreet_ui_options_initialize(nflstreet_ui_options*s){
 if(!s||s->query_4668||s->count>64)return 0;nflstreet_ui_options t=*s;
 t.word_6da8=UINT32_MAX;
 if(!append(&t,0x80004668,0,0,0))return 0;t.byte_6db0=0;
 if(!append(&t,0x8014e29c,1,7,0))return 0;t.word_6da0=t.query_7;
 if(!append(&t,0x801a6178,2,0x80286ec0,0x2c))return 0;t.word_6dac=t.sound_handle;
 if(!append(&t,0x801440f0,1,0,0))return 0;t.word_6d9c=!t.query_440f0;
 if(!append(&t,0x80143fc0,1,t.word_6d9c,0)||!append(&t,0x80143ee0,1,t.word_6d9c,0))return 0;
 for(unsigned i=0;i<2;++i)if(!append(&t,0x801441f4,1,t.word_6d9c,0)||!append(&t,0x801415b0,2,i?1:7,t.query_441f4[i]))return 0;
 t.byte_59f8=0;*s=t;return 1;
}
int nflstreet_ui_options_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){
 (void)b;(void)n;(void)base;(void)result;
 return q&&q->kind==1&&q->property==0x80000001&&q->argc==0&&nflstreet_ui_options_initialize(ctx);
}
