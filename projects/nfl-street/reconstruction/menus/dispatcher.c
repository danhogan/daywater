#include "dispatcher.h"
#include "screen.h"
int nflstreet_ui_dispatcher_service(void*ctx,uint8_t*b,size_t n,uint32_t base,
 const nflstreet_ui_service_request*q,uint32_t*result){
 (void)b;(void)n;(void)base;(void)result;nflstreet_ui_dispatcher*s=ctx;
 if(!s||!q)return 0;
 if(q->kind==1&&q->property==0xb&&q->argc==3){for(unsigned i=0;i<3;++i)s->transition[i]=q->args[i];return 1;}
 if(q->kind==1&&q->argc==0){
  if(q->property==7){*result=0;return 1;}
  if(q->property==1){*result=2;return 1;}
  if(q->property==0x134){*result=!!s->query134;return 1;}
  if(q->property==0x80000001){return 1;}
 }
 if(s->count>=64||q->kind!=9||q->argc>16||(q->property!=0&&q->property!=1&&q->property!=3&&q->property!=8))return 0;
 s->requests[s->count++]=*q;return 1;
}

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int event(uint8_t*b,size_t n,uint32_t base,uint32_t*flags,nflstreet_ui_dispatcher*s,unsigned identity,unsigned code,uint32_t state,uint32_t result){
 if(!b||!flags||!s||n<32||be(b)!=1||be(b+4)<base||be(b+4)-base>n||20>n-(be(b+4)-base))return 0;
 unsigned notification=code==1||code==2,script,found;
 if(!notification&&code!=0xfffe&&code!=0xfffc)return 0;
 if(!nflstreet_ui_event(b,n,base,0,notification?1:0,(uint16_t)code,0,&script,&found)||!found)return 0;
 nflstreet_ui_script vm={0};vm.current_screen=identity;vm.root=be(b+be(b+4)-base);vm.flags=*flags;vm.pc=base+script;vm.sp=notification?5:4;
 if(notification){vm.words[1]=state;vm.words[2]=result;vm.words[3]=vm.root;}else{vm.words[1]=code;vm.words[2]=vm.root;}
 if(!nflstreet_ui_script_services(b,n,base,0x70000000,&vm,10000,nflstreet_ui_dispatcher_service,s,sizeof *s))return 0;*flags=vm.flags;return 1;
}
int nflstreet_ui_dispatcher_activate(uint8_t*b,size_t n,uint32_t base,uint32_t*flags,nflstreet_ui_dispatcher*s){return event(b,n,base,flags,s,2,0xfffe,0,0);}
int nflstreet_ui_dispatcher_options(uint8_t*b,size_t n,uint32_t base,uint32_t*flags,nflstreet_ui_dispatcher*s){return event(b,n,base,flags,s,2,1,0x96,UINT32_MAX);}
int nflstreet_ui_dispatcher_notify(uint8_t*b,size_t n,uint32_t base,uint32_t*flags,nflstreet_ui_dispatcher*s,unsigned code,uint32_t state,uint32_t result){
 if(code!=1&&code!=2)return 0;return event(b,n,base,flags,s,2,code,state,result);
}
int nflstreet_ui_dispatcher_reactivate(uint8_t*b,size_t n,uint32_t base,uint32_t*flags,nflstreet_ui_dispatcher*s){return event(b,n,base,flags,s,2,0xfffc,0,0);}
int nflstreet_ui_quick_setup_event(uint8_t*b,size_t n,uint32_t base,uint32_t*flags,nflstreet_ui_dispatcher*s,unsigned code,uint32_t state,uint32_t result){
 return event(b,n,base,flags,s,5,code,state,result);
}
