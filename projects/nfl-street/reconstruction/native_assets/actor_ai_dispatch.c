#include "actor_ai_dispatch.h"
#include "gekko_float.h"
#include "gekko_quaternion/quaternion_kernels.h"
typedef nflstreet_ai_dispatch C;
static uint8_t *mem(C *c,uint32_t a,uint32_t n){uint8_t *p=c->ram.span(c->ram.memory,a,n);if(!p)c->ok=0;return p;}
static uint32_t r32(C *c,uint32_t a){const uint8_t *p=mem(c,a,4);return p?(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]:0;}
static unsigned r16(C *c,uint32_t a){const uint8_t *p=mem(c,a,2);return p?(unsigned)p[0]<<8|p[1]:0;}
static unsigned r8(C *c,uint32_t a){const uint8_t *p=mem(c,a,1);return p?*p:0;}
static float rf(C *c,uint32_t a){uint32_t w=r32(c,a);float f;memcpy(&f,&w,4);if(!isfinite(f))c->ok=0;return f;}
static void w32(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,4);if(p){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}}
static void w8(C *c,uint32_t a,unsigned v){uint8_t *p=mem(c,a,1);if(p)*p=(uint8_t)v;}
static void wf(C *c,uint32_t a,float f){uint32_t w;memcpy(&w,&f,4);if(!isfinite(f))c->ok=0;w32(c,a,w);}
static int begin(C *c){if(!c||!c->ram.span)return 0;c->ok=1;c->unsupported=0;return 1;}
static uint32_t service(C *c,uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t e,uint32_t f){
 uint32_t result=0;nflstreet_ai_call call={pc,{a,b,d,e,f,0}};
 if(c->ok&&(!c->service||!c->service(c->user,&call,&result))){c->ok=0;c->unsupported=pc;}return result;}
static uint32_t descriptor(C *c,uint32_t i){return r32(c,r32(c,c->ram.r13-0x7860u)+4*i);}
static unsigned count(C *c,uint32_t i){return r16(c,descriptor(c,i)+2);}
static uint32_t callback(C *c,uint32_t i,uint32_t q,unsigned slot){return r32(c,r32(c,descriptor(c,i)+4)+24*r8(c,q)+4*slot);}
static int enter(C *,uint32_t,uint32_t,uint32_t,unsigned);
static int replace(C *,uint32_t,uint32_t,uint32_t,uint32_t,unsigned);
static int follow(C *,uint32_t,uint32_t *);
/* The leaf body at 8008D5BC only calls 80021E18, regardless of +150. */
static void clear_interaction(C *c,uint32_t a){if(r32(c,a+0x124)){service(c,0x80021afcu,a,a+0x10,0xf,0,0);w32(c,a+0x124,0);}}
static void notify(C *c,uint32_t a){ /* 8005BF88 */
 const uint32_t p=r32(c,c->ram.r13-0x7a18u);
 if(a&&r8(c,p+0x5d9)&&a==r32(c,p+0x660)){
  service(c,0x80056348u,0xe,0,0,0,0);service(c,0x800595bcu,0,0,0,0,0);}}
static uint32_t invoke(C *c,uint32_t pc,uint32_t a){uint32_t result=0;if(!c->ok)return 0;
 switch(pc){
 case 0x8007ea10u:case 0x8007ea20u:return 0;
 case 0x8007ea18u:return 1;
 case 0x8008d954u:if(!nflstreet_actor_ai_enter(&c->ram,a))c->ok=0;return 0;
 case 0x8008dac8u:clear_interaction(c,a);return 1;
 case 0x8008d988u:follow(c,a,&result);return result;
 default:return service(c,pc,a,0,0,0,0);
 }}
static int pop(C *c,uint32_t i,uint32_t q){ /* 8007DD80: memmove, clears only the last TYPE byte */
 const unsigned n=count(c,i);if(!n||n>256){c->ok=0;return 0;}
 uint8_t *p=mem(c,q,4*n);if(p){memmove(p,p+4,4*(n-1));p[4*n-4]=0;}return c->ok;}
static int remove_type(C *c,uint32_t i,uint32_t q,unsigned type){ /* 8007E5A4 */
 const unsigned n=count(c,i);if(n>256){c->ok=0;return 0;}
 for(unsigned k=0;k<n&&c->ok;++k)if(r8(c,q+4*k)==type){
  uint8_t *p=mem(c,q+4*k,4*(n-k));if(p){memmove(p,p+4,4*(n-k-1));w8(c,q+4*n-4,0);}break;}
 return c->ok;}
static int enter(C *c,uint32_t i,uint32_t q,uint32_t a,unsigned depth){ /* 8007DDE8 */
 if(depth>256){c->ok=0;return 0;}w8(c,a+0xc0c,0);
 const uint32_t result=invoke(c,callback(c,i,q,0),a);
 if(result==1&&!r8(c,a+0xc0c)&&c->ok){pop(c,i,q);enter(c,i,q,a,depth+1);}
 w8(c,a+0xc0c,1);notify(c,a);return c->ok;}
static int enqueue(C *c,uint32_t i,uint32_t q,uint32_t command,uint32_t a,unsigned depth){ /* 8007E2E0 */
 if(!r8(c,q))return replace(c,i,q,command,a,depth+1);
 const unsigned n=count(c,i);if(n<2||n>256){c->ok=0;return 0;}
 unsigned k=1;while(k<n-2&&r8(c,q+4*k))++k;
 uint8_t *p=mem(c,q+4,4*(k+1));if(p){memmove(p+4,p,4*k);w32(c,q+4,command);}return c->ok;}
static int replace(C *c,uint32_t i,uint32_t q,uint32_t command,uint32_t a,unsigned depth){ /* 8007E650 */
 if(depth>256){c->ok=0;return 0;}
 const uint32_t result=invoke(c,callback(c,i,q,1),a);
 if(result==1&&c->ok){w8(c,q+4,0);w32(c,q,command);enter(c,i,q,a,depth+1);}
 else if(c->ok){enqueue(c,i,q,command,a,depth+1);w8(c,q+8,0);}return c->ok;}
int nflstreet_ai_dispatch_step(C *c,uint32_t i,uint32_t q,uint32_t a,int controlled){
 if(!begin(c))return 0;const unsigned old=r8(c,q);int entered=0;
 if(controlled){w8(c,a+0xc0c,0);const uint32_t result=invoke(c,callback(c,i,q,3),a);
  if(result==1&&c->ok){if(!r8(c,a+0xc0c)){pop(c,i,q);entered=1;enter(c,i,q,a,0);}else remove_type(c,i,q,old);}}
 if(!entered&&c->ok){w8(c,a+0xc0c,0);const uint32_t result=invoke(c,callback(c,i,q,2),a);
  if(result==1&&c->ok){if(!r8(c,a+0xc0c)){pop(c,i,q);enter(c,i,q,a,0);}else remove_type(c,i,q,old);}}
 return c->ok;}
int nflstreet_ai_dispatch_enter(C *c,uint32_t i,uint32_t q,uint32_t a){return begin(c)&&enter(c,i,q,a,0);}
int nflstreet_ai_dispatch_replace(C *c,uint32_t i,uint32_t q,uint32_t command,uint32_t a){return begin(c)&&replace(c,i,q,command,a,0);}
static uint32_t angle(C *c,float y,float x){ /* 80188D98, same instruction order as actor_ai.c */
 float k[9];const float zero=rf(c,0x802617d0u),scale=rf(c,0x802617d4u);
 for(unsigned i=0;i<9;++i)k[i]=rf(c,0x8029194cu+4*i);
 const float ay=fabsf(y),ax=fabsf(x);float q=nflstreet_gk_divs(nflstreet_gk_subs(ay,ax),nflstreet_gk_adds(ay,ax));uint32_t quadrant=0;
 if(y>=zero){if(x<zero){quadrant=0x400000u;q=-q;}}else{quadrant=0x800000u;if(x>=zero){quadrant=0xffc00000u;q=-q;}}
 const float q2=nflstreet_gk_muls(q,q);float power=nflstreet_gk_muls(q,q2);
 float p=nflstreet_gk_fmadds(q,k[0],nflstreet_gk_muls(power,k[1]));power=nflstreet_gk_muls(power,q2);
 for(unsigned i=2;i<8;++i){p=nflstreet_gk_fmadds(power,k[i],p);if(i!=7)power=nflstreet_gk_muls(power,q2);}
 p=nflstreet_gk_muls(nflstreet_gk_adds(p,k[8]),scale);return ((uint32_t)nflstreet_gk_fctiwz(p)+quadrant)&0xffffffu;}
static uint32_t angular_distance(uint32_t a,uint32_t b){const uint32_t d=(a-b)&0xffffffu;return d<0x1000000u-d?d:0x1000000u-d;}
static int face(C *c,uint32_t a,uint32_t h,float weight,uint32_t mode,unsigned slot){ /* 8005335C */
 if(slot>1||!isfinite(weight)){c->ok=0;return 0;}
 const uint32_t raw=(h-r32(c,a+0x1c0))&0xffffffu;int32_t d=(int32_t)(raw<<8)>>8;
 if(d<-0x155555)d=-0x155555;else if(d>0x155555)d=0x155555;
 const int32_t scale=nflstreet_gk_fctiwz(nflstreet_gk_muls(weight,rf(c,0x80247188u)));
 const uint64_t product=(uint64_t)((int64_t)d*scale);
 w32(c,a+0x13c+4*slot,(uint32_t)(product>>24));w32(c,a+0x144+4*slot,mode);return c->ok;}
int nflstreet_ai_face(C *c,uint32_t a,uint32_t h,float w,uint32_t m,unsigned s){return begin(c)&&face(c,a,h,w,m,s);}
static uint32_t toward(C *c,uint32_t a,uint32_t b){
 const float x=nflstreet_gk_subs(rf(c,b+0x1a8),rf(c,a+0x1a8)),y=nflstreet_gk_subs(rf(c,b+0x1ac),rf(c,a+0x1ac));return angle(c,y,x);}
static int interact(C *c,uint32_t a,uint32_t b,uint32_t *result){ /* 80021EF0, 80021CB8 */
 const int had=r32(c,a+0x124)!=0;
 const int cone=angular_distance(toward(c,a,b),r32(c,a+0x1c0))<0x471c72u;
 if(!had&&cone){const uint32_t token=r32(c,b);
  service(c,0x80021b58u,a,token,0x1a,0xf,1);w32(c,a+0x128,300);w32(c,a+0x124,token);
 }else if(had&&!cone)clear_interaction(c,a);
 *result=r32(c,a+0x124)==0;return c->ok;}
int nflstreet_ai_interact(C *c,uint32_t a,uint32_t b,uint32_t *result){return result&&begin(c)&&interact(c,a,b,result);}
static int eligible(C *c,uint32_t a,uint32_t b,uint32_t *result){ /* 8008D654 */
 const uint32_t q=r32(c,b+0x310);const unsigned type=r8(c,q);*result=1;
 if(type==0x12){*result=0;return c->ok;}
 if(type==1){const float x=nflstreet_gk_subs(rf(c,a+0x1a8),rf(c,b+0x1a8)),y=nflstreet_gk_subs(rf(c,a+0x1ac),rf(c,b+0x1ac));
  const float square=nflstreet_gk_fmadds(x,x,nflstreet_gk_muls(y,y));if(!isfinite(square))c->ok=0;
  if(nflstreet_802165c8_sqrt(square)<=rf(c,0x80250a64u)&&rf(c,a+0x1ac)<=rf(c,b+0x1ac))return c->ok;
 }else if(type==0x19&&r8(c,q+2)==9){
  if(r8(c,q+1)==r8(c,a+1)){service(c,0x8009e590u,a,9,0,0,0);*result=0;return c->ok;}
 }else return c->ok;
 replace(c,0,r32(c,a+0x310),0x21000000u,a,0);*result=0;return c->ok;}
int nflstreet_ai_eligible(C *c,uint32_t a,uint32_t b,uint32_t *result){return result&&begin(c)&&eligible(c,a,b,result);}
static uint32_t selected(C *c){ /* 800BE3F0 -> 800BDDC4 / 800BE380 / 80020944 / 8003A4A0 */
 const uint32_t root=r32(c,c->ram.r13-0x73d0u),token=r32(c,root+8+0xf4*r8(c,root+1)+0xb4);
 if((token&255)!=1)return 0;const uint32_t roster=r32(c,c->ram.r13-0x7c30u);
 return roster?r32(c,roster)+(((token>>8)&255)*r16(c,roster+8)+((token>>16)&255))*0x14d4u:0;}
static int follow(C *c,uint32_t a,uint32_t *result){ /* 8008D988 */
 const uint32_t b=selected(c);
 /* 8009AD80: the common phase !=4,7 rejection is native, including
    8002E4F8's observable cached-word write. Remaining play policy is a service. */
 const uint32_t phase=r32(c,r32(c,c->ram.r13-0x7cf8u));w32(c,c->ram.r13-0x7cf4u,phase);
 if((phase==4||phase==7)&&service(c,0x8009ad80u,a,0,0xffffffffu,0,0))w8(c,a+0x150,1);
 if(r8(c,a+0xbe0)==0x39&&(int32_t)r32(c,a+0xbdc)>30)w32(c,a+0xbdc,30);
 *result=0;if(!b){
  w8(c,a+0x20e,1);w32(c,a+0x204,r32(c,a+0x1c8));w32(c,a+0x208,r32(c,a+0x1c8));
  wf(c,a+0x200,nflstreet_gk_divs(rf(c,a+0x1c4),rf(c,c->ram.r13-0x70c4u)));
  w8(c,a+0x20f,((r32(c,a+0x1c8)-0x400000u)&0xffffffu)>0x800000u?0x11:0x12);return c->ok;
 }
 if(a==b){replace(c,0,r32(c,a+0x310),0x01000000u,a,0);*result=1;return c->ok;}
 uint32_t allowed=0;eligible(c,a,b,&allowed);if(!allowed){*result=1;return c->ok;}
 if(!(r32(c,a+0xc)&0x4000u)&&c->ok){
  if(!nflstreet_actor_ai_target(&c->ram,a,b,a+0x154,c->scratch)||!nflstreet_actor_ai_project(&c->ram,a,c->scratch))c->ok=0;
  face(c,a,toward(c,a,b),rf(c,0x80250a50u),1,0);interact(c,a,b,&allowed);
 }return c->ok;}
int nflstreet_ai_follow(C *c,uint32_t a,uint32_t *result){return result&&begin(c)&&follow(c,a,result);}
int nflstreet_ai_commit(nflstreet_locomotion_context *c,uint32_t a){return nflstreet_locomotion_select(c,a);}
/* 800F8214: final requested/active input lifecycle before movement. Contact
   admission (8005DCF8) and reached avoidance policy (800F8088) are services;
   all byte/timer/strength dispatch below is native. No universal 0.72 cap. */
int nflstreet_ai_input_finish(C *c,uint32_t a){
 if(!begin(c))return 0;const float zero=rf(c,0x8025456cu);float requested=rf(c,a+0x200);
 if(!(requested>=zero))requested=zero;wf(c,a+0x200,requested);w8(c,a+0x21f,0);
 service(c,0x8005dcf8u,a,0,0,0,0);if(!c->ok)return 0;
 unsigned code=r8(c,a+0x20f),timer=r16(c,a+0x21c);
 if(!code||timer==r16(c,c->ram.r13-0x70a8u)){
  if(!(r32(c,a+0xc)&0x4000u))timer=0;
  else if(timer){if(angular_distance(r32(c,a+0x1c8),r32(c,a+0x204))>0x11c71bu)timer=0;
   else{timer=(timer-1)&65535u;w8(c,a+0x20f,1);}}
 }else timer=0;
 uint8_t *timer_bytes=mem(c,a+0x21c,2);if(timer_bytes){timer_bytes[0]=timer>>8;timer_bytes[1]=timer;}
 code=r8(c,a+0x20f);float strength=rf(c,a+0x210);uint32_t limit=0x8025458cu;
 int avoid=0;
 switch(code){
 case 0:case 12:w8(c,a+0x21f,code);avoid=code==0;break;
 case 1:case 7:case 13:{
  w8(c,a+0x21f,code);const float first=rf(c,0x80254580u);if(first<strength)strength=first;
  if(a==selected(c)){const float second=rf(c,0x80254574u);if(second<strength)strength=second;avoid=code==1;}
  limit=0;break;}
 case 2:case 3:case 8:case 9:case 10:case 11:
  w8(c,a+0x21f,code);limit=code==2||code==8||code==10?0x80254578u:0x8025457cu;
  if((code==8||code==9)&&rf(c,a+0x1c4)!=zero){w8(c,a+0x309,1);w8(c,a+0x308,1);}
  if(code==10||code==11){w8(c,a+0x309,2);w8(c,a+0x308,2);}break;
 case 4:case 5:case 6:case 17:case 18:case 20:case 21:case 24:w8(c,a+0x21f,code);break;
 case 14:case 15:case 16:w8(c,a+0x21f,code);limit=0x80254584u;break;
 case 22:w8(c,a+0x21f,code);limit=0x80254578u;break;
 case 23:w8(c,a+0x21f,code);limit=0x8025457cu;break;
 case 25:w8(c,a+0x21f,code);limit=0x80254574u;break;
 case 26:w8(c,a+0x21f,code);limit=0x80254588u;break;
 case 27:case 29:w8(c,a+0x21f,code);limit=0x80254580u;break;
 case 28:w8(c,a+0x21f,code);limit=0x80254590u;break;
 case 30:w8(c,a+0x21f,code);strength=rf(c,a+0x200);limit=0x80254570u;break;
 case 31:w8(c,a+0x21f,code);limit=0;break;
 default:break;
 }
 if(limit){const float ceiling=rf(c,limit);if(ceiling<strength)strength=ceiling;}
 /* 800F8654/8658: every cap tail publishes BOTH strengths, including
    default/code-4 paths and code 30's requested-strength source. */
 wf(c,a+0x210,strength);wf(c,a+0x200,strength);
 /* 800F8088's first two rejection gates are side-effect free. The reached
    neighborhood/contact policy is supplied rather than guessed. */
 if(avoid&&angular_distance(0x400000u,r32(c,a+0x1c0))<=0x355554u&&a==selected(c))
  service(c,0x800f8088u,a,0,0,0,0);
 w8(c,a+0x20f,0);return c->ok;
}
