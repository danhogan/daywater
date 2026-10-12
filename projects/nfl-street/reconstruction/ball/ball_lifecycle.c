#include "ball_lifecycle.h"
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int nflstreet_ball_transition(uint8_t*r,size_t n,uint32_t state,uint32_t arg,uint32_t elapsed){
 if(!r||n<0xf4||state>3)return 0;
 uint32_t old=be(r+0xa4);
 if(old>9||old==4||old==7)return 0;
 uint8_t next[0xf4];memcpy(next,r,sizeof next);
 put(next+0xa4,state);put(next+0xb0,be(next+0xa8));put(next+0xa8,arg);put(next+0xac,old);
 /* Exit runs after state/history writes and before entry. */
 if(old==1){put(next+0xc0,elapsed);put(next+0xcc,be(next+0xb4));put(next+0xbc,0);}
 else if(old==6||old==8){put(next+0xe0,0);put(next+8,be(next+8)&~8u);}
 if(state==1){put(next+0xbc,0);put(next+0xc4,1);put(next+8,be(next+8)&~0x28u);}
 memcpy(r,next,sizeof next);return 1;
}
int nflstreet_ball_release(uint8_t*r,size_t n,uint32_t state,uint32_t arg,uint32_t elapsed,unsigned clear){
 if(clear>1||!nflstreet_ball_transition(r,n,state,arg,elapsed))return 0;
 if(clear)memset(r+0x54,0,12);
 memset(r+0x7c,0,12);put(r+0xb4,0);return 1;
}
