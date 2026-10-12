#include "controller_devices.h"
static int emit(nflstreet_controller_devices*d,uint32_t fn,uint32_t argc,uint32_t a,uint32_t b){
 if(d->count>=NFLSTREET_CONTROLLER_DEVICE_EFFECTS)return 0;d->effects[d->count++]=(nflstreet_controller_effect){fn,argc,{a,b,0}};return 1;
}
int nflstreet_controller_refresh(nflstreet_controller_select*s,nflstreet_controller_devices*d,uint32_t force,uint32_t*out){
 if(!s||!d||!out||d->count>NFLSTREET_CONTROLLER_DEVICE_EFFECTS||(d->active_port!=UINT32_MAX&&d->active_port>=4))return 0;
 int32_t ignored;if(!nflstreet_controller_get(s,4,&ignored))return 0;
 nflstreet_controller_select next=*s;nflstreet_controller_devices dev=*d;uint32_t changed=0;
 if(!emit(&dev,0x80199e58,0,0,0))return 0;
 for(unsigned i=0;i<8;++i){if(dev.connected[i]>1||!emit(&dev,0x8018013c,2,i,0))return 0;uint8_t connected=dev.status[i]==2;changed|=dev.connected[i]!=connected;dev.connected[i]=connected;}
 for(unsigned i=0;i<10;++i){dev.bindings[i]=UINT32_MAX;if(!emit(&dev,0x80129aa4,2,i,UINT32_MAX))return 0;}
 for(unsigned i=0;i<4;++i)if(dev.connected[i]){dev.bindings[i]=i;if(!emit(&dev,0x80129aa4,2,i,i))return 0;}
 changed=changed||force;
 if(changed){uint8_t used[4]={0};for(unsigned p=0;p<4;++p){if(!dev.connected[p])next.slots[p]=4;if(next.slots[p]<4)used[next.slots[p]]=1;}
  unsigned cursor=0;for(unsigned p=0;p<4;++p)if(dev.bindings[p]!=UINT32_MAX&&dev.connected[dev.bindings[p]]&&next.slots[p]>3){
   for(;;){if(cursor>=4)return 0;if(next.mapping[cursor]==-1){next.slots[p]=4;break;}next.slots[p]=cursor++;if(!used[next.slots[p]])break;}
  }
  for(unsigned p=0;p<4;++p)next.sides[p]=UINT32_MAX;
  if(force&&dev.active_port!=UINT32_MAX)next.sides[dev.active_port]=1;
 }
 *s=next;*d=dev;*out=changed;return 1;
}
int nflstreet_controller_initialize(nflstreet_controller_select*s,nflstreet_controller_devices*d,const int8_t map[4],uint32_t*choice){
 if(!s||!d||!choice)return 0;nflstreet_controller_select next=*s;nflstreet_controller_devices dev=*d;uint32_t result=*choice;
 if(!nflstreet_controller_remap(&next,map))return 0;
 if(dev.query_mode==1){uint32_t changed;if(!nflstreet_controller_refresh(&next,&dev,1,&changed))return 0;result=1;}
 if(!emit(&dev,0x80127910,1,0,0))return 0;if(dev.profile_mode!=6&&!emit(&dev,0x80127910,1,1,0))return 0;
 *s=next;*d=dev;*choice=result;return 1;
}
