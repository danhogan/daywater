#include "coroutine.h"
#include <stdlib.h>
#include <string.h>
int nflstreet_ui_coroutine_capture(void*ctx,const nflstreet_ui_script*vm,const nflstreet_ui_service_request*q){
 nflstreet_ui_coroutines*s=ctx;if(!s||!vm||!q||s->capacity>8||s->count>s->capacity||vm->suspension!=3||!vm->sp||vm->sp>256||q->kind!=9||q->property||q->argc>16)return 0;
 if(s->count==s->capacity)return 2;s->frames[s->count++]=(nflstreet_ui_continuation){*vm,*q};return 1;
}
int nflstreet_ui_coroutine_resume(nflstreet_ui_coroutines*s,uint32_t destination,uint32_t result,uint8_t*b,size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*out,nflstreet_ui_script_service service,void*ctx,size_t ctxsize,unsigned*completed){
 if(!s||!out||!completed||s->capacity>8||s->count>s->capacity||!b||!n||n>8*1024*1024||(ctxsize&&!ctx)||ctxsize>8*1024*1024)return 0;
 /* A context containing the queue would overwrite the committed pop. */
 if(ctxsize&&((uintptr_t)s<(uintptr_t)ctx+ctxsize&&(uintptr_t)ctx<(uintptr_t)s+sizeof *s))return 0;
 if(!s->count){*completed=0;return 1;}const nflstreet_ui_continuation*f=&s->frames[s->count-1];
 if(f->request.target!=destination){for(unsigned i=0;i<s->count;++i)if(s->frames[i].request.target==destination||s->frames[i].request.source==destination)return 0;*completed=0;return 1;}
 uint8_t*copy=malloc(n);void*state=ctxsize?malloc(ctxsize):NULL;if(!copy||(ctxsize&&!state)){free(copy);free(state);return 0;}memcpy(copy,b,n);if(ctxsize)memcpy(state,ctx,ctxsize);
 nflstreet_ui_coroutines local=*s;--local.count;nflstreet_ui_script vm=f->vm;int ok=vm.sp&&vm.sp<=256&&vm.suspension==3;
 if(ok){vm.suspension=0;vm.words[vm.sp-1]=result;ok=nflstreet_ui_script_services_capture(copy,n,base,stackbase,&vm,100000,service,state,ctxsize,NULL);}
 /* Subsequent suspension needs an explicit owner callback and is unsupported
    in this first resume API; failure retains the complete previous frame. */
 if(ok){memcpy(b,copy,n);*s=local;*out=vm;if(ctxsize)memcpy(ctx,state,ctxsize);*completed=1;}free(copy);free(state);return ok;
}
