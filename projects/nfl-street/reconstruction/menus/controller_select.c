#include "controller_select.h"
#include <string.h>
static int valid(const nflstreet_controller_select*s){
 if(!s||s->profile_count>4)return 0;
 for(unsigned i=0;i<4;++i)if(s->slots[i]>6||s->mapping[i]<-1||s->mapping[i]>3||!memchr(s->profiles[i],0,20))return 0;
 for(unsigned i=0;i<3;++i)if(!memchr(s->special[i],0,20))return 0;
 return 1;
}
int nflstreet_controller_remap(nflstreet_controller_select*s,const int8_t map[4]){
 if(!valid(s)||!map)return 0;for(unsigned i=0;i<4;++i)if(map[i]<-1||map[i]>3)return 0;
 nflstreet_controller_select next=*s;memcpy(next.mapping,map,4);
 for(unsigned p=0;p<4;++p)if(next.slots[p]<4){unsigned j;for(j=0;j<4;++j)if(s->mapping[s->slots[p]]==map[j])break;
  next.slots[p]=j;if(j<4&&map[j]==-1)next.slots[p]=4;
 }
 *s=next;return 1;
}
int nflstreet_controller_set(nflstreet_controller_select*s,const int8_t map[4],uint32_t port,uint32_t value){
 if(!nflstreet_controller_remap(s,map))return 0;if(value>6)value=4;
 if(port<4){if(value<4){for(unsigned i=0;i<4;++i)if(s->mapping[i]==(int32_t)value){s->slots[port]=i;break;}}
  else s->slots[port]=value;
 }return 1;
}
int nflstreet_controller_get(const nflstreet_controller_select*s,uint32_t port,int32_t*out){
 if(!valid(s)||!out)return 0;*out=4;if(port<4){unsigned slot=s->slots[port];*out=slot<4?s->mapping[slot]:(int32_t)slot;}return 1;
}
int nflstreet_controller_cycle(nflstreet_controller_select*s,uint32_t port,int32_t dir,char name[20]){
 if(!valid(s)||!name)return 0;if(port>=4)return 1;
 nflstreet_controller_select next=*s;
 for(unsigned attempt=0;attempt<32;++attempt){
  unsigned *slot=&next.slots[port];if(dir>0)*slot=*slot==6?0:*slot+1;else if(dir<0)*slot=*slot==0?6:*slot-1;
  if(*slot>=4){memcpy(name,next.special[*slot-4],strlen(next.special[*slot-4])+1);*s=next;return 1;}
  int retry=0;
  for(unsigned p=0;p<4;++p)if(p!=port&&next.slots[p]==*slot){
   uint32_t side=next.sides[p];
   if(side!=next.sides[port]&&side!=UINT32_MAX){
    unsigned earlier;for(earlier=0;earlier<4;++earlier)if(earlier!=p&&next.sides[earlier]==side)break;
    if(earlier<p)next.slots[p]=4;
    if(next.slots[p]!=4){if(!dir)*slot=4;retry=1;break;}
    break; /* Original BACC exits duplicate scan even after clearing it. */
   }else{
    if(p<=port&&side!=UINT32_MAX){retry=1;break;}
    next.slots[p]=4;
   }
  }
  if(retry)continue;
  if(*slot<4&&next.mapping[*slot]!=-1&&next.mapping[*slot]<(int32_t)next.profile_count){
   memcpy(name,next.profiles[(unsigned)next.mapping[*slot]],strlen(next.profiles[(unsigned)next.mapping[*slot]])+1);*s=next;return 1;
  }
 }
 return 0;
}
static void effect(nflstreet_controller_commit*out,uint32_t function,uint32_t argc,uint32_t a,uint32_t b,uint32_t c){
 nflstreet_controller_effect*q=&out->effects[out->count++];q->function=function;q->argc=argc;q->args[0]=a;q->args[1]=b;q->args[2]=c;
}
int nflstreet_controller_confirm(const nflstreet_controller_select*s,const uint32_t handles[4],uint32_t choice,nflstreet_controller_commit*out){
 if(!valid(s)||!handles||!out)return 0;nflstreet_controller_commit next={0};next.choice=choice;
 effect(&next,0x801e02fc,0,0,0,0);
 for(unsigned p=0;p<4;++p)if(handles[p]!=UINT32_MAX&&s->sides[p]<2)effect(&next,0x801e02b4,2,s->sides[p],handles[p],0);
 effect(&next,0x8014c20c,2,2,choice,0);effect(&next,0x8013da04,3,0xab,0x32,0);*out=next;return 1;
}
