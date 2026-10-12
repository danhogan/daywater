#include "storage_dialog.h"
#include <string.h>
static int request(nflstreet_storage_dialog*s,uint32_t f,unsigned n,uint32_t a,uint32_t b){if(s->count>=16)return 0;s->requests[s->count++]=(nflstreet_storage_request){f,n,{a,b}};return 1;}
int nflstreet_storage_common(nflstreet_storage_dialog*s,uint32_t p,uint32_t a,uint32_t*out){
 if(!s||!out||s->count>16)return 0;nflstreet_storage_dialog t=*s;int ok=1;
 switch(p){
 case 0x80000002:t.word_5a10=0xffffffff;ok=request(&t,0x8018f084,1,0,0)&&request(&t,0x8018f01c,1,1,0);t.word_573c=0;memset(t.callbacks,0,sizeof t.callbacks);break;
 case 0x80000003:ok=request(&t,0x8014e134,2,13,a);break;
 case 0x80000004:t.word_573c=0xffffffff;memcpy(t.callbacks,t.source_callbacks,sizeof t.callbacks);break;
 case 0x80000005:t.word_573c=0xffffffff;ok=request(&t,0x8018f01c,1,0,0);memset(t.callbacks,0,sizeof t.callbacks);break;
 case 0x80000006:t.word_5a10=a;break;
 default:return 0;
 }if(!ok)return 0;*s=t;*out=1;return 1;
}
int nflstreet_storage_select(nflstreet_storage_dialog*s,uint32_t*delay){
 if(!s||!delay||s->count>16)return 0;nflstreet_storage_dialog t=*s;uint32_t selected=t.query_slot;
 if(selected==0xffffffff||(t.word_5a10!=0xffffffff&&t.word_5a10!=selected)){
  if(t.word_573c<2&&!request(&t,0x8018f150,0,0,0))return 0;
  selected=t.word_5a10==0xffffffff?0:t.word_5a10;
 }
 t.word_573c=selected;
 if(!request(&t,0x8018f084,1,0,0)||!request(&t,0x8018f01c,1,1,0)||!request(&t,0x8018f120,1,8,0))return 0;
 *s=t;*delay=9999;return 1;
}

int nflstreet_storage_poll(nflstreet_storage_dialog*s,const nflstreet_storage_poll_inputs*in,nflstreet_storage_poll_output*out){
 if(!s||!in||!out||s->count>16||!in->sample_count||in->sample_count>8)return 0;
 nflstreet_storage_dialog t=*s;nflstreet_storage_poll_output o={0};
 if(!request(&t,0x800070ac,0,0,0))return 0;
 int found=0;for(unsigned i=0;i<in->sample_count;++i){
  if(!request(&t,0x80006f64,2,0,i))return 0;
  ++o.iterations;o.type=in->samples[i][0];o.id=in->samples[i][1];
  if(o.type==in->desired[0]&&o.id==in->desired[1]){found=1;break;}
 }
 if(!found)return 0;
 /* First original80006DC4 call forwards the previous query argument registers;
    requests here preserve the response boundary only, not volatile pointers. */
 if(!request(&t,0x80006dc4,0,0,0)||!request(&t,0x80006de4,2,in->desired[0],in->desired[1]))return 0;
 o.slot=in->slot;o.status=in->status;
 int probe=in->desired[0]==1;
 if(in->desired[0]==2){if(!request(&t,0x80006d6c,2,2,in->desired[1]))return 0;probe=in->type2_query!=0;}
 if(probe){if(!request(&t,0x8018f5e8,1,32,0)||!request(&t,0x8018f7b8,1,in->desired[0],0))return 0;o.status=in->probe_result;if(o.status!=0xffffffff)o.type+=0x100;}
 if(!request(&t,0x80006dc4,2,in->desired[0],in->desired[1]))return 0;
 if(in->slot==0xffffffff)o.type+=0x200;
 *s=t;*out=o;return 1;
}
