#include "clip_requests.h"
#include "gekko_float.h"
#include <string.h>
typedef nflstreet_clip_requests C;
#define R13 0x803a9a40u
#define RESOURCES 0x8032c34cu
#define ACTOR_STACK 0x8032bf10u
#define KEY_STACK 0x8032bf1cu
#define TRANSITIONS 0x80389e20u
/* Bank/group and program helpers follow the same original routines as
   actor_channels.c; kept local so this module adds no shared-file changes. */
static uint8_t *mem(C *c,uint32_t a,uint32_t n){
 if(c->scan_address&&a>=c->scan_address&&(uint64_t)a-c->scan_address+n<=sizeof c->scan)return c->scan+a-c->scan_address;
 uint8_t *p=a?c->span(c->memory,a,n):NULL;if(!p)c->ok=0;return p;}
static uint32_t r32(C *c,uint32_t a){const uint8_t *p=mem(c,a,4);return p?nflstreet_rd32(p):0;}
static uint16_t r16(C *c,uint32_t a){const uint8_t *p=mem(c,a,2);return p?nflstreet_rd16(p):0;}
static uint8_t r8(C *c,uint32_t a){const uint8_t *p=mem(c,a,1);return p?*p:0;}
static float rf(C *c,uint32_t a){return nflstreet_as_float(r32(c,a));}
static double rd(C *c,uint32_t a){return nflstreet_gk_value((uint64_t)r32(c,a)<<32|r32(c,a+4));}
static void w32(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,4);if(p)nflstreet_wr32(p,v);}
static void w16(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,2);if(p)nflstreet_wr16(p,v);}
static void w8(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,1);if(p)*p=(uint8_t)v;}
static void wf(C *c,uint32_t a,float v){w32(c,a,nflstreet_float_bits(v));}
static int begin(C *c){if(!c||!c->span)return 0;c->ok=1;c->scanning=0;c->scan_address=0;return 1;}
static uint32_t sp(C *c){return c->stack_top?c->stack_top:0x80f46fc0u;}
static uint32_t bank_header(C *c,uint32_t i){ /* 80175F48 */
 if(i>=r32(c,R13-0x4f70u))return 0;const uint32_t e=r32(c,R13-0x4f6cu)+0x1cu*i;return r16(c,e+0x14)==0x8000u?0:r32(c,e+0xc);}
static uint32_t group_row(C *c,uint32_t header,uint32_t key){ /* 80174860 */
 if(!header){c->ok=0;return 0xffffffffu;}const uint32_t rows=r32(c,header+8),n=r32(c,rows);
 for(uint32_t j=0;j<n&&c->ok;++j)if(r16(c,rows+4+0x10u*j)==key)return j;return 0xffffffffu;}
static uint32_t group_entry(C *c,uint32_t id,uint32_t key){ /* 80179650 */
 if(id>=r32(c,R13-0x4f50u))return 0;const uint32_t rec=r32(c,R13-0x4f48u)+0x14u*id;if(!rec)return 0;
 if(key>0x1fffu){key&=0x1fffu;if(key>=r16(c,rec+0xc))return 0;return r32(c,rec+0x10)+4*key;}
 if(key>=r16(c,rec+4))return 0;return r32(c,rec+8)+4*key;}
/* 801758E0: the banks (index, row) holding group key of id. */
static unsigned group_banks(C *c,uint32_t key,uint32_t id,uint16_t out[64],unsigned max){
 const uint32_t p=group_entry(c,id,key);unsigned n=0;
 if(p&&!(r16(c,p)&0x8000u)){out[0]=r16(c,p);out[1]=r16(c,p+2);return 1;}
 for(int32_t i=0;i<(int32_t)r32(c,R13-0x4f70u)&&c->ok;++i){const uint32_t e=r32(c,R13-0x4f6cu)+0x1cu*(uint32_t)i;
  if(r8(c,e+0x12)!=1||r16(c,e+0x14)!=id)continue;const uint32_t j=group_row(c,bank_header(c,(uint32_t)i),key);
  if(j!=0xffffffffu&&n<max){out[2*n+1]=(uint16_t)j;out[2*n]=(uint16_t)i;++n;}}
 if(p){if(n==1){w16(c,p,out[0]);w16(c,p+2,out[1]);}else{w16(c,p+2,0xffff);w16(c,p,0xffff);}}
 return n;}
/* 80175AF8 with 801748A0: the resource list of a group. */
static void group_resources(C *c,uint32_t id,uint32_t key,uint32_t out){
 w16(c,out,0);w16(c,out+2,0x17c);uint16_t pairs[64];const unsigned n=group_banks(c,key,id&0xffffu,pairs,0x20);
 for(unsigned j=0;j<n&&c->ok;++j){const int32_t bank=(int16_t)pairs[2*j],row=(int16_t)pairs[2*j+1];const uint32_t h=bank_header(c,(uint32_t)bank);if(!h)continue;
  const uint32_t list=r32(c,r32(c,h+8)+8+((uint32_t)row<<4));
  for(uint32_t k=0;k<r16(c,list)&&c->ok;++k){const uint32_t at=r16(c,out);if(at>=r16(c,out+2))return;
   w16(c,out+4+8*at,(uint32_t)bank);w16(c,out+6+8*r16(c,out),r16(c,list+4+8*k));w32(c,out+8+8*r16(c,out),r32(c,list+8+8*k));w16(c,out,r16(c,out)+1);}}}
/* 801778C0: resources, then the group's callback (r13-0x6718 tables). */
static void group_call(C *c,uint32_t id,uint32_t key,uint32_t actor,uint32_t mode,uint32_t set,uint32_t channels){
 if(key&0x8000u)return;group_resources(c,id,key,RESOURCES);if(!c->ok)return;
 const uint32_t fn=r32(c,r32(c,r32(c,r32(c,R13-0x6718u)+4*id)+4)+4*key);
 if(c->ok&&c->group)c->group(c->user,c,fn,RESOURCES,key,set,channels,actor,mode);}
static void stop_group(C *c,uint32_t set,uint32_t channels,uint32_t key){ /* 801740BC / 801740A8 */
 for(uint32_t i=0;i<r16(c,set+4)&&c->ok;++i)if(r8(c,channels+0x7cu*i+1)&&r16(c,channels+0x7cu*i+8)==key)w8(c,channels+0x7cu*i+1,0);}
static unsigned playing(C *c,uint32_t set,uint32_t channels,uint32_t key){ /* 8017428C */
 unsigned n=0;for(uint32_t i=0;i<r16(c,set+4)&&c->ok;++i)if(r8(c,channels+0x7cu*i+1)==2&&r16(c,channels+0x7cu*i+8)==key)n=(n+1)&0xffffu;return n;}
static void retire(C *c,uint32_t manager,uint32_t set,uint32_t channels,unsigned k,uint32_t actor){ /* 80177A64 */
 const uint32_t rec=manager+100u*k;group_call(c,r16(c,set+2),r16(c,rec+4),actor,3,set,channels);stop_group(c,set,channels,r16(c,rec+4));w16(c,rec+6,0);}
static void compact(C *c,uint32_t m){ /* 80177950 */
 int last=3;if(!r16(c,m+0x132)){uint32_t at=m+0x132;do{if(--last<0)return;at-=0x64;}while(!r16(c,at));}
 if(last<=0)return;const uint32_t k=r32(c,0x80261160u);
 for(int i=0;i<last&&c->ok;){if(r16(c,m+6+100u*(uint32_t)i)){++i;continue;}
  for(int j=i;j<last&&c->ok;++j){const uint32_t to=m+100u*(uint32_t)j+4;for(uint32_t b=0;b<0x64;b+=4)w32(c,to+b,r32(c,to+100+b));
   w16(c,m+100u*(uint32_t)(j+1)+6,0);w32(c,m+0xc+100u*(uint32_t)(j+1),k);}
  --last;}}
static uint32_t operand(C *c,uint32_t s){ /* 80176DD0 */
 const uint32_t i=r16(c,s+0x18),prog=r32(c,s)+4;const uint16_t w=r16(c,prog+2*i);w16(c,s+0x18,i+1);
 uint32_t v=w&0xfffu;if(w&0x800)v|=0xf000u;if(!(w&0x1000))return v;
 const uint16_t x=r16(c,prog+((2u*(i+1u))&0x1fffeu));w16(c,s+0x18,i+2);return (v<<16)+x;}
static uint32_t event(C *c,uint32_t s,uint32_t command){ /* 80173A24 */
 w32(c,R13-0x4f58u,s);if(c->scanning)return c->event?c->event(c->user,c,0,command,0,1):1u;const int32_t sel=(int8_t)r8(c,R13-0x579cu);
 const uint32_t key=r32(c,KEY_STACK+4u*(uint32_t)sel),actor=r32(c,ACTOR_STACK+4u*(uint32_t)sel);if(!c->ok)return 0;
 return c->event?c->event(c->user,c,c->scanning?0:actor,command,c->scanning?0:key,c->scanning):1u;}
/* 80176E28(f1, stream, 0): the next frame, or 0xFFFF at the end. */
static uint32_t next_frame(C *c,uint32_t s,float f31){
 const double k30=rd(c,0x802610f8u);const float f29=rf(c,0x80261100u);uint32_t ret=0xffff;int done=0;
 for(unsigned guard=0;!done&&c->ok;++guard){if(guard>0x100000u){c->ok=0;break;}
  const uint32_t i=r16(c,s+0x18),prog=r32(c,s)+4,op=r16(c,prog+2*i);w16(c,s+0x18,i+1);
  if(!(op&0x8000u)){ret=op&0x7fffu;done=1;w16(c,s+0x1c,r16(c,s+0x1c)+1u);continue;}
  if(op&0x4000u){if(!event(c,s,op)&&(op&0x1000u))w16(c,s+0x18,r16(c,s+0x18)+1u);continue;}
  switch(op){
  case 0x8000:if(r16(c,s+0x1a)==0xffffu)w16(c,s+0x1a,r16(c,s+0x1c));done=1;break;
  case 0x8001:w16(c,s+0x18,0);if(r16(c,s+0x1a)==0xffffu)w16(c,s+0x1a,r16(c,s+0x1c));break;
  case 0x8002:{const uint32_t d=(uint8_t)(r8(c,s+0x1f)+1u);w8(c,s+0x1f,d);w16(c,s+0x20+((d-1u)<<3),i+1);
   const uint32_t d2=r8(c,s+0x1f);w16(c,s+0x22+((d2-1u)<<3),0);const uint32_t d3=r8(c,s+0x1f);
   wf(c,s+0x24+((d3-1u)<<3),nflstreet_gk_adds(rf(c,s+8),f31));break;}
  case 0x8003:{const uint32_t v=operand(c,s)&0xffffu,d=r8(c,s+0x1f),at=s+((d-1u)<<3);const uint32_t n=r16(c,at+0x22);
   if(!n)w16(c,at+0x22,v);else w16(c,at+0x22,n-1);
   const uint32_t d2=r8(c,s+0x1f),at2=s+((d2-1u)<<3);
   if(r16(c,at2+0x22)){w16(c,s+0x18,r16(c,at2+0x20));f31=rf(c,0x802610f0u);wf(c,s+8,rf(c,s+0x24+((d2-1u)<<3)));}
   else w8(c,s+0x1f,d2-1u);break;}
  case 0x8004:{const uint32_t v=operand(c,s);const float f13=rf(c,s+0xc);
   wf(c,s+0xc,nflstreet_gk_fmadds(nflstreet_gk_convert(v^0x80000000u,k30),f29,f13));break;}
  case 0x8005:w8(c,s+0x1e,r8(c,s+0x1e)|9u);break;
  case 0x8006:w8(c,s+0x1e,r8(c,s+0x1e)|0x24u);break;
  default:if((op&0xe000u)==0xa000u&&(op&0x1000u))w16(c,s+0x18,r16(c,s+0x18)+1u);}}
 wf(c,s+8,nflstreet_gk_adds(rf(c,s+8),f31));return ret;}
/* 80177374(dt, stream): returns 1 at the program's end. */
static int timing(C *c,double dt,uint32_t s){
 const float old=rf(c,s+0xc),zero=rf(c,0x8026111cu),rem=nflstreet_gk_subs(old,dt);int end=0;wf(c,s+0xc,rem);
 if(rem>=zero)wf(c,s+8,nflstreet_gk_adds(rf(c,s+8),dt));
 else{float f1=old;
  for(;c->ok;){w16(c,s+0x10,r16(c,s+0x12));const uint32_t f=next_frame(c,s,f1);w16(c,s+0x12,f);
   if(f==0xffffu){end=1;w16(c,s+0x12,r16(c,s+0x10));}
   const float p=rf(c,s+4),r=nflstreet_gk_adds(rf(c,s+0xc),p);wf(c,s+0xc,r);f1=p;if(r>=zero||end)break;}
  wf(c,s+8,nflstreet_gk_adds(rf(c,s+8),nflstreet_gk_subs(rf(c,s+4),rf(c,s+0xc))));}
 float frac;if(end)frac=rf(c,0x80261120u);else{const float p=rf(c,s+4);frac=p==zero?zero:nflstreet_gk_divs(nflstreet_gk_subs(p,rf(c,s+0xc)),p);}
 wf(c,s+0x14,frac);return end;}
static void push(C *c,uint32_t index,uint32_t stack,uint32_t v){const uint32_t b=(uint8_t)(r8(c,index)+1u);w8(c,index,b);w32(c,stack+4u*(uint32_t)(int32_t)(int8_t)b,v);}
int nflstreet_clip_request_operand(C *c,uint32_t *value){if(!c||!value)return 0;*value=operand(c,r32(c,R13-0x4f58u));return c->ok;}
static void pop(C *c){w8(c,R13-0x579bu,r8(c,R13-0x579bu)-1u);w8(c,R13-0x579cu,r8(c,R13-0x579cu)-1u);}
/* 801791A8 / 80179288 / 80179310. The mode-size table controls copying. */
static void constant(C *c,uint32_t b,float a,float z){const float keep=rf(c,0x802611e0u);
 if(a!=keep)wf(c,b,a);if(z!=keep)wf(c,b+4,z);w32(c,b+8,0);}
static void linear(C *c,uint32_t b,float a,float z,uint32_t n){if(a==rf(c,0x802611e4u))a=rf(c,b);
 if(a!=rf(c,0x802611e0u))wf(c,b,a);if(z!=rf(c,0x802611e0u))wf(c,b+4,z);w32(c,b+8,1);
 const unsigned size=r8(c,r32(c,R13-0x6710u)+1);if(size){if(size!=8){c->ok=0;return;}wf(c,b+0xc,a);w32(c,b+0x10,n);}}
static void phases(C *c,uint32_t b,float a,float z,uint32_t rise,uint32_t hold){if(a==rf(c,0x802611e8u))a=rf(c,b);
 if(a!=rf(c,0x802611e0u))wf(c,b,a);if(z!=rf(c,0x802611e0u))wf(c,b+4,z);w32(c,b+8,3);
 const unsigned size=r8(c,r32(c,R13-0x6710u)+3);if(size){if(size!=12){c->ok=0;return;}wf(c,b+0xc,a);
 wf(c,b+0x10,nflstreet_gk_convert(rise,rd(c,0x802611f0u)));w8(c,b+0x14,rise);w8(c,b+0x15,0);w16(c,b+0x16,hold);}}
/* 8017724C. Scanning selects r13-6730, not the normal event provider. */
static int initialize(C *c,uint32_t s,uint32_t program,int scan){
 w32(c,s,program);float p=nflstreet_gk_muls(nflstreet_gk_convert(r8(c,program),rd(c,0x80261108u)),rf(c,0x80261110u));
 const float zero=rf(c,0x80261114u);if(!(p>zero))p=rf(c,0x80261118u);wf(c,s+4,p);w16(c,s+0x1c,0);wf(c,s+8,zero);
 w16(c,s+0x18,0);w16(c,s+0x1a,0xffff);w8(c,s+0x1e,0);w8(c,s+0x1f,0);wf(c,s+0xc,p);
 const int saved=c->scanning;c->scanning=scan;w16(c,s+0x10,next_frame(c,s,zero));const uint32_t f=next_frame(c,s,zero);
 c->scanning=saved;wf(c,s+0x14,zero);w16(c,s+0x12,f==0xffff?r16(c,s+0x10):f);return f==0xffff;}
static int initialize_channel(C *c,uint32_t ch,uint32_t program,uint32_t actor){ /* 80174444 */
 push(c,R13-0x579bu,KEY_STACK,r16(c,ch+8));push(c,R13-0x579cu,ACTOR_STACK,actor);
 const int end=initialize(c,ch+0x4c,program,0);pop(c);w8(c,ch+7,r8(c,program+2));return end;}
/* 801770FC: scans the instruction words, without interpreting loop or rate
   commands after the first pair. The local stream leaks to r13-4F58 on events.
   scratch is the original caller's stack address, retained as a RAM token. */
static float scan_program(C *c,uint32_t program,uint32_t scratch,uint16_t *count){
 const uint32_t prev=c->scan_address;c->scan_address=scratch;memset(c->scan,0,sizeof c->scan);
 initialize(c,scratch,program,1);wf(c,scratch+8,nflstreet_gk_adds(rf(c,scratch+8),nflstreet_gk_adds(rf(c,scratch+0xc),rf(c,scratch+0xc))));
 for(unsigned guard=0;c->ok;++guard){if(guard>0x100000u){c->ok=0;break;}
  const uint32_t i=r16(c,scratch+0x18),op=r16(c,r32(c,scratch)+4+2*i);w16(c,scratch+0x18,i+1);
  if(!(op&0x8000u)){w16(c,scratch+0x1c,r16(c,scratch+0x1c)+1);wf(c,scratch+8,nflstreet_gk_adds(rf(c,scratch+8),rf(c,scratch+0xc)));}
  else if(op&0x4000u){w32(c,R13-0x4f58u,scratch);if(c->event)c->event(c->user,c,0,op,0,1);}
  else if(op==0x8000u||op==0x8001u){if(op==0x8000u)wf(c,scratch+8,nflstreet_gk_subs(rf(c,scratch+8),rf(c,scratch+0xc)));
   if(r16(c,scratch+0x1a)==0xffff)w16(c,scratch+0x1a,r16(c,scratch+0x1c));break;}
  else if(op>0x8006u&&(op&0xe000u)==0xa000u&&(op&0x1000u))w16(c,scratch+0x18,r16(c,scratch+0x18)+1);
  if(op==0xffffu)break; /* 801770FC also stops at its explicit sentinel after the callback. */
 }
 const float time=rf(c,scratch+8);*count=r16(c,scratch+0x1a);c->scan_address=prev;return time;}
static uint32_t program_at(C *c,uint32_t bank,uint32_t clip){const uint32_t h=bank_header(c,bank);return r32(c,r32(c,h+0xc)+4+4*clip);}
static int channel_slot(C *c,uint32_t set,uint32_t ch){ /* 80173C24 */
 const uint32_t n=r16(c,set+4);for(unsigned st=0;st<2;++st)for(uint32_t i=0;i<n&&c->ok;++i)if(r8(c,ch+0x7c*i+1)==st)return (int)i;
 int slot=-1;float weight=rf(c,0x802610c0u);for(uint32_t i=0;i<n&&c->ok;++i){const uint32_t a=ch+0x7c*i;
  if((r16(c,a+8)&0x8000u)&&rf(c,a+0x30)<=weight){slot=(int)i;weight=rf(c,a+0x30);}}
 if(slot!=-1)w8(c,ch+0x7cu*(unsigned)slot+1,0);return slot;}
static int start(C *c,float weight,uint32_t set,uint32_t ch,uint32_t bank,uint32_t clip,uint32_t key,uint32_t actor,uint32_t row_key){
 const int i=channel_slot(c,set,ch);if(i==-1)return i;const uint32_t a=ch+0x7cu*(unsigned)i;
 constant(c,a+0x30,weight,weight);const uint32_t h=bank_header(c,bank);w16(c,a+4,r16(c,h+2));w8(c,a+6,bank);w16(c,a+2,clip);w16(c,a+8,key);
 w16(c,a+0xa,row_key==0xffff&&(key&0x8000u)?0xffff:group_row(c,bank_header(c,bank&0xffu),row_key==0xffff?key:row_key));
 w32(c,a+0xc,r32(c,set+8));w32(c,a+0x1c,0);w32(c,a+0x20,0x1000000);w32(c,a+0x2c,r32(c,0x802610d0u));
 w32(c,a+0x48,r32(c,0x802610ccu));w32(c,a+0x28,r32(c,0x802610ccu));w32(c,a+0x24,r32(c,0x802610ccu));
 if(!initialize_channel(c,a,program_at(c,bank,clip),actor))w8(c,a+1,2);return i;}
int nflstreet_clip_request_start(C *c,float weight,uint32_t set,uint32_t ch,uint32_t bank,uint32_t clip,uint32_t key,uint32_t actor,uint32_t row_key,int32_t *index){
 if(!index||!begin(c))return 0;*index=start(c,weight,set,ch,bank,clip,key,actor,row_key);return c->ok;}
int nflstreet_clip_request_replace(C *c,uint32_t set,uint32_t ch,uint32_t index,uint32_t bank,uint32_t clip,uint32_t key,uint32_t actor){
 (void)set;if(!begin(c))return 0;const uint32_t a=ch+0x7cu*index;if(r8(c,a+1)!=2)return c->ok;
 w16(c,a+4,r16(c,bank_header(c,bank)+2));w16(c,a+8,key);w8(c,a+6,bank);w16(c,a+2,clip);
 uint16_t n=r16(c,a+0x66);const uint32_t scratch=sp(c)-0x48u-0x38u+8;
 if(n==0xffff){scan_program(c,r32(c,a+0x4c),scratch,&n);w16(c,a+0x66,n);}
 const float length=nflstreet_gk_muls(rf(c,a+0x50),nflstreet_gk_convert(n,rd(c,0x802610d8u)));
 const double elapsed=fmod((double)rf(c,a+0x54),(double)length);
 if(initialize_channel(c,a,program_at(c,bank,clip),actor)==1)w8(c,a+1,1);
 else{scan_program(c,r32(c,a+0x4c),scratch,&n);w16(c,a+0x66,n);
  const float rate=nflstreet_gk_divs(length,nflstreet_gk_convert(n,rd(c,0x802610d8u))),old=rf(c,a+0x50);
  if(old==rf(c,0x80261124u)){w32(c,a+0x54,r32(c,0x80261124u));w32(c,a+0x58,r32(c,0x80261124u));}
  else{wf(c,a+0x54,nflstreet_gk_divs(nflstreet_gk_muls(rf(c,a+0x54),rate),old));wf(c,a+0x58,nflstreet_gk_divs(nflstreet_gk_muls(rf(c,a+0x58),rate),old));}
  wf(c,a+0x50,rate);push(c,R13-0x579cu,ACTOR_STACK,actor);push(c,R13-0x579bu,KEY_STACK,r16(c,a+8));
  /* f1 remains the double returned by fmod, through the first fsubs. */
  const int end=timing(c,elapsed,a+0x4c);pop(c);if(end==1)w8(c,a+1,1);}
 return c->ok;}
/* 80177DC8: first unused temporary key, descending from FFFE. */
static uint32_t temporary(C *c,uint32_t m){for(uint32_t k=0xfffe;c->ok;k=(k-1)&0xffffu){int found=0;for(unsigned i=0;i<4;++i)found|=r16(c,m+100*i+4)==k;if(!found)return k;}return 0;}
static unsigned manager_slot(C *c,uint32_t m,uint32_t set,uint32_t ch,uint32_t actor){
 compact(c,m);for(unsigned i=0;i<4;++i)if(!r16(c,m+100*i+6))return i;
 int victim=-1;for(unsigned i=0;i<4;++i)if(rf(c,m+100*i+0xc)!=rf(c,0x80261164u)){victim=(int)i;break;}
 for(unsigned pass=0;victim==-1&&pass<2;++pass)for(unsigned i=0;i<4;++i){const uint32_t a=m+100*i,key=r16(c,a+4);
  if(r16(c,a+6)==1&&key!=0xffff&&(!pass?((key&0x8000u)&&rf(c,a+0x50)<=rf(c,0x80261168u)):1)&&!playing(c,set,ch,key)){victim=(int)i;break;}}
 if(victim==-1){float low=rf(c,0x8026116cu);for(unsigned i=0;i<4;++i){const uint32_t a=m+100*i;
  if(r16(c,a+6)==1&&r16(c,a+4)!=0xffff&&rf(c,a+0x50)<low){victim=(int)i;low=rf(c,a+0x50);}}}
 if(victim==-1){retire(c,m,set,ch,0,actor);compact(c,m);return 3;}
 retire(c,m,set,ch,(unsigned)victim,actor);compact(c,m);for(unsigned i=0;i<4;++i)if(!r16(c,m+100*i+6))return i;c->ok=0;return 3;}
static uint32_t group_key(C *c,uint32_t m){uint32_t key=0xffff;for(int i=3;i>=0;--i)if(r16(c,m+100u*(unsigned)i+6)==3)key=r16(c,m+100u*(unsigned)i+4);return key;}
/* 80175C88 / 80174934: transition table list, also populates bank caches. */
static uint32_t transition_catalog(C *c,uint32_t id,uint32_t key){
 w16(c,TRANSITIONS,0);w16(c,TRANSITIONS+2,8);w16(c,TRANSITIONS+4,key);uint16_t pairs[64];const unsigned n=group_banks(c,key,id,pairs,32);
 for(unsigned j=0;j<n&&c->ok;++j){const uint32_t h=bank_header(c,(uint32_t)(int32_t)(int16_t)pairs[2*j]);
  const uint32_t rows=r32(c,h+8),p=r32(c,rows+0x10+16u*(uint32_t)(int32_t)(int16_t)pairs[2*j+1]),count=r16(c,TRANSITIONS);
  if(count>=r16(c,TRANSITIONS+2)||!p||!r32(c,p))continue;
  w16(c,TRANSITIONS+8+8*count,pairs[2*j]);w16(c,TRANSITIONS+10+8*count,r16(c,h+2));w32(c,TRANSITIONS+12+8*count,p);w16(c,TRANSITIONS,count+1);}
 return r16(c,TRANSITIONS);}
static uint32_t default_row(C *c,uint32_t id){ /* 80175E00 */
 for(int32_t i=0;i<(int32_t)r32(c,R13-0x4f70u)&&c->ok;++i){const uint32_t e=r32(c,R13-0x4f6cu)+0x1cu*(uint32_t)i;
  if(r8(c,e+0x12)!=1||r16(c,e+0x14)!=id)continue;const uint32_t h=r32(c,e+0xc),rows=r32(c,h+8);
  for(uint32_t j=0;j<r16(c,rows+2)&&c->ok;++j){const uint32_t key=r16(c,rows+4+16*j);if((key&0x4000u)&&(r16(c,h+6)&8))return key;}}
 return 0xffff;}
static uint32_t find_bank(C *c,uint32_t id,uint32_t file){ /* 80175D18 */
 for(int32_t i=0;i<(int32_t)r32(c,R13-0x4f70u)&&c->ok;++i){const uint32_t e=r32(c,R13-0x4f6cu)+0x1cu*(uint32_t)i;
  if(r8(c,e+0x12)==1&&r16(c,e+0x14)==id&&r16(c,r32(c,e+0xc)+2)==file)return (uint32_t)i;}return 0xffffff;}
static uint32_t strongest(C *c,uint32_t set,uint32_t ch,uint32_t key){float high=rf(c,0x80261190u);uint32_t index=0xffff;
 for(unsigned i=0;i<r16(c,set+4)&&c->ok;++i){const uint32_t a=ch+0x7c*i;
  if(r8(c,a+1)==2&&r16(c,a+8)==key&&rf(c,a+0x30)>high){index=i;high=rf(c,a+0x30);}}return index;}
/* 80178D8C: the LAST matching row wins; entries start four bytes before
   their first key (the previous row's result pointer is the next count word). */
static uint32_t transition_rule(C *c,uint32_t from_clip,uint32_t position,uint32_t new_key,uint32_t to_clip,uint32_t to_file){
 uint32_t result=0,target=to_clip==0xffff?0xffff:to_file|0x8000u;if(to_clip!=0xffff)new_key=to_clip;
 for(unsigned j=0;j<r16(c,TRANSITIONS)&&c->ok;++j){uint32_t row=r32(c,TRANSITIONS+12+8*j);const int32_t n=(int32_t)r32(c,row);
  for(int32_t k=0;k<n&&c->ok;++k,row+=16){const uint32_t id=r16(c,row+4);
   if((id==target||(target==0xffff&&!(id&0x8000u)))&&r16(c,row+6)==new_key&&r16(c,row+8)==from_clip&&
      (from_clip==0xffff||(r16(c,row+10)<=position&&position<=r16(c,row+12))))result=r32(c,row+16);}}
 return result;}
static uint32_t uint_from_float(C *c,double f){const double k=rd(c,0x802611a8u);return f>=k?(uint32_t)nflstreet_gk_fctiwz(f-k)^0x80000000u:(uint32_t)nflstreet_gk_fctiwz(f);}
static void transition(C *c,uint32_t m,uint32_t set,uint32_t ch,uint32_t new_key,uint32_t old_key,uint32_t state,uint32_t actor,uint32_t entry_sp){
 int fallback=1;for(unsigned i=0;i<4&&c->ok;++i){const uint32_t a=m+100*i,key=r16(c,a+4),b=a+0x50;
  if(r16(c,a+6)!=state||key==0xffff){if(key==0xffff)w16(c,a+10,default_row(c,r16(c,set+2)));continue;}
  w16(c,a+6,1);if(r16(c,TRANSITIONS+4)!=old_key)transition_catalog(c,r16(c,set+2),old_key);
  uint32_t rule=0,from=0,to=0;
  if(r16(c,TRANSITIONS)){
   const uint32_t fi=strongest(c,set,ch,key),ti=strongest(c,set,ch,new_key);
   if(fi!=0xffff&&ti!=0xffff){from=ch+0x7c*fi;to=ch+0x7c*ti;
    rule=transition_rule(c,r16(c,from+2),r16(c,from+0x64),new_key,r16(c,to+2),r16(c,to+4));
    if(!rule)rule=transition_rule(c,r16(c,from+2),r16(c,from+0x64),new_key,0xffff,0);
    if(!rule)rule=transition_rule(c,0xffff,0,new_key,r16(c,to+2),r16(c,to+4));
    if(!rule)rule=transition_rule(c,0xffff,0,new_key,0xffff,0);
   }
  }
  if(rule){const uint32_t type=r16(c,rule+4);
   if(type==0||type==2){constant(c,b,rf(c,0x80261194u),rf(c,0x80261194u));fallback=0;}
   else if(type==1){const float n=nflstreet_gk_convert(r16(c,rule+6)^0x80000000u,rd(c,0x80261198u)),mix=rf(c,rule+8);
    const float right=nflstreet_gk_muls(nflstreet_gk_muls(n,rf(c,to+0x50)),mix),left=nflstreet_gk_muls(n,rf(c,from+0x50));
    const uint32_t ticks=uint_from_float(c,nflstreet_gk_fmadds(left,nflstreet_gk_subs(rf(c,0x802611a0u),mix),right));
    linear(c,b,rf(c,0x802611b0u),rf(c,0x80261194u),ticks);fallback=0;linear(c,b+200,rf(c,0x80261194u),rf(c,a+0x54+200),ticks);
   }else if(type==3){w16(c,a+10,default_row(c,r16(c,set+2)));const uint32_t bank=find_bank(c,r16(c,set+2),r16(c,rule+0xc));
    const int slot=bank==0xffffff?-1:start(c,rf(c,0x802611a0u),set,ch,bank&0xffffu,r16(c,rule+0xe),0xffff,actor,r16(c,a+10));
    if(slot==-1)fallback=1;else{uint16_t unused;const float length=scan_program(c,r32(c,ch+0x7cu*(unsigned)slot+0x4c),entry_sp-0x98u-0x38u+8,&unused);
     const uint32_t rise=uint_from_float(c,nflstreet_gk_muls(nflstreet_gk_muls(length,nflstreet_gk_subs(rf(c,0x802611a0u),rf(c,rule+8))),rf(c,0x802611b4u)));
     const uint32_t hold=uint_from_float(c,length)-2u*rise;
     linear(c,b,rf(c,0x802611b0u),rf(c,0x80261194u),rise);phases(c,b+100,rf(c,0x802611b0u),rf(c,0x802611a0u),rise&255u,hold&255u);
     linear(c,b+200,rf(c,0x80261194u),rf(c,a+0x54+200),rise);
     const float delay=nflstreet_gk_adds(nflstreet_gk_convert(hold,rd(c,0x802611b8u)),nflstreet_gk_convert(rise,rd(c,0x802611b8u)));
     for(unsigned j=0;j<4;++j)if(r16(c,m+100*j+6)&&r16(c,m+100*j+4)==new_key)wf(c,m+100*j+0xc,delay);
     for(unsigned j=0;j<r16(c,set+4)&&c->ok;++j)if(r8(c,ch+0x7c*j+1)==2&&r16(c,ch+0x7c*j+8)==new_key)wf(c,ch+0x7c*j+0x48,delay);fallback=0;
    }
   }
  }
  if(fallback)linear(c,b,rf(c,0x802611b0u),rf(c,0x80261194u),10);
 }
}
int nflstreet_clip_request_transition(C *c,uint32_t m,uint32_t set,uint32_t ch,uint32_t new_key,uint32_t old_key,uint32_t state,uint32_t actor){
 if(!begin(c))return 0;transition(c,m,set,ch,new_key,old_key,state,actor,sp(c));return c->ok;}
static void rekey(C *c,uint32_t set,uint32_t ch,uint32_t old,uint32_t key){
 for(unsigned i=0;i<r16(c,set+4)&&c->ok;++i)if(r8(c,ch+0x7c*i+1)==2&&r16(c,ch+0x7c*i+8)==old)w16(c,ch+0x7c*i+8,key);}
int nflstreet_clip_request_group(C *c,float weight,uint32_t m,uint32_t set,uint32_t ch,uint32_t key,uint32_t actor){
 if(!begin(c))return 0;const uint32_t old=group_key(c,m),zero=r32(c,0x80261170u);
 for(unsigned i=0;i<4&&c->ok;++i){const uint32_t a=m+100*i;if(r16(c,a+6)!=3)continue;w16(c,a+6,2);
  if(rf(c,a+0xc)!=nflstreet_as_float(zero)&&i){retire(c,m,set,ch,i,actor);const uint32_t prev=a-100;
   if(r16(c,prev+4)==0xffff){w16(c,prev+4,temporary(c,m));linear(c,prev+0x50,rf(c,0x80261174u),nflstreet_as_float(zero),10);rekey(c,set,ch,0xffff,r16(c,prev+4));}}
  break;}
 for(unsigned i=0;i<4&&c->ok;++i){const uint32_t a=m+100*i,k=r16(c,a+4);if(!r16(c,a+6)||(k!=key&&k!=0xffff))continue;
  w16(c,a+4,temporary(c,m));w16(c,a+8,r16(c,a+8)&0xfffeu);rekey(c,set,ch,k,r16(c,a+4));}
 if(transition_catalog(c,r16(c,set+2),old)){
  const uint32_t a=m+100*manager_slot(c,m,set,ch,actor);uint8_t *p=mem(c,a+4,100);if(p)memset(p,0,100);w16(c,a+6,1);w16(c,a+4,0xffff);w32(c,a+0xc,zero);constant(c,a+0x50,nflstreet_as_float(zero),nflstreet_as_float(zero));}
 const uint32_t a=m+100*manager_slot(c,m,set,ch,actor);uint8_t *p=mem(c,a+4,100);if(p)memset(p,0,100);
 w16(c,a+6,3);w16(c,a+4,key);w32(c,a+0xc,zero);linear(c,a+0x50,nflstreet_as_float(zero),weight,10);
 group_call(c,r16(c,set+2),key,actor,0,set,ch);transition(c,m,set,ch,key,old,2,actor,sp(c)-0x50u);return c->ok;}
