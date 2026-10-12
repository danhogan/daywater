#include "team_player_window_callback.h"
#include <stdint.h>
#include <string.h>

static int range(size_t n,uint32_t base,uint32_t address,size_t bytes){
 return address>=base&&(size_t)(address-base)<=n&&bytes<=n-(size_t)(address-base);
}
static int overlap(uint32_t a,size_t an,uint32_t b,size_t bn){
 return a<=b?(uint64_t)b-a<an:(uint64_t)a-b<bn;
}
static int word(uint8_t*b,size_t n,uint32_t base,uint32_t sb,uint32_t*w,
                uint32_t address,uint32_t*value,int write){
 if(address&3u)return 0;
 if(range(n,base,address,4)){uint8_t*p=b+address-base;if(write){p[0]=(uint8_t)(*value>>24);p[1]=(uint8_t)(*value>>16);p[2]=(uint8_t)(*value>>8);p[3]=(uint8_t)*value;}else *value=(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];return 1;}
 if(range(1024,sb,address,4)&&!((address-sb)&3)){if(write){w[(address-sb)/4]=*value;}else *value=w[(address-sb)/4];return 1;}
 return 0;
}
static int separated(uint8_t*b,size_t n,uint32_t base,uint32_t sb,uint32_t*w,
                     uint32_t desc,uint32_t*out){
 uint32_t dims;if(!word(b,n,base,sb,w,desc,&dims,0)||dims>8||desc>UINT32_MAX-(dims+1u)*4u)return 0;
 uint32_t pre=(dims+1u)*4u,at=desc+pre,ignore;
 if(!word(b,n,base,sb,w,desc,&ignore,0))return 0;
 /* Both the descriptor and its two output words must be bounded in RAM or stack. */
 int screen_desc=range(n,base,desc,pre),stack_desc=range(1024,sb,desc,pre);
 int screen_out=range(n,base,at,8),stack_out=range(1024,sb,at,8);
 if(!((screen_desc&&screen_out)||(stack_desc&&stack_out)))return 0;
 uint32_t a,z;if(!word(b,n,base,sb,w,at,&a,0)||!word(b,n,base,sb,w,at+4,&z,0))return 0;
 *out=at;return 1;
}
static int desc_extent(uint8_t*b,size_t n,uint32_t base,uint32_t sb,uint32_t*w,uint32_t d,uint32_t*first,uint32_t*last){
 uint32_t dims;if(!word(b,n,base,sb,w,d,&dims,0)||dims>8||d>UINT32_MAX-(dims+1u)*4u)return 0;
 uint32_t z=(dims+1u)*4u;if(!range(n,base,d,z)&&!range(1024,sb,d,z))return 0;*first=d;*last=d+z;return 1;
}
int nflstreet_team_player_window_callback_memory(void*ctx,uint8_t*b,size_t n,uint32_t base,
 const nflstreet_ui_service_request*q,uint32_t*out,uint32_t sb,uint32_t words[256]){
 nflstreet_team_player_window*s=ctx;if(!s||!b||!q||!out||!words||q->kind!=1||!base||!sb||n>UINT32_MAX-base||sb>UINT32_MAX-1024u||
   (base<sb+1024u&&sb<(uint64_t)base+n)||q->argc>16u)return 0;
 if(q->property!=0x8000000fu&&q->property!=0x80000010u)return 0;
 if((q->property==0x8000000fu&&q->argc!=5)||(q->property==0x80000010u&&q->argc!=4)||q->args[0]>1)return 0;
 unsigned side=q->args[0];nflstreet_team_player_window next=*s;
 uint32_t outputs[3],dfirst[4],dlast[4];unsigned dc=q->property==0x8000000fu?3u:1u;
 for(unsigned i=0;i<dc;++i){uint32_t d=q->property==0x8000000fu?q->args[1+i]:q->args[3];if(!separated(b,n,base,sb,words,d,&outputs[i])||!desc_extent(b,n,base,sb,words,d,&dfirst[i],&dlast[i]))return 0;}
 for(unsigned i=0;i<dc;++i){for(unsigned j=0;j<i;++j)if(overlap(outputs[i],8,outputs[j],8)||overlap(dfirst[i],dlast[i]-dfirst[i],dfirst[j],dlast[j]-dfirst[j])||overlap(outputs[i],8,dfirst[j],dlast[j]-dfirst[j])||overlap(outputs[j],8,dfirst[i],dlast[i]-dfirst[i]))return 0;
  if(overlap(outputs[i],8,dfirst[i],dlast[i]-dfirst[i]))return 0;}
 uint32_t scalar=0;if(q->property==0x8000000fu){uint32_t scalar_old;scalar=q->args[4];if(!word(b,n,base,sb,words,scalar,&scalar_old,0))return 0;
  for(unsigned i=0;i<dc;++i)if(overlap(scalar,4,outputs[i],8)||overlap(scalar,4,dfirst[i],dlast[i]-dfirst[i]))return 0;}
 if(q->property==0x8000000fu){nflstreet_team_player_window_pair pairs[3];uint32_t mode;if(!nflstreet_team_player_window_callback_f(&next,side,pairs,&mode))return 0;
  for(unsigned i=0;i<3;++i){uint32_t a=pairs[i].id,z=pairs[i].slot;if(!word(b,n,base,sb,words,outputs[i],&a,1)||!word(b,n,base,sb,words,outputs[i]+4,&z,1))return 0;}
  if(!word(b,n,base,sb,words,scalar,&mode,1))return 0;
 }else{nflstreet_team_player_window_pair pair;if(!nflstreet_team_player_window_callback_10(&next,side,(int32_t)q->args[2],&pair))return 0;uint32_t a=pair.id,z=pair.slot;if(!word(b,n,base,sb,words,outputs[0],&a,1)||!word(b,n,base,sb,words,outputs[0]+4,&z,1))return 0;}
 *s=next;return 1;
}
