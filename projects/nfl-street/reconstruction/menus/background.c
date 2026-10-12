#include "background.h"
#include "screen.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void shortput(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
static int effect(nflstreet_ui_background*s,unsigned kind,unsigned archive,unsigned entry){
 if(s->effect_count>=512)return 0;s->effects[s->effect_count++]=(nflstreet_ui_resource_effect){kind,archive,entry};return 1;
}
int nflstreet_ui_background_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){
 nflstreet_ui_background*s=ctx;if(!s||!q||!b)return 0;
 if(q->kind>=3&&q->kind<=5)return nflstreet_ui_rate_request(&s->rates,b,n,base,0,q);
 /*8010C3BC/8010CB40 uses lwz/srawi, despite Ghidra's inferred float type. */
 if(q->kind==1&&q->property==9&&q->argc==2){float lower,upper;memcpy(&lower,q->args,4);memcpy(&upper,q->args+1,4);if(!isfinite(lower)||!isfinite(upper)||!isfinite(s->random_sample)||s->random_sample<0||s->random_sample>1)return 0;float span=(float)((double)upper-lower),value=(float)fma((double)span,(double)s->random_sample,(double)lower);memcpy(result,&value,4);return 1;}
 if(q->kind==1&&q->property==0x144&&q->argc==2){unsigned archive=q->args[1]>>16,entry=q->args[1]&65535;if(archive!=8||entry>=22)return 0;*result=s->ready[entry]?1:0;return 1;}
 if(q->kind!=0||q->class_id!=0||q->property!=5||q->argc!=1||q->target<base||q->target-base>n||16>n-(q->target-base))return 0;
 uint8_t*o=b+q->target-base;unsigned oldarchive=sh(o+6),oldentry=sh(o+4),archive=q->args[0]>>16,entry=q->args[0]&65535;
 if(archive==0||archive==65535)archive=oldarchive;
 if(archive!=8||oldarchive!=8||entry>=22||oldentry>=22)return 0;
 if(archive!=oldarchive||entry!=oldentry){
  /*801D60DC pending branch; no claim that pending entries are installed. */
  if(s->busy[oldentry]){shortput(o+12,entry);shortput(o+14,archive);shortput(o,sh(o)|0x4000);return 1;}
  /*801D84D8 for the registered non-streaming archive: release then request. */
  if(!effect(s,0,oldarchive,oldentry)||!effect(s,1,archive,entry))return 0;
  shortput(o+6,archive);shortput(o+4,entry);
 }
 shortput(o+12,65535);shortput(o+14,65535);shortput(o,sh(o)|0x4000);return 1;
}
static int call(void*ctx,nflstreet_ui_rates*rates,uint8_t*b,size_t n,uint32_t base,uint32_t screen,uint32_t root,uint32_t script,unsigned argc,const uint32_t*args,float*result){
 nflstreet_ui_background*s=ctx;s->rates=*rates;if(screen||argc>16)return 0;nflstreet_ui_script vm={0};vm.flags=s->entry_flags;vm.pc=script;vm.words[vm.sp++]=0x3f800000;
 for(unsigned i=0;i<argc;++i)vm.words[vm.sp++]=args[i];vm.words[vm.sp++]=root;vm.words[vm.sp++]=0;
 if(!nflstreet_ui_script_services(b,n,base,0x70000000,&vm,10000,nflstreet_ui_background_service,s,sizeof *s))return 0;
 s->entry_flags=vm.flags;*rates=s->rates;memcpy(result,vm.words,4);return 1;
}
int nflstreet_ui_background_advance(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_background*s,uint32_t elapsed){
 if(!b||!s||!n||n>8*1024*1024)return 0;uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_background state=*s;
 int ok=nflstreet_ui_rate_update(&state.rates,copy,n,base,0,elapsed,0,call,&state);
 if(ok){memcpy(b,copy,n);*s=state;}free(copy);return ok;
}

int nflstreet_ui_background_activate(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_background*s){
 if(!b||!s||n<32||n>8*1024*1024||be(b)!=4)return 0;unsigned table=be(b+4);if(table<base||table-base>n||80>n-(table-base))return 0;
 uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_background state=*s;int ok=1;
 /* Source screen0 has only child3 and root0 activation scripts. Postorder. */
 const unsigned order[]={3,0};for(unsigned i=0;i<2&&ok;++i){unsigned script,found;ok=nflstreet_ui_event(copy,n,base,order[i],0,0xfffe,0,&script,&found);if(!ok||!found){ok=0;break;}
  nflstreet_ui_script vm={0};vm.flags=state.entry_flags;vm.pc=base+script;vm.words[0]=0;vm.words[1]=0xfffe;vm.words[2]=be(copy+table-base+order[i]*20);vm.sp=4;
  ok=nflstreet_ui_script_services(copy,n,base,0x70000000,&vm,10000,nflstreet_ui_background_service,&state,sizeof state);if(ok)state.entry_flags=vm.flags;
 }if(ok){memcpy(b,copy,n);*s=state;}free(copy);return ok;
}
