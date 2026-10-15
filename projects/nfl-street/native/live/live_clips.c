#include "live_clips.h"
#include "gekko_float.h"
#include <string.h>
#define R13 0x803a9a40u
typedef nflstreet_live_clips C;
static uint8_t *mem(C *c,uint32_t a,uint32_t n){uint8_t *p=a?c->requests.span(c->requests.memory,a,n):NULL;if(!p)c->requests.ok=0;return p;}
static uint32_t r32(C *c,uint32_t a){const uint8_t *p=mem(c,a,4);return p?nflstreet_rd32(p):0;}
static uint16_t r16(C *c,uint32_t a){const uint8_t *p=mem(c,a,2);return p?nflstreet_rd16(p):0;}
static uint8_t r8(C *c,uint32_t a){const uint8_t *p=mem(c,a,1);return p?*p:0;}
static float rf(C *c,uint32_t a){return nflstreet_as_float(r32(c,a));}
static void w32(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,4);if(p)nflstreet_wr32(p,v);}
static void w8(C *c,uint32_t a,uint8_t v){uint8_t *p=mem(c,a,1);if(p)*p=v;}
static uint32_t distance(uint32_t a,uint32_t b){const uint32_t d=(a-b)&0xffffffu;return d<0x1000000u-d?d:0x1000000u-d;}
static uint32_t current_key(C *c,uint32_t m){ /* 801784B8: first state-3 record */
 for(unsigned i=0;i<4;++i)if(r16(c,m+6+100*i)==3)return r16(c,m+4+100*i);return 0xffffu;}
static void copy_input(C *c,uint32_t g){
 w32(c,g+0x210,r32(c,g+0x200));w32(c,g+0x214,r32(c,g+0x204));w32(c,g+0x218,r32(c,g+0x208));w8(c,g+0x21e,r8(c,g+0x20e));}
/* 80014644 with 80061158/8017847C's heading test. */
static int guard(C *c,uint32_t g,uint32_t m,uint32_t key){
 if(r8(c,g+3)!=1||key!=0x21)return 0;int blocked=1;uint32_t flags=r32(c,g+0xc);
 if(flags&4u)blocked=0;
 else for(unsigned i=0;i<4;++i)if(r16(c,m+6+100*i)&&r16(c,m+4+100*i)==key){
  blocked=distance(r32(c,m+0x10+100*i),r32(c,g+0x218))<=0x2aaaaau;break;}
 if(!blocked)w32(c,g+0xc,flags&~4u);w8(c,g+0x20e,0);w8(c,g+0x21e,0);return blocked;}
/* 800146CC/8001454C: preserve the short-circuits; never guess 8004F214,
   which can change gameplay state as well as return a predicate. */
static int suppression(C *c,uint32_t g,uint32_t m,uint32_t old,uint32_t key){
 if(r8(c,g+3)!=1)return 0;if(old==0x21)return guard(c,g,m,old);
 const uint8_t status=r8(c,r32(c,g+0x310));if(status==0x3d||status==0x50)return 0;
 if(key!=0x4a||rf(c,g+0x210)==rf(c,0x80244d54u)||distance(r32(c,g+0x1c0),r32(c,g+0x218))<=0x5fffffu)return 0;
 const uint32_t d=distance(r32(c,g+0x1c8),r32(c,g+0x214)),world=r32(c,R13-0x7078u);
 const uint32_t own=world?(uint32_t)(r8(c,world+0x38)^1u):0;
 if(r8(c,g+2)==own&&((d>0x755555u&&rf(c,g+0x1c4)>=rf(c,0x80244d58u))||d<0xaaaaau))return -1;
 return 0;}
/* Request scans use 80015D8C, NOT the live 8001532C callback. This small
   source/disassembly-derived adapter consumes its operands through the
   request context, whose scan stream is private shadow storage. */
static uint32_t request_event(void *user,nflstreet_clip_requests *r,uint32_t actor,uint32_t command,uint32_t key,int scanning){
 C *c=user;if(!scanning){const uint32_t result=nflstreet_channel_callbacks_event(c->callbacks,c->channels,actor,command,key);
  if(!c->callbacks->ok)r->ok=0;return result;}
 if(r32(c,R13-0x6730u)!=0x80015d8cu){r->ok=0;return 0;}
 unsigned n=0;uint32_t result=1;
 switch(command){
 case 0xc000:case 0xc00e:case 0xc00f:case 0xc014:case 0xc015:case 0xc016:case 0xc017:
 case 0xc01a:case 0xc01b:case 0xc01e:case 0xc01f:case 0xc021:case 0xc022:case 0xc023:case 0xd004:n=1;break;
 case 0xc009:case 0xc00d:case 0xc011:n=2;break;
 case 0xc010:case 0xc012:case 0xc019:n=3;break;
 default:result=(command>=0xc001&&command<=0xc00c)||command==0xc018||command==0xc01c||command==0xc01d||
   command==0xc020||(command>=0xd000&&command<=0xd003)||command==0xd005;break;}
 for(unsigned i=0;i<n&&r->ok;++i){uint32_t value;if(!nflstreet_clip_request_operand(r,&value))return 0;}return result;}
static void request_group(void *user,nflstreet_clip_requests *r,uint32_t function,uint32_t resources,uint32_t key,uint32_t set,uint32_t channels,uint32_t actor,uint32_t mode){
 C *c=user;if(function)nflstreet_channel_callbacks_group(c->callbacks,c->channels,function,resources,key,set,channels,actor,mode);
 if(!c->callbacks->ok)r->ok=0;}
void nflstreet_live_clips_init(C *c,nflstreet_actor_channels *channels,nflstreet_channel_callbacks *callbacks){
 memset(c,0,sizeof *c);c->channels=channels;c->callbacks=callbacks;c->requests.span=channels->span;c->requests.memory=channels->memory;
 c->requests.event=request_event;c->requests.group=request_group;c->requests.user=c;}
int nflstreet_live_clips_movement(C *c,uint32_t g){
 c->requests.ok=1;if(!c->callbacks->ok)return 0;
 const uint32_t m=r32(c,g+0x318),set=r32(c,g+0x31c),channels=r32(c,g+0x320),old=current_key(c,m);
 const uint8_t selector=r8(c,g+0x20e);uint32_t key=old,weight=0;int blocked=0;
 switch(selector){
 case 0:case 6:case 7:
  copy_input(c,g);
  if(selector&&r8(c,g+3)>=1&&r8(c,g+3)<=2){key=0x64;weight=0x80244d88u;break;}
  if(old==0x21||(old>=0x31&&old<=0x37)||(old>=0x39&&old<=0x3c)||old==0x49||old==0x54||old==0x64||
     (old>=0x6a&&old<=0x6c)||old==0xa0)key=0x4a;
  blocked=guard(c,g,m,old);weight=0x80244d70u;break;
 case 1:case 2:case 3:
  /* 8006510C and the 80014750/80014898 helper decisions are not part of
     clip_requests. Preserve their boundary; no speed-derived substitute. */
  if(r8(c,g+3)!=2)return c->requests.ok?-1:0;
  key=0x4a;w8(c,g+0x21e,1);w32(c,g+0x210,r32(c,g+0x200));w32(c,g+0x214,r32(c,g+0x204));w32(c,g+0x218,r32(c,g+0x204));
  blocked=suppression(c,g,m,old,key);weight=0x80244d7cu;break;
 case 4:
  w8(c,g+0x21e,selector);w32(c,g+0x214,r32(c,g+0x204));w32(c,g+0x210,r32(c,g+0x200));guard(c,g,m,old);return c->requests.ok;
 case 5:{
  const uint32_t status=r32(c,g+0x408);
  if(!((((old>=0x31&&old<=0x37)||(old>=0x39&&old<=0x3c)||(old>=0x6a&&old<=0x6c))&&status==4)||
       ((old==0x19||old==0x38||old==0x3d||old==0x4f)&&status==5)))key=0x4a;
  copy_input(c,g);blocked=guard(c,g,m,old);weight=0x80244d80u;break;}
 case 8:{
  const uint32_t h=r32(c,g+0x204),t=r32(c,g+0x208),d=distance(h,t);const float s=rf(c,g+0x200),v=rf(c,g+0x1ec),l=rf(c,R13-0x70c0u);
  if(d<0x2e38e3u||s==rf(c,0x80244d8cu)){key=0x4a;copy_input(c,g);w8(c,g+0x21e,1);}
  else if((d<0x56c16cu&&v>l)||(d>=0x56c16cu&&v>rf(c,R13-0x70bcu))){copy_input(c,g);w8(c,g+0x21e,1);}
  else{float magnitude=s;key=d<0x56c16cu?0xa0:0x4a;
   if(key==0xa0){const float floor=nflstreet_gk_divs(nflstreet_gk_muls(l,rf(c,0x80244d90u)),l);if(magnitude<floor)magnitude=floor;}
   w32(c,g+0x210,nflstreet_float_bits(magnitude));w32(c,g+0x214,h);w8(c,g+0x21e,selector);}
  blocked=suppression(c,g,m,old,key);weight=0x80244d94u;break;}
 case 9:copy_input(c,g);key=0x4a;weight=0x80244d84u;break;
 case 10:
  w8(c,g+0x21e,selector);w32(c,g+0x210,r32(c,g+0x200));w32(c,g+0x214,r32(c,g+0x204));w32(c,g+0x218,r32(c,g+0x204));
  key=0x4a;blocked=guard(c,g,m,old);weight=0x80244d74u;break;
 default:return c->requests.ok?-1:0;}
 if(!c->requests.ok)return 0;if(blocked<0)return -1;if(blocked||key==old)return 1;
 const float strength=rf(c,weight);if(!c->requests.ok)return 0;
 return nflstreet_clip_request_group(&c->requests,strength,m,set,channels,key,g)&&c->callbacks->ok;
}
