#include "options_screen.h"
#include "events.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void shortput(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
static uint8_t*ptr(uint8_t*b,size_t n,unsigned base,unsigned a,size_t z){if(a<base||a-base>n||z>n-(a-base))return NULL;return b+a-base;}
static const unsigned profile_map[21]={23,0,9,7,11,10,12,13,3,4,5,6,15,16,17,18,1,2,1,19,20};
static int effect(nflstreet_ui_options_screen*s,unsigned fn,unsigned argc,unsigned a,unsigned b,unsigned c){if(s->effect_count>=128)return 0;unsigned i=s->effect_count++;s->effects[i].function=fn;s->effects[i].argc=argc;s->effects[i].args[0]=a;s->effects[i].args[1]=b;s->effects[i].args[2]=c;return 1;}
static int cleanup(nflstreet_ui_options_screen*s){
 if(s->options.query_4668||s->options.byte_6db0)return 0;
 if(s->options.word_6da8!=UINT32_MAX){s->options.word_6da8=UINT32_MAX;if(!effect(s,0x80141818,1,s->preview_channel,0,0))return 0;}
 return effect(s,0x80004668,0,0,0,0);
}
static int audio_preview(nflstreet_ui_options_screen*s,unsigned index){
 if(index==11)return 1;if(!effect(s,0x80004668,0,0,0,0))return 0;
 if(index==9){s->options.word_6da8=3;s->preview_channel=1;}
 else if(index==8){s->options.word_6da8=2;s->preview_channel=s->options.word_6d9c?5:4;}
 else if(index==10){if(!effect(s,0x80141d54,0,0,0,0))return 0;s->options.word_6da8=s->query_41d54+4;s->preview_channel=7;}
 else return 1;
 return effect(s,0x80141818,1,s->preview_channel,0,0)&&effect(s,0x801416b0,2,s->preview_channel,0,0)&&effect(s,0x8014164c,3,s->preview_channel,0,127);
}
int nflstreet_ui_options_screen_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){
 nflstreet_ui_options_screen*s=ctx;if(!s||!q||!result)return 0;
 if(q->kind==10){if(q->argc!=1&&q->argc!=2)return 0;if(!q->target||!q->property)return 1;uint8_t*d=ptr(b,n,base,q->target,12),*f=ptr(b,n,base,q->property,12);if(!d||!f)return 0;uint8_t*format=ptr(b,n,base,be(f+8),q->argc==1?3:8);if(!format||memcmp(format,q->argc==1?"%d":"%d,%03d",q->argc==1?3:8))return 0;unsigned cap=be(d+4);if(!cap)return 1;uint8_t*dest=ptr(b,n,base,be(d+8),(size_t)cap+1);if(!dest)return 0;char value[28];int length=q->argc==1?snprintf(value,sizeof value,"%d",(int32_t)q->args[0]):snprintf(value,sizeof value,"%d,%03d",(int32_t)q->args[0],(int32_t)q->args[1]);if(length<0||length>=(int)sizeof value)return 0;unsigned copied=(unsigned)length<cap?(unsigned)length:cap;memmove(dest,value,copied);dest[copied]=0;return 1;}
 if(q->kind==8&&q->property==3&&!q->argc&&q->target==2){++s->return_count;s->return_identity=q->target;return 1;}
 if(q->kind==1){if(q->property==0x80000002&&q->argc==0){
  if(!cleanup(s)||!effect(s,0x8014e29c,1,7,0,0))return 0;
  if(s->options.word_6da0!=s->profile[7]){if(!effect(s,0x8014e29c,1,7,0,0)||s->shared.broadcast_count>=128)return 0;nflstreet_ui_service_request notify={0};notify.kind=2;notify.property=0x8000000f;notify.argc=1;notify.args[0]=!!s->profile[7];s->shared.broadcasts[s->shared.broadcast_count++]=notify;
   if(!effect(s,0x8014e29c,1,7,0,0)||!effect(s,0x8012c498,0,0,0,0))return 0;if(s->profile[7]!=1&&!effect(s,0x8014e29c,1,7,0,0))return 0;if(!effect(s,0x8012c440,2,s->query_c498,s->profile[7]==1?0:s->profile[7],0))return 0;}
  if(!effect(s,0x80004668,0,0,0,0)||!effect(s,0x801415b0,2,7,0,0)||!effect(s,0x801415b0,2,1,0,0)||!effect(s,0x80004668,0,0,0,0)||!effect(s,0x80143eb8,1,s->options.word_6d9c,0,0)||!effect(s,0x801a65d0,1,s->options.word_6dac,0,0))return 0;
  s->options.word_6da8=s->options.word_6dac=UINT32_MAX;return 1;
 }
 if(q->property==0x80000001)return nflstreet_ui_options_service(&s->options,b,n,base,q,result);
  if(q->property==0x80000005&&q->argc==1){unsigned index=q->args[0];if(index>=21&&index!=UINT32_MAX)return 0;s->selection=index;
   if(index>=8&&index<=10)return audio_preview(s,index);
   if(!cleanup(s))return 0;if(!s->options.byte_59f8){if(!effect(s,0x80137d38,0,0,0,0))return 0;if(index==11&&!effect(s,0x80137a50,3,1,0,1))return 0;}return 1;
  }
  if(q->property==0x80000004&&q->argc==3){unsigned index=q->args[0];if(index>=21)return 0;unsigned id=profile_map[index],was=s->profile[id],value=q->args[1];
   if(!effect(s,0x8014e29c,1,id,0,0)||!effect(s,0x8014e134,2,id,value,0))return 0;s->profile[id]=value;
   if(q->args[2]==1){unsigned changed=!was&&value;
    if(index>=8&&index<=11){if(!changed){if(!effect(s,0x8014e29c,1,id,0,0))return 0;if(!value&&index==11&&!effect(s,0x80137d38,0,0,0,0))return 0;}
     else{if(index==11){if(s->options.byte_59f8){if(!effect(s,0x80137c08,1,0,0,0))return 0;}else if(s->selection==11&&!effect(s,0x80137a50,3,1,0,1))return 0;}if(!audio_preview(s,index))return 0;}}
    else if(index==5&&changed){if(!effect(s,0x8014e29c,1,10,0,0))return 0;if(s->profile[10]){if(!effect(s,0x80004668,0,0,0,0)||s->query_99bcc==2)return 0;for(unsigned i=0;i<10;++i)if(!effect(s,0x8018013c,2,i,0,0)||!effect(s,0x80199bcc,0,0,0,0))return 0;}}
   }return 1;
  }
  if(q->property==1&&q->argc==0){*result=2;return 1;}
  if(q->property==0x80000003&&q->argc==1){/* Original DAT80282AAC, checked against each reference module context. */
   
   if(q->args[0]>=21||s->options.count>=64)return 0;unsigned id=profile_map[q->args[0]];
   s->options.requests[s->options.count++]=(nflstreet_ui_options_request){0x8014e29c,1,{id,0}};*result=s->profile[id];return effect(s,0x8014e29c,1,id,0,0);
  }return nflstreet_ui_main_service(&s->shared,b,n,base,q,result);
 }
 if(q->kind==6)return nflstreet_ui_class_activation(b,n,base,q,nflstreet_ui_options_screen_service,s);
 if(q->kind==0&&q->class_id==0&&q->property==0&&q->argc==4){uint8_t*o=ptr(b,n,base,q->target,76);float v[4];memcpy(v,q->args,sizeof v);if(!o||q->args[0]>=4)return 0;unsigned index=q->args[0];for(unsigned i=1;i<4;++i){float scaled=v[i]*16.0f;if(!isfinite(scaled)||scaled<-2147483648.0f||scaled>=2147483648.0f)return 0;shortput(o+52+index*6+(i-1)*2,(uint32_t)(int32_t)scaled);}shortput(o,sh(o)|0x4000);return 1;}
 if(q->kind==0&&q->class_id==0&&q->property==1&&q->argc==3){uint8_t*o=ptr(b,n,base,q->target,52);if(!o||q->args[0]>=4)return 0;for(unsigned i=1;i<3;++i){float v;memcpy(&v,&q->args[i],4);float scaled=v*256.0f;if(!isfinite(scaled)||scaled<-2147483648.0f||scaled>=2147483648.0f)return 0;shortput(o+36+q->args[0]*4+(i-1)*2,(uint32_t)(int32_t)scaled);}shortput(o,sh(o)|0x4000);return 1;}
 if(q->kind==0&&q->class_id==0&&(q->property==2||q->property==3)&&q->argc==(q->property==2?4:2)){uint8_t*o=ptr(b,n,base,q->target,36);if(!o||(q->args[0]>=4&&q->args[0]!=UINT32_MAX))return 0;unsigned begin=q->args[0]==UINT32_MAX?0:q->args[0],end=q->args[0]==UINT32_MAX?4:begin+1;for(unsigned v=begin;v<end;++v){if(q->property==2)for(unsigned j=0;j<3;++j)o[20+v*4+j]=(uint8_t)q->args[j+1];else o[23+v*4]=(uint8_t)q->args[1];}shortput(o,sh(o)|0x4000);return 1;}
 if(q->kind==0&&q->class_id==0&&q->property==0xfffffffc&&q->argc==1){uint8_t*o=ptr(b,n,base,q->target,36);if(!o)return 0;if((sh(o)&2)&&sh(o+6)!=1&&sh(o+6)!=3)return 0;shortput(o,sh(o)|0x4000);return 1;}
 if(q->kind==3||q->kind==4||q->kind==5)return nflstreet_ui_rate_request(&s->shared.rates,b,n,base,0x50000,q);
 if(q->kind==0&&q->class_id==8){uint8_t*o=ptr(b,n,base,q->target,40);if(!o)return 0;
  if(q->property==0xfffffffc)return 1;
  if(q->property<=1&&q->argc==4){for(unsigned i=0;i<4;++i)o[24+q->property*4+i]=(uint8_t)q->args[i];return 1;}
  if(q->property==5&&q->argc==1){if(q->args[0]>1&&q->args[0]!=UINT32_MAX)return 0;shortput(o,(sh(o)&0xfffc)|(q->args[0]==UINT32_MAX?0:q->args[0]?2:1));return 1;}
  if(o[3]!=2||(o[2]!=0&&o[2]!=2)||!s->fonts[o[2]].bytes)return 0;const nflstreet_font*f=&s->fonts[o[2]];
  uint8_t*d=ptr(b,n,base,be(o+32),12);if(!d)return 0;uint8_t*text=ptr(b,n,base,be(d+8),1);if(!text)return 0;size_t available=n-(size_t)(text-b);uint8_t*end=memchr(text,0,available);if(!end)return 0;
  if(q->property==3&&q->argc==1){uint8_t*src=ptr(b,n,base,q->args[0],12);if(!src)return 0;uint8_t*value=ptr(b,n,base,be(src+8),1);if(!value)return 0;uint8_t*last=memchr(value,0,n-(size_t)(value-b));if(!last)return 0;size_t length=last-value,capacity=be(d+4);if(capacity>=available||length>capacity)return 0;memmove(text,value,length);text[length]=0;return nflstreet_ui_text_prepare(o,f,(const char*)text);}
  if(q->property==12&&q->argc==1){if(sh(o)&0x8000){if(!nflstreet_ui_text_prepare(o,f,(const char*)text))return 0;}if(o[15]!=1||sh(o+36))return 0;unsigned length=end-text,index=q->args[0];if(index&&index>=length)index=length-1;unsigned width=0;for(unsigned i=0;i<index&&i<sh(o+38);++i){const uint8_t*g=nflstreet_font_glyph(f,text[i]);if(g)width+=(int8_t)g[8];}float value=(float)(int16_t)width;memcpy(result,&value,4);return 1;}return 0;
 }
 return nflstreet_ui_main_service(&s->shared,b,n,base,q,result);
}
int nflstreet_ui_options_screen_activate(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_options_screen*s){
 if(!b||!s||n<32||be(b+4)<base||be(b+4)-base>n||20>n-(be(b+4)-base))return 0;
 unsigned script,found;if(!nflstreet_ui_event(b,n,base,0,0,0xfffe,0,&script,&found)||!found)return 0;
 nflstreet_ui_script vm={0};vm.current_screen=0x50000;vm.flags=s->shared.entry_flags;vm.root=be(b+be(b+4)-base);vm.pc=base+script;vm.sp=4;vm.words[1]=0xfffe;vm.words[2]=vm.root;
 if(!nflstreet_ui_script_services(b,n,base,0x817c0000,&vm,100000,nflstreet_ui_options_screen_service,s,sizeof *s))return 0;s->shared.entry_flags=vm.flags;return 1;
}

static int options_tick(void*ctx,nflstreet_ui_rates*rates,uint8_t*b,size_t n,uint32_t base,uint32_t screen,uint32_t root,uint32_t script,unsigned argc,const uint32_t*args,float*result){
 nflstreet_ui_options_screen*s=ctx;if(screen!=0x50000||argc>16)return 0;s->shared.rates=*rates;nflstreet_ui_script vm={0};vm.current_screen=screen;vm.linked_screen=2;vm.root=root;vm.flags=s->shared.entry_flags;vm.pc=script;memcpy(vm.words+vm.sp++,result,4);
 for(unsigned i=0;i<argc;++i)vm.words[vm.sp++]=args[i];vm.words[vm.sp++]=root;vm.words[vm.sp++]=0;
 if(!nflstreet_ui_script_services(b,n,base,0x817c0000,&vm,100000,nflstreet_ui_options_screen_service,s,sizeof *s))return 0;s->shared.entry_flags=vm.flags;*rates=s->shared.rates;memcpy(result,vm.words,4);return 1;
}
int nflstreet_ui_options_screen_advance(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_options_screen*s,uint32_t elapsed){
 if(!b||!s||!n||n>8*1024*1024)return 0;uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_options_screen state=*s;
 int ok=nflstreet_ui_rate_update(&state.shared.rates,copy,n,base,0x50000,elapsed,0,options_tick,&state);
 if(ok){memcpy(b,copy,n);*s=state;}free(copy);return ok;
}

int nflstreet_ui_options_screen_start(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_options_screen*s){
 if(!b||!s||n<32||n>8*1024*1024)return 0;uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_options_screen state=*s;unsigned executed;
 int ok=nflstreet_ui_events_linked(copy,n,base,0x50000,2,&state.shared.entry_flags,0,0xfffe,0,0,NULL,nflstreet_ui_options_screen_service,&state,sizeof state,&executed);
 uint8_t*row=ptr(copy,n,base,be(copy+4),20);uint8_t*root=row?ptr(copy,n,base,be(row),8):NULL;
 if(ok&&root){root[4]=root[5]=root[6]=0;root[7]=1;ok=nflstreet_ui_events_linked(copy,n,base,0x50000,2,&state.shared.entry_flags,0,0xfffc,0,0,NULL,nflstreet_ui_options_screen_service,&state,sizeof state,&executed);}else ok=0;
 if(ok){memcpy(b,copy,n);*s=state;}free(copy);return ok;
}
static int options_drain(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_options_screen*s,unsigned first){
 uint8_t*nodes=ptr(b,n,base,be(b+4),(size_t)be(b)*20);if(!nodes||s->shared.pending_count>128)return 0;
 for(unsigned i=first;i<s->shared.pending_count;++i){nflstreet_ui_service_request q=s->shared.pending[i];unsigned target=be(b);for(unsigned j=0;j<be(b);++j)if(be(nodes+j*20)==q.target){target=j;break;}if(target==be(b))return 0;nflstreet_ui_focus_effects effects={0};if(!nflstreet_ui_focus_indexed_execute(b,n,base,target,1,(uint32_t)(int32_t)(int16_t)q.property,&effects,&s->shared.entry_flags,nflstreet_ui_options_screen_service,s,sizeof *s,q.args[0]))return 0;}return 1;
}
static int options_capture(void*ctx,const nflstreet_ui_script*vm,const nflstreet_ui_service_request*q){nflstreet_ui_options_screen*s=ctx;return nflstreet_ui_coroutine_capture(&s->coroutines,vm,q);}
int nflstreet_ui_options_screen_control(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_options_screen*s,unsigned event){
 if(!b||!s||n<32||n>8*1024*1024||event<2||event>7)return 0;uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_options_screen state=*s;unsigned first=state.shared.pending_count,executed,arg=0x3f800000;
 int ok=nflstreet_ui_events_capture(copy,n,base,0x50000,2,&state.shared.entry_flags,0,event,event,1,&arg,nflstreet_ui_options_screen_service,&state,sizeof state,&executed,state.coroutines.capacity?options_capture:NULL);
 if(ok)ok=options_drain(copy,n,base,&state,first);if(ok){memcpy(b,copy,n);*s=state;}free(copy);return ok;
}

int nflstreet_ui_options_screen_complete(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_options_screen*s,uint32_t destination,uint32_t result,unsigned*completed){
 if(!b||!s||!completed||n<32||n>8*1024*1024||s->coroutines.capacity>8||s->coroutines.count>s->coroutines.capacity)return 0;
 if(!s->coroutines.count){*completed=0;return 1;}nflstreet_ui_continuation*f=&s->coroutines.frames[s->coroutines.count-1];if(f->request.target!=destination){for(unsigned i=0;i<s->coroutines.count;++i)if(s->coroutines.frames[i].request.target==destination||s->coroutines.frames[i].request.source==destination)return 0;*completed=0;return 1;}
 uint8_t*copy=malloc(n);nflstreet_ui_options_screen*state=malloc(sizeof *state);if(!copy||!state){free(copy);free(state);return 0;}memcpy(copy,b,n);*state=*s;nflstreet_ui_script vm=f->vm;--state->coroutines.count;int ok=vm.sp&&vm.sp<=256&&vm.suspension==3;
 if(ok){vm.suspension=0;vm.words[vm.sp-1]=result;vm.flags=state->shared.entry_flags;ok=nflstreet_ui_script_services_capture(copy,n,base,0x817c0000,&vm,100000,nflstreet_ui_options_screen_service,state,sizeof *state,options_capture);}
 if(ok){state->shared.entry_flags=vm.flags;memcpy(b,copy,n);*s=*state;*completed=1;}free(copy);free(state);return ok;
}
