#include "title.h"
#include "screen.h"
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int nflstreet_ui_title_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){
 nflstreet_ui_title*s=ctx;
 if(q->kind==1&&q->argc==0){
  /* Original common query 8010C3BC -> 8010BE48 returns integer word 2. */
  if(q->property==1){*result=2;return 1;}
  if(q->property==0x80000001){s->title_flag=1;s->flag_470c=0;s->flag_22130=0;return 1;}
  if(q->property==0x80000002){s->title_flag=0;return 1;}return 0;
 }
 if(q->kind==2){if(s->broadcasts>=32)return 0;s->requests[s->broadcasts++]=*q;return 1;}
 if(q->kind==3||q->kind==4||q->kind==5)return nflstreet_ui_rate_request(&s->rates,b,n,base,3,q);return 0;
}
static int event(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_title*s,uint32_t root,uint32_t script,const uint32_t*args,unsigned argc,int sourceid){
 if(argc>16)return 0;nflstreet_ui_script vm={0};vm.flags=s->entry_flags;vm.pc=script;vm.words[vm.sp++]=0;if(sourceid>=0)vm.words[vm.sp++]=(uint32_t)sourceid;
 for(unsigned i=0;i<argc;++i)vm.words[vm.sp++]=args[i];vm.words[vm.sp++]=root;vm.words[vm.sp++]=0;
 int ok=nflstreet_ui_script_services(b,n,base,0x70000000,&vm,10000,nflstreet_ui_title_service,s,sizeof *s);if(ok)s->entry_flags=vm.flags;return ok;
}
int nflstreet_ui_title_activate(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_title*s){
 if(!b||!s||n<32||s->rates.clock==0)return 0;uint32_t script;unsigned found;
 if(!nflstreet_ui_event(b,n,base,0,0,0xfffe,0,&script,&found)||!found)return 0;uint32_t table=be(b+4);
 if(table<base||table-base>n||20>n-(table-base))return 0;return event(b,n,base,s,be(b+table-base),base+script,NULL,0,0xfffe);
}
typedef struct {uint8_t*b;size_t n;uint32_t base;nflstreet_ui_title*state;} call_context;
static int completion(void*context,nflstreet_ui_rates*rates,uint8_t*b,size_t n,uint32_t base,uint32_t screen,uint32_t root,uint32_t script,unsigned argc,const uint32_t*args,float*result){
 call_context*c=context;(void)result;if(screen!=3)return 0;c->state->rates=*rates;int ok=event(b,n,base,c->state,root,script,args,argc,-1);if(ok)*rates=c->state->rates;return ok;
}
int nflstreet_ui_title_advance(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_title*s,uint32_t elapsed){
 if(!b||!s||!n||n>8*1024*1024)return 0;uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_title state=*s;call_context c={copy,n,base,&state};
 /* All completion scripts execute against the scheduler's owned screen. */
 int ok=nflstreet_ui_rate_update(&state.rates,copy,n,base,3,elapsed,0,completion,&c);
 if(ok){memcpy(b,copy,n);*s=state;}free(copy);return ok;
}
