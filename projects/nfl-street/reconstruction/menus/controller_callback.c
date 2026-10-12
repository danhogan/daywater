#include "controller_callback.h"
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int range(size_t n,uint32_t base,uint32_t addr,size_t z){return addr>=base&&(size_t)(addr-base)<=n&&z<=n-(addr-base);}
static int word(uint8_t*b,size_t n,uint32_t base,uint32_t sb,uint32_t words[256],uint32_t addr,uint32_t*v,int write){
 if(range(n,base,addr,4)){if(write)put(b+addr-base,*v);else *v=be(b+addr-base);return 1;}
 if(range(1024,sb,addr,4)&&!((addr-sb)&3)){if(write)words[(addr-sb)/4]=*v;else *v=words[(addr-sb)/4];return 1;}return 0;
}
int nflstreet_controller_callback_memory(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*out,uint32_t sb,uint32_t words[256]){
 nflstreet_controller_callback*s=ctx;if(!s||!b||!q||!out||!words||q->kind!=1)return 0;
 switch(q->property){
 case 8:{if(q->argc!=3)return 0;uint32_t ignored;if(!word(b,n,base,sb,words,q->args[2],&ignored,0))return 0;
  int selected=q->args[0]==99||q->args[0]==100;if(selected&&!word(b,n,base,sb,words,q->args[1],&ignored,0))return 0;uint32_t value=UINT32_MAX;
  if(!word(b,n,base,sb,words,q->args[2],&value,1))return 0;if(selected){value=q->args[0];if(!word(b,n,base,sb,words,q->args[1],&value,1))return 0;}return 1;}
 case 0x80000001:{if(q->argc!=1)return 0;uint32_t existing;if(!word(b,n,base,sb,words,q->args[0],&existing,0))return 0;nflstreet_controller_callback next=*s;uint32_t choice=next.committed.choice;
  if(!nflstreet_controller_initialize(&next.selection,&next.devices,next.next_mapping,&choice))return 0;next.committed.choice=choice;for(unsigned p=0;p<4;++p)next.handles[p]=next.devices.bindings[p];
  if(!word(b,n,base,sb,words,q->args[0],&choice,1))return 0;*s=next;return 1;}
 case 0x80000002:if(q->argc!=1)return 0;return nflstreet_controller_confirm(&s->selection,s->handles,q->args[0],&s->committed);
 case 0x80000003:{if(q->argc!=1)return 0;uint32_t dims=0,addr=q->args[0];if(!word(b,n,base,sb,words,addr,&dims,0)||dims>8||addr>UINT32_MAX-(dims+1)*4)return 0;addr+=(dims+1)*4;
  if(!range(n,base,addr,16)&&!(range(1024,sb,addr,16)&&!((addr-sb)&3)))return 0;
  for(unsigned i=0;i<4;++i){uint32_t v=s->selection.sides[i];if(!word(b,n,base,sb,words,addr+i*4,&v,1))return 0;}return 1;}
 case 0x80000004:{if(q->argc!=2)return 0;uint32_t a,bv;/* Validate both destinations before writing either. */
  if(!word(b,n,base,sb,words,q->args[0],&a,0)||!word(b,n,base,sb,words,q->args[1],&bv,0))return 0;a=99;bv=100;return word(b,n,base,sb,words,q->args[0],&a,1)&&word(b,n,base,sb,words,q->args[1],&bv,1);}
 case 0x80000005:{if(q->argc!=3||!range(n,base,q->args[2],12))return 0;uint8_t*d=b+q->args[2]-base;uint32_t cap=be(d+4),addr=be(d+8);if(!range(n,base,addr,(size_t)cap+1)||cap>255)return 0;
  nflstreet_controller_select next=s->selection;char name[20];memset(name,0,20);if(!nflstreet_controller_cycle(&next,q->args[0],(int32_t)q->args[1],name))return 0;
  if(q->args[0]>=4)return 1;size_t length=strlen(name);if(length>cap)return 0;memcpy(b+addr-base,name,length+1);s->selection=next;return 1;}
 case 0x80000006:{if(q->argc!=2)return 0;uint32_t dims=0,addr=q->args[0],existing;if(!word(b,n,base,sb,words,addr,&dims,0)||dims>8||addr>UINT32_MAX-(dims+1)*4)return 0;addr+=(dims+1)*4;
  if((!range(n,base,addr,16)&&!(range(1024,sb,addr,16)&&!((addr-sb)&3)))||!word(b,n,base,sb,words,q->args[1],&existing,0))return 0;
  nflstreet_controller_callback next=*s;uint32_t changed;if(!nflstreet_controller_refresh(&next.selection,&next.devices,0,&changed))return 0;
  if(!word(b,n,base,sb,words,q->args[1],&changed,1))return 0;
  for(unsigned p=0;p<4;++p){next.handles[p]=next.devices.bindings[p];uint32_t connected=next.handles[p]!=UINT32_MAX;if(!word(b,n,base,sb,words,addr+p*4,&connected,1))return 0;}
  *s=next;return 1;}
 case 0x80000007:{if(q->argc!=1)return 0;int32_t value;if(!nflstreet_controller_get(&s->selection,q->args[0],&value))return 0;*out=(uint32_t)value;return 1;}
 case 0x80000009:if(q->argc!=2)return 0;return nflstreet_controller_set(&s->selection,s->next_mapping,q->args[0],q->args[1]);
 case 0x8000000b:if(q->argc!=1)return 0;if(q->args[0]>1)return 1;if(s->devices.count>=NFLSTREET_CONTROLLER_DEVICE_EFFECTS)return 0;
  s->devices.effects[s->devices.count++]=(nflstreet_controller_effect){0x801278b4,2,{UINT32_MAX,q->args[0],0}};return 1;
 case 0x8000000c:if(q->argc)return 0;*out=0;return 1;
 case 0x8000000d:if(q->argc!=2||q->args[0]>=4)return 0;s->selection.sides[q->args[0]]=q->args[1];return 1;
 default:return 0;
 }
}
