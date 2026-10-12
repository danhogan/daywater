#include "main_menu.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void shortput(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
static uint8_t*ptr(uint8_t*b,size_t n,unsigned base,unsigned a,size_t z){if(a<base||a-base>n||z>n-(a-base))return NULL;return b+a-base;}
typedef struct {uint8_t*b;size_t n;unsigned base,budget,found,mode,desired;
 nflstreet_ui_main_menu*state;nflstreet_ui_script_service class_service;void*class_context;} traversal;
static int search(traversal*t,unsigned row,unsigned kind,unsigned target,unsigned*found,unsigned*active){
 if(!t->budget--)return 0;uint8_t*r=ptr(t->b,t->n,t->base,row,kind==8?20:12);if(!r)return 0;unsigned cell=be(r);uint8_t*c=ptr(t->b,t->n,t->base,cell,4);if(!c)return 0;
 if(cell==target){*found=row;*active=!!be(c);return 1;}if(!be(c))return 1;
 unsigned count=be(r+4);uint8_t*list=ptr(t->b,t->n,t->base,be(r+8),(size_t)count*(kind==8?4:8));if(!list)return 0;
 for(unsigned i=0;i<count&&!*found;++i){unsigned child;
  if(kind==8){child=be(list+i*4);if(!search(t,child,7,target,found,active))return 0;}
  else if(sh(list+i*8)==65535){unsigned index=be(list+i*8+4);if(index>=be(t->b))return 0;child=be(t->b+4)+index*20;if(!search(t,child,8,target,found,active))return 0;}
 }return 1;
}
static int apply(traversal*t,unsigned row,unsigned kind,unsigned force){
 if(!t->budget--)return 0;uint8_t*r=ptr(t->b,t->n,t->base,row,kind==8?20:12);if(!r)return 0;unsigned cell=be(r);uint8_t*c=ptr(t->b,t->n,t->base,cell,4);if(!c)return 0;if(!force&&!be(c))return 1;
 unsigned count=be(r+4);uint8_t*list=ptr(t->b,t->n,t->base,be(r+8),(size_t)count*(kind==8?4:8));if(!list)return 0;
 for(unsigned i=0;i<count;++i){if(kind==8){if(!apply(t,be(list+i*4),7,0))return 0;}else{uint8_t*l=list+i*8;
  if(sh(l)==65535){unsigned index=be(l+4);if(index>=be(t->b)||!apply(t,be(t->b+4)+index*20,8,0))return 0;}
  else{unsigned pointercell=be(l+4),target=0;if(pointercell){uint8_t*p=ptr(t->b,t->n,t->base,pointercell,4);if(!p||be(p)>UINT32_MAX-t->base)return 0;target=t->base+be(p);}
   nflstreet_ui_service_request q={0};q.kind=0;q.target=target;q.class_id=sh(l);q.property=0xfffffffc;q.argc=1;q.args[0]=t->desired;unsigned ignored=0;
   if(t->class_service){if(!t->class_service(t->class_context,t->b,t->n,t->base,&q,&ignored))return 0;}else if(!nflstreet_ui_main_service(t->state,t->b,t->n,t->base,&q,&ignored))return 0;
  }
 }}return 1;
}
int nflstreet_ui_class_activation(uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,nflstreet_ui_script_service service,void*ctx){
 if(!b||n<32||!q||q->kind!=6||q->argc!=1||!service||!ctx)return 0;unsigned found=0,active=0;traversal t={b,n,base,4096,0,q->class_id,!!q->args[0],NULL,service,ctx};
 if(!search(&t,be(b+4),8,q->target,&found,&active))return 0;if(!found||active==t.desired)return 1;if(q->class_id!=7&&q->class_id!=8)return 1;return apply(&t,found,q->class_id,1);
}
int nflstreet_ui_main_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){
 nflstreet_ui_main_menu*s=ctx;(void)result;if(!s||!q)return 0;
 if(q->kind==3||q->kind==4||q->kind==5)return nflstreet_ui_rate_request(&s->rates,b,n,base,1,q);
 /*8010C3BC ->801E6958/801E6820: first matching supplied registry ID. */
 if(q->kind==1&&q->property==0x108&&q->argc==0){if(s->registry_count>127)return 0;int index=-1;for(unsigned i=0;i<s->registry_count;++i)if(s->registry[i]==s->registry_value){index=(int)i;break;}float f=(float)index;memcpy(result,&f,4);return 1;}
 /*8010C3BC query26 returns an integer word, not float1. */
 if(q->kind==1&&q->property==0x26&&q->argc==0){*result=!!((uint8_t)s->callback_byte_6f78||(uint8_t)s->callback_byte_7fcc||(s->callback_word_6f7c&&(uint8_t)s->callback_byte_6dd0));return 1;}
 /*1EF(-1,index):801E67F4 then801E6914; unsigned stored ID. */
 if(q->kind==1&&q->property==0x1ef&&q->argc==2&&q->args[0]==UINT32_MAX){int index=(int8_t)q->args[1];if(index<0||s->registry_count>127)return 0;s->registry_value=(unsigned)index<s->registry_count?s->registry[index]:UINT32_MAX;return 1;}
 /*8010C3BC ->8010C3AC stores the raw argument==1 boolean. */
 if(q->kind==1&&q->property==0x11f&&q->argc==1){s->callback_word_6f7c=q->args[0]==1;return 1;}
 /*8010CB80 branch8D reaches the shared result=0 epilogue. */
 if(q->kind==1&&q->property==0x8d&&q->argc==0){*result=0;return 1;}
 if(q->kind==1&&q->property==0x122&&q->argc==0){float f=(float)(uint8_t)s->callback_byte_6f79;memcpy(result,&f,4);return 1;}
 if(q->kind==1&&q->property==0x80000003&&q->argc==1){s->callback_word_6e74=q->args[0];return 1;}
 /*8011B0E0(8000000C) has no effect unless its argument is73. */
 if(q->kind==1&&q->property==0x8000000c&&q->argc==1&&q->args[0]!=0x73)return 1;
 if(q->kind==1&&q->property==0x8000000b&&q->argc==1){s->callback_word_6e68=q->args[0];return 1;}
 if(q->kind==6){if(q->argc!=1)return 0;unsigned found=0,active=0;traversal t={b,n,base,4096,0,q->class_id,!!q->args[0],s,NULL,NULL};
  if(!search(&t,be(b+4),8,q->target,&found,&active))return 0;
  if(!found||active==t.desired)return 1;if(q->class_id!=7&&q->class_id!=8)return 1;return apply(&t,found,q->class_id,1);
 }
 if(q->kind==8&&q->property==3&&!q->argc&&s->link_enabled&&q->target==s->linked_screen){if(s->link_count>=16)return 0;s->links[s->link_count++]=q->target;return 1;}
 if(q->kind==7){if(q->argc!=2||s->pending_count>=128)return 0;s->pending[s->pending_count++]=*q;return 1;}
 if(q->kind==2){if(s->broadcast_count>=128)return 0;s->broadcasts[s->broadcast_count++]=*q;return 1;}
 if(q->kind!=0)return 0;
 if(q->class_id==1&&q->property==0&&q->argc==1&&q->target){if(s->audio_count>=128)return 0;s->audio[s->audio_count++]=q->args[0];return 1;}
 if(q->class_id==8&&q->property==0xfffffffc)return 1;
 uint8_t*o=ptr(b,n,base,q->target,q->class_id==8?40:16);if(!o)return 0;
 if(q->class_id==8&&q->property==0&&q->argc==4){for(unsigned i=0;i<4;++i)o[0x18+i]=(uint8_t)q->args[i];return 1;}
 if(q->class_id==8){
  if(o[2]!=3||o[3]!=2||!s->font.bytes)return 0;
  if(q->property==5&&q->argc==1){unsigned flags=sh(o)&0xfffc;if((int32_t)q->args[0]>=0)flags|=q->args[0]==0?1:2;shortput(o,flags);return 1;}
  uint8_t*d=ptr(b,n,base,be(o+32),12);if(!d)return 0;uint8_t*text=ptr(b,n,base,be(d+8),1);if(!text)return 0;size_t available=n-(size_t)(text-b);uint8_t*end=memchr(text,0,available);if(!end)return 0;
  if(q->property==3&&q->argc==1){uint8_t*src=ptr(b,n,base,q->args[0],12);if(!src)return 0;uint8_t*value=ptr(b,n,base,be(src+8),1);if(!value)return 0;uint8_t*last=memchr(value,0,n-(size_t)(value-b));if(!last)return 0;size_t length=(size_t)(last-value),capacity=be(d+4);if(capacity>=available||length>capacity)return 0;size_t copied=length;memmove(text,value,copied);text[copied]=0;return nflstreet_ui_text_prepare(o,&s->font,(const char*)text);}
  if(q->property==12&&q->argc==1){if(sh(o)&0x8000){if(!nflstreet_ui_text_prepare(o,&s->font,(const char*)text))return 0;}if(o[15]!=1||sh(o+36)!=0)return 0;unsigned length=(unsigned)(end-text),index=q->args[0];if(index&&index>=length)index=length-1;unsigned width=0;for(unsigned i=0;i<index&&i<sh(o+38);++i){const uint8_t*g=nflstreet_font_glyph(&s->font,text[i]);if(g)width+=(int8_t)g[8];}float f=(float)(int16_t)width;memcpy(result,&f,4);return 1;}
  return 0;
 }
 if(q->class_id!=0)return 0;
 /* Class0 activation on the supplied static (non-streaming) archive3. */
 if(q->property==0xfffffffc&&q->argc==1){if(sh(o+6)!=1&&sh(o+6)!=3)return 0;shortput(o,sh(o)|0x4000);return 1;}
 if(q->property!=5||q->argc!=1)return 0;unsigned archive=q->args[0]>>16,entry=q->args[0]&65535,oldarchive=sh(o+6),oldentry=sh(o+4);
 if(archive==0||archive==65535)archive=oldarchive;if(archive!=3||oldarchive!=3||entry>1||oldentry>1)return 0;
 if(entry!=oldentry){if(s->resource_count>126)return 0;s->resources[s->resource_count++]=(nflstreet_ui_button_effect){0,oldarchive,oldentry};s->resources[s->resource_count++]=(nflstreet_ui_button_effect){1,archive,entry};shortput(o+4,entry);}
 shortput(o+12,65535);shortput(o+14,65535);shortput(o,sh(o)|0x4000);return 1;
}
int nflstreet_ui_main_select(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_main_menu*s,unsigned node,nflstreet_ui_focus_effects*out){
 if(!s)return 0;return nflstreet_ui_focus_execute(b,n,base,node,1,0,out,&s->entry_flags,nflstreet_ui_main_service,s,sizeof *s);
}

int nflstreet_ui_main_text_bind(uint8_t*b,size_t n,uint32_t base){
 if(!b||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base)return 0;unsigned roots[]={0xa5c,0xbdc,0xd5c,0xedc,0x105c,0x11dc};
 unsigned objects[6],descriptors[6];for(unsigned i=0;i<6;++i){uint8_t*cell=ptr(b,n,base,base+roots[i]+0x17c,4);if(!cell)return 0;unsigned off=be(cell);if(off>n||40>n-off)return 0;unsigned relative=be(b+off+32);if(relative>n-off||12>n-off-relative)return 0;objects[i]=off;descriptors[i]=base+off+relative;}
 for(unsigned i=0;i<6;++i)put(b+objects[i]+32,descriptors[i]);return 1;
}
int nflstreet_ui_main_layout(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_main_menu*s,unsigned mode){
 if(!s||mode>4||n<=0xb229)return 0;nflstreet_ui_script vm={0};vm.flags=s->entry_flags;vm.sp=4;vm.pc=base+0xb229;vm.words[1]=mode;vm.words[2]=base+0x960;
 if(!nflstreet_ui_script_services(b,n,base,0x70000000,&vm,10000,nflstreet_ui_main_service,s,sizeof *s))return 0;s->entry_flags=vm.flags;return 1;
}

static int drain(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_main_menu*s,unsigned first){
 uint8_t*nodes=ptr(b,n,base,be(b+4),(size_t)be(b)*20);if(!nodes||s->pending_count>128)return 0;
 for(unsigned i=first;i<s->pending_count;++i){nflstreet_ui_service_request q=s->pending[i];unsigned target=be(b);for(unsigned j=0;j<be(b);++j)if(be(nodes+j*20)==q.target){target=j;break;}if(target==be(b))return 0;nflstreet_ui_focus_effects effects={0};if(!nflstreet_ui_focus_indexed_execute(b,n,base,target,1,(uint32_t)(int32_t)(int16_t)q.property,&effects,&s->entry_flags,nflstreet_ui_main_service,s,sizeof *s,q.args[0]))return 0;}return 1;
}
int nflstreet_ui_main_move(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_main_menu*s,unsigned node,unsigned event){
 if(!b||!s||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||node>=be(b)||(event!=2&&event!=3)||s->pending_count>128)return 0;uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_main_menu state=*s;unsigned first=state.pending_count,script,found;int ok=0;
 uint8_t*nodes=ptr(copy,n,base,be(copy+4),(size_t)be(copy)*20);if(!nodes||!nflstreet_ui_event(copy,n,base,node,0,event,node,&script,&found)||!found)goto done;unsigned root=be(nodes+node*20);
 nflstreet_ui_script vm={0};vm.root=root;vm.pc=base+script;vm.flags=state.entry_flags;vm.sp=5;vm.words[1]=event;vm.words[2]=0x3f800000;vm.words[3]=root;
 if(!nflstreet_ui_script_services(copy,n,base,0x70000000,&vm,10000,nflstreet_ui_main_service,&state,sizeof state))goto done;state.entry_flags=vm.flags;
 if(!drain(copy,n,base,&state,first))goto done;ok=1;memcpy(b,copy,n);*s=state;
 done:free(copy);return ok;
}

int nflstreet_ui_main_control(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_main_menu*s,unsigned node,unsigned event){
 if(!b||!s||n<32||n>8*1024*1024||!base||n>UINT32_MAX-base||node>=be(b)||(event!=6&&event!=7))return 0;uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_main_menu state=*s;unsigned first=state.pending_count,script,found;int ok=0;
 if(!nflstreet_ui_event(copy,n,base,0,event==7?2:0,event,node,&script,&found)||!found)goto done;
 uint8_t*nodes=ptr(copy,n,base,be(copy+4),(size_t)be(copy)*20);if(!nodes)goto done;nflstreet_ui_script vm={0};vm.root=be(nodes);vm.pc=base+script;vm.flags=state.entry_flags;vm.sp=5;vm.words[1]=event;vm.words[2]=0x3f800000;vm.words[3]=vm.root;
 if(!nflstreet_ui_script_services(copy,n,base,0x70000000,&vm,10000,nflstreet_ui_main_service,&state,sizeof state))goto done;state.entry_flags=vm.flags;if(!drain(copy,n,base,&state,first))goto done;ok=1;memcpy(b,copy,n);*s=state;
 done:free(copy);return ok;
}
static int main_tick(void*ctx,nflstreet_ui_rates*rates,uint8_t*b,size_t n,uint32_t base,uint32_t screen,uint32_t root,uint32_t script,unsigned argc,const uint32_t*args,float*result){
 nflstreet_ui_main_menu*s=ctx;s->rates=*rates;if(screen!=1||argc>16)return 0;nflstreet_ui_script vm={0};vm.root=root;vm.current_screen=screen;vm.linked_screen=s->linked_screen;vm.flags=s->entry_flags;vm.pc=script;memcpy(vm.words+vm.sp++,result,4);
 for(unsigned i=0;i<argc;++i)vm.words[vm.sp++]=args[i];vm.words[vm.sp++]=root;vm.words[vm.sp++]=0;
 if(!nflstreet_ui_script_services(b,n,base,0x70000000,&vm,10000,nflstreet_ui_main_service,s,sizeof *s))return 0;s->entry_flags=vm.flags;*rates=s->rates;memcpy(result,vm.words,4);return 1;
}
int nflstreet_ui_main_advance(uint8_t*b,size_t n,uint32_t base,nflstreet_ui_main_menu*s,uint32_t elapsed){
 if(!b||!s||!n||n>8*1024*1024)return 0;uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_main_menu state=*s;unsigned first=state.pending_count;int ok=nflstreet_ui_rate_update(&state.rates,copy,n,base,1,elapsed,0,main_tick,&state);if(ok)ok=drain(copy,n,base,&state,first);if(ok){memcpy(b,copy,n);*s=state;}free(copy);return ok;
}
