#include "controller_screen.h"
#include "events.h"
#include "construct.h"
#include "text_construct.h"
#include "image_construct.h"
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int nflstreet_controller_screen_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*out){
 nflstreet_controller_screen*s=ctx;if(!s||!b||!q||!out||!base||n>UINT32_MAX-base)return 0;
 if(q->kind==0&&q->property==UINT32_MAX){if(q->target<base||q->target-base>n)return 0;
  if(q->class_id==8){if(n-(q->target-base)<40||b[q->target-base+2]>3)return 0;unsigned entry=b[q->target-base+2];return nflstreet_ui_text_construct(b,n,base,q->target,entry==3?&s->classes.shared.font:&s->classes.fonts[entry]);}
  if(q->class_id==0){unsigned requested;uint32_t identity;return nflstreet_ui_image_construct(b,n,base,q->target,&requested,&identity);}return 0;
 }
 if(q->kind==10&&q->argc==1){
  if(!q->target||!q->property)return 1;
  if(q->target<base||q->property<base||q->args[0]<base||q->target-base>n||q->property-base>n||q->args[0]-base>n||n-(q->target-base)<12||n-(q->property-base)<12||n-(q->args[0]-base)<12)return 0;
  uint8_t*d=b+q->target-base;uint32_t cap=be(d+4),dest=be(d+8),format=be(b+q->property-base+8),source=be(b+q->args[0]-base+8);
  if(format<base||format-base>n||n-(format-base)<6)return 0;
  if(!memcmp(b+format-base,"%s...",6)){
   if(!cap)return 1;if(dest<base||dest-base>n||cap==UINT32_MAX||n-(dest-base)<(size_t)cap+1)return 0;
   const uint8_t*text=(const uint8_t*)"(null)";size_t length=6;
   if(source){if(source<base||source-base>=n)return 0;text=b+source-base;const uint8_t*end=memchr(text,0,n-(source-base));if(!end)return 0;length=(size_t)(end-text);if(dest<source+length+1&&source<dest+cap+1)return 0;}
   unsigned copied=0;for(size_t i=0;i<length&&copied<cap;++i)b[dest-base+copied++]=text[i];for(unsigned i=0;i<3&&copied<cap;++i)b[dest-base+copied++]='.';b[dest-base+copied]=0;return 1;
  }
 }
 if(q->kind==3||q->kind==4||q->kind==5)return nflstreet_ui_rate_request(&s->classes.shared.rates,b,n,base,6,q);
 if(q->kind==0&&q->class_id==0&&q->property==10&&q->argc==2){if(q->target<base||q->target-base>n||n-(q->target-base)<36||(q->args[0]>=4&&q->args[0]!=UINT32_MAX))return 0;
  uint8_t*o=b+q->target-base;unsigned first=q->args[0]==UINT32_MAX?0:q->args[0],end=q->args[0]==UINT32_MAX?4:first+1;for(unsigned i=first;i<end;++i)o[20+i*4]=(uint8_t)q->args[1];o[0]|=0x40;return 1;}
 if(q->kind==0&&q->class_id==0&&q->property==0xfffffffc&&q->argc==1){if(q->target<base||q->target-base>n||n-(q->target-base)<36)return 0;uint8_t*o=b+q->target-base;unsigned archive=(unsigned)o[6]<<8|o[7];if((o[1]&2)&&(archive>=32||!(s->supplied_static_archives&(1u<<archive))))return 0;o[0]|=0x40;return 1;} /* Supplied archive flags exclude original streaming bit2. */
 if(q->kind==6)return nflstreet_ui_class_activation(b,n,base,q,nflstreet_controller_screen_service,s);
 if(q->kind==1&&q->property==0x1ef&&q->argc==2&&q->args[0]<=1&&q->args[1]<4){if(s->game.devices.count>=NFLSTREET_CONTROLLER_DEVICE_EFFECTS)return 0;s->game.devices.effects[s->game.devices.count++]=(nflstreet_controller_effect){0x801278b4,2,{q->args[1],q->args[0]!=0,0}};return 1;} /* Bounded source1EF raw0/1 -> float nonzero comparison. */
 if(q->kind==1&&(q->property==0x104||q->property==0x105)&&!q->argc){*out=0;return 1;} /* Same supplied80009E5C/6C=false boundary as local initialization; float0. */
 if(q->kind==1&&q->property==0xcb&&!q->argc){*out=0;return 1;} /* Original8010C3BC common zero-result branch. */
 if(q->kind==1&&q->property==0x139&&q->argc==1){if(q->args[0]>=4)return 0;int32_t index=-1;for(unsigned i=0;i<10;++i)if(s->game.devices.bindings[i]==q->args[0]){index=(int32_t)i;break;}float value=(float)index;memcpy(out,&value,4);return 1;}
 if(q->kind==1)return nflstreet_ui_main_service(&s->classes.shared,b,n,base,q,out);
 if(q->kind==0&&q->class_id==8&&q->target>=base&&q->target-base<=n&&n-(q->target-base)>=40&&b[q->target-base+2]==3)return nflstreet_ui_main_service(&s->classes.shared,b,n,base,q,out);
 if(q->kind==0||q->kind==10)return nflstreet_ui_options_screen_service(&s->classes,b,n,base,q,out);
 return nflstreet_ui_main_service(&s->classes.shared,b,n,base,q,out);
}
int nflstreet_controller_screen_memory(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*out,uint32_t sb,uint32_t words[256]){
 nflstreet_controller_screen*s=ctx;if(!s||!b||!q||!out||!words)return 0;
 if(q->kind==1&&q->property==10&&q->argc==1){uint32_t addr=q->args[0],value;if(addr>=base&&addr-base<=n&&n-(addr-base)>=4){uint8_t*p=b+addr-base;uint32_t index=be(p);if(index>=10)return 0;value=s->game.devices.bindings[index];p[0]=value>>24;p[1]=value>>16;p[2]=value>>8;p[3]=value;return 1;}if(addr>=sb&&addr-sb<=1020&&!((addr-sb)&3)){uint32_t index=words[(addr-sb)/4];if(index>=10)return 0;value=s->game.devices.bindings[index];words[(addr-sb)/4]=value;return 1;}return 0;}
if(q->kind==1&&(q->property>=0x80000000||q->property==8))return nflstreet_controller_callback_memory(&s->game,b,n,base,q,out,sb,words);
 return nflstreet_controller_screen_service(ctx,b,n,base,q,out);
}
int nflstreet_controller_screen_construct(uint8_t*b,size_t n,uint32_t base,nflstreet_controller_screen*s){return s&&nflstreet_ui_construct(b,n,base,0,UINT32_MAX,0x101,nflstreet_controller_screen_service,s,sizeof *s);}
int nflstreet_controller_screen_start(uint8_t*b,size_t n,uint32_t base,nflstreet_controller_screen*s){
 if(!b||!s||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base)return 0;uint8_t*copy=malloc(n);nflstreet_controller_screen*next=malloc(sizeof *next);if(!copy||!next){free(copy);free(next);return 0;}memcpy(copy,b,n);*next=*s;unsigned executed;
 int ok=nflstreet_ui_events_memory(copy,n,base,6,5,&next->classes.shared.entry_flags,0,0xfffe,0,0,NULL,nflstreet_controller_screen_service,next,sizeof *next,&executed,NULL,nflstreet_controller_screen_memory);
 uint32_t rows=be(copy+4);if(rows<base||rows-base>n||n-(rows-base)<20)ok=0;uint32_t root=ok?be(copy+rows-base):0;if(root<base||root-base>n||n-(root-base)<8)ok=0;
 if(ok){memset(copy+root-base+4,0,4);copy[root-base+7]=1;ok=nflstreet_ui_events_memory(copy,n,base,6,5,&next->classes.shared.entry_flags,0,0xfffc,0,0,NULL,nflstreet_controller_screen_service,next,sizeof *next,&executed,NULL,nflstreet_controller_screen_memory);}
 if(ok){memcpy(b,copy,n);*s=*next;}free(copy);free(next);return ok;
}
static int tick(void*ctx,nflstreet_ui_rates*r,uint8_t*b,size_t n,uint32_t base,uint32_t id,uint32_t root,uint32_t pc,unsigned argc,const uint32_t*args,float*out){
 nflstreet_controller_screen*s=ctx;if(id!=6||argc>16)return 0;s->classes.shared.rates=*r;nflstreet_ui_script vm={0};vm.current_screen=6;vm.linked_screen=5;vm.root=root;vm.pc=pc;vm.flags=s->classes.shared.entry_flags;memcpy(vm.words+vm.sp++,out,4);
 for(unsigned i=0;i<argc;++i)vm.words[vm.sp++]=args[i];vm.words[vm.sp++]=root;vm.words[vm.sp++]=0;
 if(!nflstreet_ui_script_services_memory(b,n,base,0x817c0000,&vm,100000,nflstreet_controller_screen_service,s,sizeof *s,NULL,nflstreet_controller_screen_memory))return 0;
 s->classes.shared.entry_flags=vm.flags;*r=s->classes.shared.rates;memcpy(out,vm.words,4);return 1;
}
int nflstreet_controller_screen_advance(uint8_t*b,size_t n,uint32_t base,nflstreet_controller_screen*s,uint32_t elapsed){
 if(!b||!s||n<32||n>8*1024*1024)return 0;uint8_t*copy=malloc(n);nflstreet_controller_screen*next=malloc(sizeof *next);if(!copy||!next){free(copy);free(next);return 0;}memcpy(copy,b,n);*next=*s;
 int ok=nflstreet_ui_rate_update(&next->classes.shared.rates,copy,n,base,6,elapsed,0,tick,next);if(ok){memcpy(b,copy,n);*s=*next;}free(copy);free(next);return ok;
}

/* Original8012C14C ->801D094C supplies one float word after logical port.
   This consumes already translated source events, not physical controller input. */
int nflstreet_controller_screen_input(uint8_t*b,size_t n,uint32_t base,nflstreet_controller_screen*s,uint32_t port,uint16_t event,float value){
 if(!s||port>=10||event<2||event>7||!(value>=0&&value<=1))return 0;uint32_t word;memcpy(&word,&value,4);unsigned executed;
 return nflstreet_ui_events_memory(b,n,base,6,5,&s->classes.shared.entry_flags,0,event,port,1,&word,nflstreet_controller_screen_service,s,sizeof *s,&executed,NULL,nflstreet_controller_screen_memory);
}
