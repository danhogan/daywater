#include "locomotion_predicates.h"
#include "gekko_float.h"
#define R13 0x803a9a40u
typedef nflstreet_locomotion_context C;
static uint8_t *mem(C *c,uint32_t a,uint32_t n){uint8_t *p=a?c->span(c->memory,a,n):NULL;if(!p)c->ok=0;return p;}
static uint32_t r32(C *c,uint32_t a){const uint8_t *p=mem(c,a,4);return p?nflstreet_rd32(p):0;}
static uint16_t r16(C *c,uint32_t a){const uint8_t *p=mem(c,a,2);return p?nflstreet_rd16(p):0;}
static uint8_t r8(C *c,uint32_t a){const uint8_t *p=mem(c,a,1);return p?*p:0;}
static float rf(C *c,uint32_t a){return nflstreet_as_float(r32(c,a));}
static double rd(C *c,uint32_t a){return nflstreet_gk_value((uint64_t)r32(c,a)<<32|r32(c,a+4));}
static void w32(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,4);if(p)nflstreet_wr32(p,v);}
static void wf(C *c,uint32_t a,float f){w32(c,a,nflstreet_float_bits(f));}
static void w8(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,1);if(p)*p=(uint8_t)v;}
static int begin(C *c){if(!c||!c->span)return 0;c->ok=1;c->unsupported=0;return 1;}
static uint32_t distance(uint32_t a,uint32_t b){ /* 80188F28 */
 const uint32_t d=(a-b)&0xffffffu;return d<0x1000000u-d?d:0x1000000u-d;}
static uint32_t current(C *c,uint32_t m){ /* 801784B8 */
 for(unsigned i=0;i<4&&c->ok;++i)if(r16(c,m+6+100*i)==3)return r16(c,m+4+100*i);return 0xffff;}
static uint32_t record(C *c,uint32_t m,uint32_t key){ /* 8017847C returns record+10 */
 for(unsigned i=0;i<4&&c->ok;++i)if(r16(c,m+6+100*i)&&r16(c,m+4+100*i)==key)return m+100*i;return 0;}
static uint32_t side(C *c){const uint32_t w=r32(c,R13-0x7078u);return w?r8(c,w+0x38):0;} /* 80103C88 */
static uint32_t own(C *c){const uint32_t w=r32(c,R13-0x7078u);return w?(r8(c,w+0x38)^1u):0;} /* 80103CA0 */
static uint32_t actor_at(C *c,uint32_t team,uint32_t slot){ /* 8003A4A0 */
 const uint32_t t=r32(c,R13-0x7c30u);return t?r32(c,t)+(team*r16(c,t+8)+slot)*0x14d4u:0;}
static uint32_t team_state(C *c){return r32(c,R13-0x5a98u)+side(c)*0x7878u+0xd1cu;} /* 800F2E94 */
static int phase(C *c){ /* 800B78A0 / 800EF6C4 and 800B78C8 */
 const uint32_t t=team_state(c),v=r32(c,t+0x14);return (v<6&&v!=0)||r8(c,t+0x17)==14;}
static uint32_t state_word(C *c){ /* 8002E4F8 has an observable cached-word write */
 const uint32_t v=r32(c,r32(c,R13-0x7cf8u));w32(c,R13-0x7cf4u,v);return v;}
static uint32_t mode_bits(C *c){return r8(c,r32(c,R13-0x7328u));} /* 800CE4EC/800CE6E4/800CE930 */
static int inhibited(C *c){return (r32(c,r32(c,R13-0x7078u)+0x34)&1u)!=0;} /* 80103D4C(0) */
static uint32_t controller(C *c,uint32_t g){ /* 800BE4F8 / 80020944 */
 uint32_t p=r32(c,R13-0x73d0u);if(!p)return 0;
 for(uint32_t i=0;i<r8(c,p)&&c->ok;++i){const uint32_t token=r32(c,p+0xbcu+0xf4u*i);
  if((token&255u)==1&&actor_at(c,(token>>8)&255u,(token>>16)&255u)==g)return p+8+0xf4u*i;
  p=r32(c,R13-0x73d0u);}
 return 0;}
static int gameplay(C *c,uint32_t g){ /* 8006510C including 80064C0C */
 const uint32_t header=r32(c,g);if((header&255u)!=1)return 0;
 const uint32_t team=r32(c,R13-0x5a98u)+((header>>8)&255u)*0x7878u+0x38u;
 if(inhibited(c)||!controller(c,g))return 0;
 if(g!=actor_at(c,side(c),r8(c,team+0x16))){
  const uint32_t role=r8(c,g+0xb5e);if(role!=1&&role!=3)return 0;
  if(r8(c,team_state(c)+0x17)!=2)return 0;
  if(!(mode_bits(c)&1u)&&(r32(c,g+0xc)&0x400u))return 0;
 }
 if(!phase(c)||state_word(c)!=3||r8(c,r32(c,g+0x310))==1||r8(c,g+0x20f)==1)return 0;
 /* 80064C0C: writes only its caller's stack byte; retain short-circuits
    and 8002E4F8's non-stack write even when the byte is ultimately zero. */
 if(r8(c,g+8)==255||!(r32(c,g+0xc)&0x4000u)){
  if(!inhibited(c)){const uint32_t role=r8(c,g+0xb5e);
   if((role==0||role==20||role==1||role==3)&&phase(c)&&state_word(c)==3&&
      r8(c,r32(c,g+0x310))==0x12&&r8(c,g+0x168)==1)return 0;}
  return 1;
 }
 if(!(mode_bits(c)&2u))return 1;
 uint32_t flags=r32(c,g+0xc);
 if((mode_bits(c)&1u)!=1){w32(c,g+0xc,flags|0x400000u);return 0;}
 if(flags&0x400000u){
  const float limit=nflstreet_gk_muls(rf(c,R13-0x70c4u),rf(c,0x8024fab4u));
  if(rf(c,g+0x1c4)>=limit)return 0;
  w32(c,g+0xc,flags&~0x400000u);
 }
 return 1;
}
/* 80188628's three original generators and 801EE1C0(0). These mutate the
   saved RNG state and both wrapper counters; no host RNG is substituted. */
static float random_zero(C *c){
 const uint32_t wrapper=r32(c,R13-0x61f8u),s=r32(c,r32(c,wrapper+0x14)),kind=r32(c,s);
 float f=0;uint32_t constant=0;
 if(kind==0){const uint32_t v=r32(c,s+4)*1664525u+1013904223u;w32(c,s+4,v);
  f=nflstreet_gk_convert(v,rd(c,0x80261700u));constant=0x80261708u;
 }else if(kind==1){
  /* 801881D0: Schrage step; shuffle index is arithmetic shift of the
     adjusted PREVIOUS +4 word, not of the new seed. */
  const int32_t old=(int32_t)r32(c,s+0x88);const int32_t q=old/127773;
  int32_t v=(int32_t)((uint32_t)(old-q*127773)*16807u-(uint32_t)q*2836u);
  if(v<0)v=(int32_t)((uint32_t)v+0x7fffffffu);
  int32_t previous=(int32_t)r32(c,s+4);if(previous<0)previous=(int32_t)((uint32_t)previous+0x03ffffffu);
  const uint32_t at=s+8+4u*(uint32_t)(previous>>26);w32(c,s+4,r32(c,at));w32(c,at,(uint32_t)v);w32(c,s+0x88,(uint32_t)v);
  f=nflstreet_gk_convert((uint32_t)v^0x80000000u,rd(c,0x80261710u));constant=0x80261718u;
 }else if(kind==2){
  uint32_t i=r32(c,s+4)+1u,j=r32(c,s+8)+1u;if((int32_t)i>55)i=1;if((int32_t)j>55)j=1;w32(c,s+4,i);w32(c,s+8,j);
  int32_t v=(int32_t)(r32(c,s+0x10+4*i)-r32(c,s+0x10+4*j));if(v<0)v=(int32_t)((uint32_t)v+1000000000u);
  w32(c,s+0x10+4*i,(uint32_t)v);f=nflstreet_gk_convert((uint32_t)v^0x80000000u,rd(c,0x80261720u));constant=0x80261728u;
 }else{c->unsupported=0x80188628u;c->ok=0;return 0;}
 f=nflstreet_gk_muls(f,rf(c,constant));const float ceiling=rf(c,constant+4);if(!(f<ceiling))f=ceiling;
 w32(c,wrapper+8,r32(c,wrapper+8)+1u);w32(c,wrapper+0xc,r32(c,wrapper+0xc)+1u);return f;
}
static int effect(C *c,uint32_t function,uint32_t g,uint32_t mode){
 if(!c->ok)return 0;if(!c->effect){c->unsupported=function;c->ok=0;return 0;}
 if(!c->effect(c->user,function,g,mode))c->ok=0;return c->ok;
}
static float suppression_weight(C *c,uint32_t g,unsigned slot){ /* 8004F118 / 8004F4C0 */
 const uint32_t p=r32(c,g+0x30c),index=p?r32(c,p+8):3,status=r8(c,r32(c,g+0x310));uint32_t table=0x8026e290u;
 switch(status){case 0x22:table=0x8026e0d0u;break;case 0x23:table=0x8026e060u;break;
 case 0x24:table=0x8026e140u;break;case 0x14:table=0x8026e1b0u;break;case 0x1c:table=0x8026e220u;break;default:break;}
 return rf(c,table+index*8u+4u*slot);
}
static int suppression(C *c,uint32_t g){ /* 8004F214; 8002BD94 always returns 1 */
 const float random=random_zero(c);
 const int32_t value=(int16_t)r16(c,g+0xbb4);
 const float amount=nflstreet_gk_convert((uint32_t)value^0x80000000u,rd(c,0x80246e78u));
 const float threshold=nflstreet_gk_muls(nflstreet_gk_muls(random,rf(c,0x80246e70u)),amount);
 const uint32_t status=r8(c,r32(c,g+0x310)),mode=status==0x22?3u:status==0x23?4u:5u;
 if(!(threshold>=suppression_weight(c,g,0))){effect(c,0x80076e18u,g,mode);return 1;}
 if(!(threshold>=suppression_weight(c,g,1))){effect(c,0x80076c48u,g,mode);return 1;}
 const int32_t max=(int16_t)r16(c,R13-0x7af8u);
 if(status-0x22u<3&&value<max){
  const float r=random_zero(c),den=nflstreet_gk_convert((uint32_t)max^0x80000000u,rd(c,0x80246e78u));
  const float left=rf(c,R13-0x7af4u),right=rf(c,R13-0x7af0u);
  const float a=nflstreet_gk_adds(nflstreet_gk_divs(nflstreet_gk_muls(amount,-left),den),left);
  if(!(r>=a)){effect(c,0x80076e18u,g,mode);return 1;}
  const float b=nflstreet_gk_adds(nflstreet_gk_divs(nflstreet_gk_muls(amount,-right),den),right);
  if(!(r>=b)){effect(c,0x80076c48u,g,mode);return 1;}
 }
 return 0;
}
static int guard(C *c,uint32_t g){ /* 80014644 / 80061158 */
 const uint32_t m=r32(c,g+0x318);if(r8(c,g+3)!=1||current(c,m)!=0x21)return 0;
 int blocked=1;const uint32_t flags=r32(c,g+0xc);
 if((flags&4u)||distance(r32(c,record(c,m,0x21)+0x14),r32(c,g+0x218))>0x2aaaaau){
  blocked=0;w32(c,g+0xc,r32(c,g+0xc)&~4u);}
 w8(c,g+0x21e,0);w8(c,g+0x20e,0);return blocked;
}
static int suppress_request(C *c,uint32_t g,uint32_t key){ /* 800146CC / 8001454C */
 if(r8(c,g+3)!=1)return 0;if(current(c,r32(c,g+0x318))==0x21)return guard(c,g);
 const uint32_t status=r8(c,r32(c,g+0x310));if(status==0x3d||status==0x50)return 0;
 if(key!=0x4a||rf(c,g+0x210)==rf(c,0x80244d54u)||distance(r32(c,g+0x1c0),r32(c,g+0x218))<=0x5fffffu)return 0;
 const uint32_t d=distance(r32(c,g+0x1c8),r32(c,g+0x214)),team=r8(c,g+2);int result=0;
 if(d>0x755555u&&team==own(c)&&rf(c,g+0x1c4)>=rf(c,0x80244d58u)&&suppression(c,g))result=1;
 if(d<0xaaaaau&&team==own(c)&&suppression(c,g))result=1;return result;
}
static void copy(C *c,uint32_t g){
 w32(c,g+0x210,r32(c,g+0x200));w32(c,g+0x214,r32(c,g+0x204));w32(c,g+0x218,r32(c,g+0x208));w8(c,g+0x21e,r8(c,g+0x20e));}
static void neutral(C *c,uint32_t g){
 w8(c,g+0x21e,1);w32(c,g+0x214,r32(c,g+0x204));w32(c,g+0x218,r32(c,g+0x204));w32(c,g+0x210,r32(c,g+0x200));}
static void turn(C *c,uint32_t g,uint32_t heading){ /* 8005335C(f1=80244D6C,r5=2,r6=0) */
 const uint32_t u=(heading-r32(c,g+0x1c0))&0xffffffu;int32_t d=(int32_t)(u<<8)>>8;
 if(d<-0x155555)d=-0x155555;else if(d>0x155555)d=0x155555;
 const int32_t scale=nflstreet_gk_fctiwz(nflstreet_gk_muls(rf(c,0x80244d6cu),rf(c,0x80247188u)));
 const uint64_t product=(uint64_t)((int64_t)d*scale);
 w32(c,g+0x13c,(uint32_t)(product>>24));w32(c,g+0x144,2);
}
static float floor_strength(C *c,uint32_t g,uint32_t constant){
 const float l=rf(c,R13-0x70c0u),f=nflstreet_gk_divs(nflstreet_gk_muls(l,rf(c,constant)),l),s=rf(c,g+0x200);
 /* fcmpu then cror eq,gt: unordered also selects the computed floor. */
 return s>=f?s:f;
}
static uint32_t direction(C *c,uint32_t g,int forced){ /* 80014898 */
 w32(c,g+0x208,r32(c,g+0x208)&0xffffffu);w32(c,g+0x204,r32(c,g+0x204)&0xffffffu);
 if(forced)w32(c,g+0x208,0xc00000u);
 const uint32_t h=r32(c,g+0x204),t=r32(c,g+0x208),d=distance(h,t);uint32_t key=0x4a;
 if(d<0x2e38e3u||rf(c,g+0x200)<=rf(c,0x80244d60u))goto flat;
 if(d<0x56c16cu){
  if(!forced&&rf(c,g+0x1ec)>rf(c,R13-0x70c0u))goto flat;
  key=(r8(c,g+2)==own(c)||forced)?0xa0u:0x49u;
  wf(c,g+0x210,floor_strength(c,g,key==0xa0?0x80244d64u:0x80244d68u));
  w8(c,g+0x21e,r8(c,g+0x20e));w32(c,g+0x218,t);w32(c,g+0x214,h);return key;
 }
 if(d<=0x71c71bu&&r8(c,g+2)==own(c)){
  if(!forced&&rf(c,g+0x1ec)>rf(c,R13-0x70bcu))goto flat;
  turn(c,g,t);int32_t delta=(int32_t)d-0x400000;key=0x9f;
  if(((t-h)&0xffffffu)<=0x800000u){key=0x9d;delta=-delta;}
  wf(c,g+0x210,floor_strength(c,g,0x80244d64u));w8(c,g+0x21e,r8(c,g+0x20e));
  w32(c,g+0x218,(t+(uint32_t)delta)&0xffffffu);w32(c,g+0x214,h);return key;
 }
 if(!forced&&rf(c,g+0x1ec)>rf(c,R13-0x70bcu))goto flat;
 copy(c,g);return key;
flat:
 if(t!=h)turn(c,g,t);neutral(c,g);return 0x4a;
}
static int special_direction(C *c,uint32_t g,uint32_t d,uint32_t *key){ /* 80014750 */
 if(d<0x1c71c7u||rf(c,g+0x200)==rf(c,0x80244d5cu)){*key=0x4a;neutral(c,g);return 0;}
 int restart=0;
 if(d<0x56c16cu&&(int32_t)r32(c,g+0x204)>0x800000){
  *key=0x54;const uint32_t m=r32(c,g+0x318);
  if(current(c,m)==0x54){const uint32_t rec=record(c,m,0x54);
   const float delay=rec?rf(c,rec+0xc):rf(c,0x8026118cu);
   if(delay!=rf(c,0x80244d5cu)){w32(c,g+0x208,r32(c,g+0x1c0));w32(c,g+0x200,r32(c,0x80244d5cu));}}
  restart=distance(r32(c,g+0x204),r32(c,g+0x214))>0x400000u;
 }else *key=0x4a;
 copy(c,g);return restart;
}
static int dispatch(C *c,uint32_t g,unsigned selector){
 const uint32_t old=current(c,r32(c,g+0x318));uint32_t key=old,weight=0;int blocked=0,restart=0;
 switch(selector){
 case 0:case 6:case 7:
  copy(c,g);
  if(selector&&r8(c,g+3)>=1&&r8(c,g+3)<=2){key=0x64;weight=0x80244d88u;break;}
  if(old==0x21||(old>=0x31&&old<=0x37)||(old>=0x39&&old<=0x3c)||old==0x49||old==0x54||old==0x64||
     (old>=0x6a&&old<=0x6c)||old==0xa0)key=0x4a;
  blocked=guard(c,g);weight=0x80244d70u;break;
 case 1:case 2:case 3:{
  const uint32_t d=distance(r32(c,g+0x204),r32(c,g+0x208));
  if(r8(c,g+3)==2){key=0x4a;neutral(c,g);}
  else if(!gameplay(c,g)){
   const int forced=(uint32_t)(r8(c,g+0x20f)-6u)<2u;
   if(forced&&rf(c,g+0x200)==rf(c,0x80244d78u))return dispatch(c,g,6);
   key=direction(c,g,forced);
  }else restart=special_direction(c,g,d,&key);
  if(key==0x21)key=0x4a;
  blocked=suppress_request(c,g,key);weight=0x80244d7cu;break;}
 case 4:
  w8(c,g+0x21e,r8(c,g+0x20e));w32(c,g+0x214,r32(c,g+0x204));w32(c,g+0x210,r32(c,g+0x200));guard(c,g);return c->ok;
 case 5:{
  const uint32_t status=r32(c,g+0x408);
  if(!((((old>=0x31&&old<=0x37)||(old>=0x39&&old<=0x3c)||(old>=0x6a&&old<=0x6c))&&status==4)||
       ((old==0x19||old==0x38||old==0x3d||old==0x4f)&&status==5)))key=0x4a;
  copy(c,g);blocked=guard(c,g);weight=0x80244d80u;break;}
 case 8:{
  const uint32_t h=r32(c,g+0x204),t=r32(c,g+0x208),d=distance(h,t);const float s=rf(c,g+0x200),v=rf(c,g+0x1ec),l=rf(c,R13-0x70c0u);
  if(d<0x2e38e3u||s==rf(c,0x80244d8cu)){key=0x4a;copy(c,g);w8(c,g+0x21e,1);}
  else if((d<0x56c16cu&&v>l)||(d>=0x56c16cu&&v>rf(c,R13-0x70bcu))){copy(c,g);w8(c,g+0x21e,1);}
  else{key=d<0x56c16cu?0xa0:0x4a;float magnitude=s;
   if(key==0xa0){const float floor=nflstreet_gk_divs(nflstreet_gk_muls(l,rf(c,0x80244d90u)),l);if(!(magnitude>=floor))magnitude=floor;}
   wf(c,g+0x210,magnitude);w32(c,g+0x214,h);w8(c,g+0x21e,r8(c,g+0x20e));}
  blocked=suppress_request(c,g,key);weight=0x80244d94u;break;}
 case 9:copy(c,g);key=0x4a;weight=0x80244d84u;break;
 case 10:
  neutral(c,g);w8(c,g+0x21e,r8(c,g+0x20e));key=0x4a;blocked=guard(c,g);weight=0x80244d74u;break;
 default:c->unsupported=0x800150e8u;return -1;
 }
 if(!c->ok)return 0;
 if(!blocked&&(key!=current(c,r32(c,g+0x318))||restart)){
  const nflstreet_locomotion_request r={r32(c,g+0x318),r32(c,g+0x31c),r32(c,g+0x320),key,g,rf(c,weight)};
  if(c->ok&&(!c->request||!c->request(c->user,&r)))c->ok=0;
 }
 return c->ok;
}
int nflstreet_locomotion_select(C *c,uint32_t g){if(!begin(c))return 0;return dispatch(c,g,r8(c,g+0x20e));}
int nflstreet_locomotion_gameplay(C *c,uint32_t g,uint32_t *value){if(!value||!begin(c))return 0;*value=(uint32_t)gameplay(c,g);return c->ok;}
int nflstreet_locomotion_suppression(C *c,uint32_t g,uint32_t *value){if(!value||!begin(c))return 0;*value=(uint32_t)suppression(c,g);return c->ok;}
