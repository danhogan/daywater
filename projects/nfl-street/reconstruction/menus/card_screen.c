#include "card_screen.h"
#include "events.h"
#include "construct.h"
#include "text_construct.h"
#include "image_construct.h"
#include <string.h>
#include <stdlib.h>
int nflstreet_card_screen_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*out){
 nflstreet_card_screen*s=ctx;if(!s||!q||!out)return 0;
 if(q->kind==9&&q->property==1&&q->source==0x60004&&q->target==0x60004&&!q->argc){if(s->request_count>=8)return 0;s->requests[s->request_count++]=*q;return 1;}
 if(q->kind==1&&q->property>=0x80000002&&q->property<=0x80000006){unsigned argc=q->property==0x80000003||q->property==0x80000006?1:0;if(q->argc!=argc)return 0;uint32_t handled;return nflstreet_storage_common(&s->storage,q->property,argc?q->args[0]:0,&handled);}
 if(q->kind==1&&q->property==0x178&&!q->argc){if(s->storage.callbacks[1]&&s->storage.callbacks[1]!=0x80121fec)return 0;if(s->storage.word_573c<2){if(s->storage.count>=16)return 0;s->storage.requests[s->storage.count++]=(nflstreet_storage_request){0x8018f150,0,{0,0}};}s->storage.word_573c=UINT32_MAX;if(s->storage.callbacks[1])s->word_5a14=0;return 1;}
 if(q->kind==1&&q->property==0x21&&!q->argc){*out=s->ui.profile[13]==1&&s->ui.options.query_4668!=0;return 1;}
 if(q->kind==1&&q->property==0x1f8&&q->argc==2){*out=0;if(!s->ui.options.query_4668)return 1;
  s->operation=q->args[0];*out=s->query_69a8;if(!*out)return 1;
  if(!memchr(s->device_name,0,sizeof s->device_name))return 0;
  /* Supplied device-query result/name; physical mount request retained. */
  s->device[0]=q->args[0];s->device[1]=q->args[1];memcpy(s->selected_name,s->device_name,strlen(s->device_name)+1);
  if(!s->query_6d6c){if(s->storage.count>=16)return 0;s->storage.requests[s->storage.count++]=(nflstreet_storage_request){0x80007228,2,{q->args[0],0}};}return 1;}
 if(q->kind==1&&q->property==0x5e&&q->argc==1){s->byte_6f77=(uint8_t)q->args[0];return 1;}
 if(q->kind==1)return nflstreet_ui_main_service(&s->ui.shared,b,n,base,q,out);
 if(q->kind==3||q->kind==4||q->kind==5)return nflstreet_ui_rate_request(&s->ui.shared.rates,b,n,base,0x60004,q);
 if(q->kind==6)return nflstreet_ui_class_activation(b,n,base,q,nflstreet_card_screen_service,s);
 if(q->kind==0&&q->property==UINT32_MAX){if(q->target<base||q->target-base>=n)return 0;if(q->class_id==8){if(n-(q->target-base)<40||b[q->target-base+2]>2)return 0;return nflstreet_ui_text_construct(b,n,base,q->target,&s->ui.fonts[b[q->target-base+2]]);}if(q->class_id==0){unsigned request;uint32_t identity;return nflstreet_ui_image_construct(b,n,base,q->target,&request,&identity);}return 0;}
 return nflstreet_ui_options_screen_service(&s->ui,b,n,base,q,out);
}
int nflstreet_card_screen_construct(uint8_t*b,size_t n,uint32_t base,nflstreet_card_screen*s){return s&&nflstreet_ui_construct(b,n,base,0,UINT32_MAX,0x101,nflstreet_card_screen_service,s,sizeof *s);}
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int nflstreet_card_screen_start(uint8_t*b,size_t n,uint32_t base,nflstreet_card_screen*s,const uint32_t args[2],unsigned*executed){
 if(!b||!s||!args||!executed||n<32||n>8*1024*1024||be(b)!=2)return 0;uint8_t*copy=malloc(n);nflstreet_card_screen*local=malloc(sizeof *local);if(!copy||!local){free(copy);free(local);return 0;}memcpy(copy,b,n);*local=*s;unsigned count=0,pc=0,found=0;
 int ok=nflstreet_ui_events_linked(copy,n,base,0x60004,0x50000,&local->ui.shared.entry_flags,1,0xfffe,0,0,NULL,nflstreet_card_screen_service,local,sizeof *local,&count)&&nflstreet_ui_event(copy,n,base,0,0,0xfffe,0,&pc,&found)&&found;
 if(ok){uint32_t rows=be(copy+4);if(rows<base||rows-base>n||n-(rows-base)<20)ok=0;else{nflstreet_ui_script vm={0};vm.current_screen=0x60004;vm.linked_screen=0x50000;vm.flags=local->ui.shared.entry_flags;vm.root=be(copy+rows-base);vm.pc=base+pc;vm.sp=6;vm.words[1]=0xfffe;vm.words[2]=args[0];vm.words[3]=args[1];vm.words[4]=vm.root;ok=nflstreet_ui_script_services(copy,n,base,0x817c0000,&vm,100000,nflstreet_card_screen_service,local,sizeof *local);local->ui.shared.entry_flags=vm.flags;}}
 if(ok){memcpy(b,copy,n);*s=*local;*executed=count+1;}free(copy);free(local);return ok;
}

static int capture(void*ctx,const nflstreet_ui_script*vm,const nflstreet_ui_service_request*q){nflstreet_card_screen*s=ctx;return nflstreet_ui_coroutine_capture(&s->ui.coroutines,vm,q);}
static int tick(void*ctx,nflstreet_ui_rates*r,uint8_t*b,size_t n,uint32_t base,uint32_t id,uint32_t root,uint32_t pc,unsigned argc,const uint32_t*args,float*result){
 nflstreet_card_screen*s=ctx;if(id!=0x60004||argc>16)return 0;s->ui.shared.rates=*r;nflstreet_ui_script vm={0};vm.current_screen=id;vm.linked_screen=0x50000;vm.root=root;vm.pc=pc;vm.flags=s->ui.shared.entry_flags;memcpy(vm.words+vm.sp++,result,4);for(unsigned i=0;i<argc;++i)vm.words[vm.sp++]=args[i];vm.words[vm.sp++]=root;vm.words[vm.sp++]=0;
 if(!nflstreet_ui_script_services_capture(b,n,base,0x817c0000,&vm,100000,nflstreet_card_screen_service,s,sizeof *s,s->ui.coroutines.capacity?capture:NULL))return 0;s->ui.shared.entry_flags=vm.flags;*r=s->ui.shared.rates;memcpy(result,vm.words,4);return 1;
}
int nflstreet_card_screen_advance(uint8_t*b,size_t n,uint32_t base,nflstreet_card_screen*s,uint32_t elapsed){if(!b||!s||n<32||n>8*1024*1024)return 0;uint8_t*copy=malloc(n);nflstreet_card_screen*local=malloc(sizeof *local);if(!copy||!local){free(copy);free(local);return 0;}memcpy(copy,b,n);*local=*s;int ok=nflstreet_ui_rate_update(&local->ui.shared.rates,copy,n,base,0x60004,elapsed,0,tick,local);if(ok){memcpy(b,copy,n);*s=*local;}free(copy);free(local);return ok;}

int nflstreet_card_screen_complete(uint8_t*b,size_t n,uint32_t base,nflstreet_card_screen*s,uint32_t destination,uint32_t result,unsigned*completed){
 if(!b||!s||!completed||n<32||n>8*1024*1024||s->ui.coroutines.capacity>8||s->ui.coroutines.count>s->ui.coroutines.capacity)return 0;
 if(!s->ui.coroutines.count){*completed=0;return 1;}nflstreet_ui_continuation*f=&s->ui.coroutines.frames[s->ui.coroutines.count-1];if(f->request.target!=destination){for(unsigned i=0;i<s->ui.coroutines.count;++i)if(s->ui.coroutines.frames[i].request.target==destination||s->ui.coroutines.frames[i].request.source==destination)return 0;*completed=0;return 1;}
 uint8_t*copy=malloc(n);nflstreet_card_screen*state=malloc(sizeof *state);if(!copy||!state){free(copy);free(state);return 0;}memcpy(copy,b,n);*state=*s;nflstreet_ui_script vm=f->vm;--state->ui.coroutines.count;int ok=vm.sp&&vm.sp<=256&&vm.suspension==3;
 if(ok){vm.suspension=0;vm.words[vm.sp-1]=result;vm.flags=state->ui.shared.entry_flags;ok=nflstreet_ui_script_services_capture(copy,n,base,0x817c0000,&vm,100000,nflstreet_card_screen_service,state,sizeof *state,capture);}
 if(ok){state->ui.shared.entry_flags=vm.flags;memcpy(b,copy,n);*s=*state;*completed=1;}free(copy);free(state);return ok;
}
