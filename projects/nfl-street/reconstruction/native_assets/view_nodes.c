#include "view_nodes.h"
#include <string.h>
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint32_t n){
 if(!r.bytes||(a&3u)||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;return r.bytes+(a-r.base);}
static double wide(const uint8_t *p){uint64_t bits=(uint64_t)rd(p)<<32|rd(p+4);double d;memcpy(&d,&bits,8);return d;}
static float single(const uint8_t *p){uint32_t bits=rd(p);float f;memcpy(&f,&bits,4);return f;}
static void putf(uint8_t *p,float f){uint32_t bits;memcpy(&bits,&f,4);wr(p,bits);}
static float converted(uint32_t low,double bias){uint64_t bits=0x4330000000000000ull|low;double d;memcpy(&d,&bits,8);return (float)(d-bias);}
int nflstreet_view_node_compare(void *u,uint32_t function,uint32_t left,uint32_t right,int32_t *result){
 if(!result)return 0;*result=0;if(!u||(function!=0x801def40u&&function!=0x801dee3cu))return 0;
 nflstreet_pool_ram r=*(nflstreet_pool_ram *)u;uint8_t *a=span(r,left,4),*b=span(r,right,4);if(!a||!b)return 0;
 uint32_t difference=rd(a)-rd(b);memcpy(result,&difference,4);return 1;}
static int context(const nflstreet_view_nodes_context *x){
 return x&&x->lists.services&&x->lists.services->error&&span(x->lists.ram,x->view_list_slot,4)&&span(x->lists.ram,x->callback_list_slot,4);}
int nflstreet_view_node_create(const nflstreet_view_nodes_context *x,uint32_t descriptor,uint32_t *result){
 if(!result)return 0;*result=0;if(!context(x))return 0;nflstreet_pool_ram r=x->lists.ram;
 uint8_t *d=span(r,descriptor,20),*c=span(r,x->constants,32),*display=span(r,x->display_word,4);if(!d||!c||!display)return 0;
 unsigned count=d[3];if(count>50||!span(r,descriptor,4u+count*4u))return 0;
 nflstreet_list_nodes_context nodes=x->lists;nodes.compare_user=&r;nodes.compare=nflstreet_view_node_compare;
 uint32_t payload,list=rd(span(r,x->view_list_slot,4));if(!nflstreet_list_node_allocate(&nodes,list,&payload))return 0;if(!payload)return 1;
 uint8_t *p=span(r,payload,0xf8);if(!p)return 0;
 wr(p,(uint32_t)d[0]<<8|d[1]);p[0x2f]=(uint8_t)count;p[0x2e]=(uint8_t)count;
 for(unsigned i=0;i<count;++i)wr(p+0x30+4*i,rd(d+4+4*i));
 float zero=single(c),half=single(c+24),one=single(c+28);putf(p+4,zero);
 float screen=converted(rd(display)^0x80000000u,wide(c+8));
 float first=converted((uint32_t)d[16]<<8|d[17],wide(c+16));
 float second=converted((uint32_t)d[18]<<8|d[19],wide(c+16));
 volatile float delta=screen-first;float centered=delta*half;
 putf(p+8,centered);putf(p+12,second);putf(p+16,first);putf(p+20,second);
 p[0x44]=0;putf(p+0x28,zero);putf(p+0x20,zero);putf(p+0x24,zero);
 wr(p+0x1c,1);putf(p+0x18,first);putf(p+0xf0,one);wr(p+0x3c,0);wr(p+0x48,0);
 uint32_t published;if(!nflstreet_list_node_publish(&nodes,rd(span(r,x->view_list_slot,4)),payload,&published))return 0;
 /* Original801DF8B4 is li r3,0;blr. No error service follows that zero. */
 *result=payload;return 1;}
int nflstreet_view_callback_register(const nflstreet_view_nodes_context *x,uint32_t arg1,uint32_t arg2,uint32_t arg3,uint32_t arg4,uint32_t *result){
 if(!result)return 0;*result=0;if(!context(x)||!x->get_error)return 0;nflstreet_pool_ram r=x->lists.ram;
 nflstreet_list_nodes_context nodes=x->lists;nodes.compare_user=&r;nodes.compare=nflstreet_view_node_compare;
 uint32_t payload,list=rd(span(r,x->callback_list_slot,4));if(!nflstreet_list_node_allocate(&nodes,list,&payload))return 0;
 if(!payload){*result=x->get_error(x->lists.services->user);return 1;}
 uint8_t *p=span(r,payload,16);if(!p)return 0;wr(p,arg4);wr(p+4,arg2);wr(p+8,arg3);wr(p+12,arg1);
 uint32_t published;if(!nflstreet_list_node_publish(&nodes,rd(span(r,x->callback_list_slot,4)),payload,&published))return 0;
 return 1;}
