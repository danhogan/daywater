#include "focus.h"
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint8_t*ptr(uint8_t*b,size_t n,unsigned base,unsigned v,size_t z){if(v<base||v-base>n||z>n-(v-base))return NULL;return b+v-base;}
typedef struct {nflstreet_ui_focus_effects effects;unsigned flags,index;
 nflstreet_ui_script_service service;void*ctx;size_t ctxsize;int execute;
 uint32_t current_screen,linked_screen,stack_base;
 nflstreet_ui_script_memory_service memory;} focus_dispatch;
/* Complete original 801D4474/4514 direct nested-node traversal. */
static int group(uint8_t*b,size_t n,unsigned base,uint8_t*nodes,unsigned count,unsigned parent,unsigned target,unsigned*contains,unsigned*active){
 uint8_t*r=nodes+parent*20;unsigned members=be(r+4);uint8_t*list=ptr(b,n,base,be(r+8),(size_t)members*4);if(!list)return 0;*contains=0;*active=UINT32_MAX;
 for(unsigned i=0;i<members;++i){uint8_t*object=ptr(b,n,base,be(list+i*4),12);if(!object)return 0;unsigned links=be(object+4);uint8_t*p=ptr(b,n,base,be(object+8),(size_t)links*8);if(!p)return 0;
  for(unsigned j=0;j<links;++j){uint8_t*l=p+j*8;if(sh(l)!=65535)continue;unsigned child=be(l+4);if(child>=count)return 0;
   if(child==target)*contains=1;uint8_t*root=ptr(b,n,base,be(nodes+child*20),8);if(!root)return 0;if(*active==UINT32_MAX&&be(root+4))*active=child;
  }
 }return 1;
}
static int request(uint8_t*b,size_t n,unsigned base,uint8_t*nodes,unsigned node,unsigned kind,unsigned event,unsigned target,unsigned arg,focus_dispatch*out){
 unsigned script,found;if(!nflstreet_ui_event(b,n,base,node,kind,(uint16_t)event,(uint16_t)target,&script,&found))return 0;
 if(found){if(out->effects.count>=4)return 0;unsigned root=be(nodes+node*20);if(!ptr(b,n,base,root,8))return 0;out->effects.requests[out->effects.count++]=(nflstreet_ui_focus_request){root-base,script,{arg,out->index}};
   if(out->execute){nflstreet_ui_script vm={0};vm.current_screen=out->current_screen;vm.linked_screen=out->linked_screen;vm.flags=out->flags;vm.pc=base+script;vm.sp=5;vm.words[1]=arg;vm.words[2]=out->index;vm.root=root;vm.words[3]=root;
   if(!nflstreet_ui_script_services_memory(b,n,base,out->stack_base,&vm,10000,out->service,out->ctx,out->ctxsize,NULL,out->memory))return 0;out->flags=vm.flags;
  }
 }return 1;
}
static int focus(uint8_t*b,size_t n,unsigned base,uint8_t*nodes,unsigned count,unsigned target,unsigned selected,unsigned arg,focus_dispatch*out){
 uint8_t*root=ptr(b,n,base,be(nodes+target*20),8);if(!root)return 0;
 unsigned owner=UINT32_MAX;
 /* Original scans all nodes in reverse; lowest matching owner survives. */
 for(unsigned i=count;i-- >0;){unsigned events=be(nodes+i*20+12);uint8_t*p=ptr(b,n,base,be(nodes+i*20+16),(size_t)events*8);if(!p)return 0;
  for(unsigned j=0;j<events;++j){unsigned flags=sh(p+j*8);if(!(flags&0xc000)&&(flags&0x2fff)==target){owner=i;break;}}
 }
 if(selected){for(unsigned i=0;i<count;++i){unsigned contains,active;if(!group(b,n,base,nodes,count,i,target,&contains,&active))return 0;
   if(contains){unsigned saved=out->index;out->index=UINT32_MAX;int ok=active==UINT32_MAX||focus(b,n,base,nodes,count,active,0,0,out);out->index=saved;if(!ok)return 0;break;}
  }
 }
 put(root+4,selected);unsigned event=selected?0xfffa:0xfff9;
 if(!request(b,n,base,nodes,target,0,event,target,arg,out))return 0;
 if(owner!=UINT32_MAX&&!request(b,n,base,nodes,owner,2,event,target,arg,out))return 0;return 1;
}
int nflstreet_ui_focus(uint8_t*b,size_t n,uint32_t base,unsigned node,unsigned selected,uint32_t arg,nflstreet_ui_focus_effects*out){
 if(!b||!out||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||selected>1||node>=be(b)||be(b)>1024)return 0;
 uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);uint8_t*nodes=ptr(copy,n,base,be(copy+4),(size_t)be(copy)*20);focus_dispatch effects={0};effects.index=UINT32_MAX;
 int ok=nodes&&focus(copy,n,base,nodes,be(copy),node,selected,arg,&effects);if(ok){memcpy(b,copy,n);*out=effects.effects;}free(copy);return ok;
}

int nflstreet_ui_focus_indexed_execute(uint8_t*b,size_t n,uint32_t base,unsigned node,unsigned selected,uint32_t arg,nflstreet_ui_focus_effects*out,uint32_t*flags,nflstreet_ui_script_service service,void*ctx,size_t ctxsize,uint32_t list){
 return nflstreet_ui_focus_indexed_execute_context(b,n,base,node,selected,arg,out,flags,0,0,0x70000000,service,ctx,ctxsize,list,NULL);
}

int nflstreet_ui_focus_indexed_execute_context(uint8_t*b,size_t n,uint32_t base,unsigned node,unsigned selected,uint32_t arg,nflstreet_ui_focus_effects*out,uint32_t*flags,uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base,nflstreet_ui_script_service service,void*ctx,size_t ctxsize,uint32_t list,nflstreet_ui_script_memory_service memory){
 if(!b||!out||!flags||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||selected>1||node>=be(b)||be(b)>1024||(!service&&!memory)||!ctx||!ctxsize||ctxsize>8*1024*1024||!stack_base||sizeof(uint32_t)*256>UINT32_MAX-stack_base)return 0;
 if((uint64_t)base+n>stack_base&&(uint64_t)stack_base+sizeof(uint32_t)*256>base)return 0;
 uint8_t*copy=malloc(n);void*services=malloc(ctxsize);if(!copy||!services){free(copy);free(services);return 0;}memcpy(copy,b,n);memcpy(services,ctx,ctxsize);
 uint8_t*nodes=ptr(copy,n,base,be(copy+4),(size_t)be(copy)*20);focus_dispatch effects={0};effects.index=UINT32_MAX;effects.flags=*flags;effects.service=service;effects.ctx=services;effects.ctxsize=ctxsize;effects.execute=1;effects.current_screen=current_screen;effects.linked_screen=linked_screen;effects.stack_base=stack_base;effects.memory=memory;
 if(list){uint8_t*p=ptr(copy,n,base,list,8);if(!p||!nodes||be(p)>1024){free(copy);free(services);return 0;}unsigned count=be(p);p=ptr(copy,n,base,list+8,(size_t)count*4);if(!p){free(copy);free(services);return 0;}for(unsigned i=count;i-- >0;)if(be(p+i*4)+base==be(nodes+node*20)){effects.index=i;break;}}
 int ok=nodes&&focus(copy,n,base,nodes,be(copy),node,selected,arg,&effects);if(ok){memcpy(b,copy,n);memcpy(ctx,services,ctxsize);*out=effects.effects;*flags=effects.flags;}free(copy);free(services);return ok;
}

int nflstreet_ui_focus_execute(uint8_t*b,size_t n,uint32_t base,unsigned node,unsigned selected,uint32_t arg,nflstreet_ui_focus_effects*out,uint32_t*flags,nflstreet_ui_script_service service,void*ctx,size_t ctxsize){return nflstreet_ui_focus_indexed_execute(b,n,base,node,selected,arg,out,flags,service,ctx,ctxsize,0);}
