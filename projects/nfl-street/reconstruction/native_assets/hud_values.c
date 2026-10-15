#include "hud_values.h"
#include "gekko_float.h"
#include <stddef.h>
#include <string.h>
static uint8_t *at(nflstreet_hud_values *c,uint32_t a,uint32_t n){
 uint8_t *p=c->span?c->span(c->memory,a,n):NULL;if(!p&&!c->rejected_address)c->rejected_address=a;return p;}
static uint32_t word(nflstreet_hud_values *c,uint32_t a){const uint8_t *p=at(c,a,4);return p?(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]:0;}
static unsigned half(nflstreet_hud_values *c,uint32_t a){const uint8_t *p=at(c,a,2);return p?(unsigned)p[0]<<8|p[1]:0;}
static unsigned byte(nflstreet_hud_values *c,uint32_t a){const uint8_t *p=at(c,a,1);return p?*p:0;}
static float real(nflstreet_hud_values *c,uint32_t a){uint32_t b=word(c,a);float f;memcpy(&f,&b,4);return f;}
static double wide(nflstreet_hud_values *c,uint32_t a){return nflstreet_gk_value((uint64_t)word(c,a)<<32|word(c,a+4));}
static void put(nflstreet_hud_values *c,uint32_t a,uint32_t v){uint8_t *p=at(c,a,4);if(p){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}}
static void putf(nflstreet_hud_values *c,uint32_t a,float f){uint32_t b;memcpy(&b,&f,4);put(c,a,b);}
static int reject(nflstreet_hud_values *c,uint32_t routine){if(!c->rejected_address)c->rejected_address=routine;return 0;}
static uint32_t actor(nflstreet_hud_values *c,uint32_t reference){
 if((reference&255u)!=1)return 0;uint32_t roster=word(c,c->r13-0x7c30u);if(!roster)return 0;
 return word(c,roster)+(((reference>>8)&255u)*half(c,roster+8)+((reference>>16)&255u))*0x14d4u;}
/* Only the neutral 8003DCAC path is supported. 8007F878 copies 0x6c input
   bytes (or the disabled-input record); a bytewise zero comparison returns
   before 8002E4F8, 800F8828 and 8003D208 can change gameplay state. */
static int can_recharge(nflstreet_hud_values *c,uint32_t a,int *yes){
 *yes=0;if(!a)return !c->rejected_address;
 const unsigned controller=byte(c,a+8);
 if(controller!=255&&(word(c,a+12)&0x4000u)){
  uint32_t manager=word(c,c->r13-0x7858u),input=word(c,manager+4)+controller*0x78u;
  uint32_t source=byte(c,input+0x74)?0x8026e68cu:input+8;
  const uint8_t *p=at(c,source,0x6c);if(!p)return 0;
  for(unsigned j=0;j<0x6c;++j)if(p[j])return reject(c,0x8003dcacu);}
 const uint32_t group=word(c,a+0x318);unsigned key=65535;
 for(unsigned i=4;i;--i){uint32_t channel=group+(i-1)*100u;if(half(c,channel+6)==3)key=half(c,channel+4);}
 switch(key){
 case 0x2c:case 0x30:case 0x43:case 0x46:case 0x54:case 0xa5:case 0xc1:
 case 0xc3:case 0xc4:case 0xc5:case 0xcf:case 0xd0:case 0xd1:case 0xd2:*yes=1;break;
 case 0x4a:{unsigned status=byte(c,a+0x21f);*yes=status!=3&&status!=9&&status!=11;break;}
 default:break;}
 return !c->rejected_address;}
int nflstreet_hud_turbo_update(nflstreet_hud_values *c){
 const uint32_t turbo=word(c,c->r13-0x79fcu);int recharge[4]={0};
 /* Preflight all records before changing RAM; the original's valid-path order
    of stores is retained below. Guard rejection is a native boundary. */
 for(unsigned i=0;i<4;++i){uint32_t r=turbo+24u*i;if(!at(c,r+4,24))return 0;
  const uint32_t a=actor(c,word(c,r+4));if(!byte(c,r+0x14)&&!can_recharge(c,a,&recharge[i]))return 0;}
 if(c->rejected_address)return 0;
 for(unsigned i=0;i<4;++i){uint32_t r=turbo+24u*i;unsigned wait=half(c,r+0x16);
  if(wait){uint8_t *p=at(c,r+0x16,2);--wait;p[0]=(uint8_t)(wait>>8);p[1]=(uint8_t)wait;}
  if(recharge[i]){uint32_t value=word(c,r+8)+word(c,r+0x10),maximum=word(c,r+0x18);
   if(value>maximum)value=maximum;put(c,r+8,value);}
  uint8_t *p=at(c,r+0x14,2);p[1]=p[0];p[0]=0;}
 return !c->rejected_address;}
int nflstreet_hud_turbo_fraction(nflstreet_hud_values *c,uint32_t slot,float *out){
 if(slot>=4||!out)return 0;uint32_t r=word(c,c->r13-0x79fcu)+24u*slot,value=word(c,r+8);
 if(!value)*out=real(c,0x8024f850u);
 else{double bias=wide(c,0x8024f858u);float n=nflstreet_gk_convert(value,bias),d=nflstreet_gk_convert(word(c,r+0x18),bias);
  *out=nflstreet_gk_divs(n,d);}
 return !c->rejected_address;}
int nflstreet_hud_gamebreaker_fraction(nflstreet_hud_values *c,uint32_t side,float *out){
 if(!out)return 0;uint32_t s=word(c,c->r13-0x7a18u),value=word(c,s+(side?12u:8u));
 if(!value)*out=real(c,0x8024f7f0u);
 else if(value==10000)*out=real(c,0x8024f7f4u);
 else{double bias=wide(c,0x8024f7f8u);float n=nflstreet_gk_convert(value^0x80000000u,bias);
  float d=nflstreet_gk_convert(word(c,c->r13-0x4e50u)^0x80000000u,bias);*out=nflstreet_gk_divs(n,d);}
 return !c->rejected_address;}
int nflstreet_hud_turbo_publish(nflstreet_hud_values *c,uint32_t payload){
 if(!at(c,payload,16))return 0;for(unsigned i=0;i<4;++i){float value;if(!nflstreet_hud_turbo_fraction(c,i,&value))return 0;
  putf(c,payload+(i<2?8u+4u*i:4u*i-8u),value);}
 return !c->rejected_address;}
int nflstreet_hud_timer_cleanup(nflstreet_hud_values *c,uint32_t screen){
 uint32_t count=word(c,screen+0x54),timers=word(c,screen+0x58);if(count>4096)return reject(c,0x801d15b0u);
 if(count&&!at(c,timers,count*0x34u))return 0;
 for(uint32_t i=count;i;){--i;uint32_t t=timers+i*0x34u,state=word(c,t+0x14);
  if(state==1){--count;put(c,screen+0x54,count);if(i<count){uint8_t *p=at(c,t,(count-i+1)*0x34u);memmove(p,p+0x34,(count-i)*0x34u);}}
  else if(!state)put(c,t+0x14,2);}
 return !c->rejected_address;}
