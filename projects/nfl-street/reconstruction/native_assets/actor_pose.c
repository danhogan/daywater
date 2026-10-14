#include "actor_pose.h"
#include "draw_engine.h"
#include <string.h>
typedef nflstreet_actor_pose_context context;
/* Constants and tables in the original's data. */
#define TABLE_MIRROR 0x8028cb30u   /* bone mirror map (words) */
#define DEFAULT_VECTOR 0x8028cba8u /* three floats, then three words (<<8) */
#define WEIGHT_SCALE 0x802606b0u   /* 4095.0: blend weight to 12-bit */
#define ROOT_ZERO 0x802606b4u
#define LERP_SCALE 0x80261748u     /* 2^24 */
#define CLIP_DEFAULT 0x80261180u
#define TWIST 0x8026e338u
/* 8015DBFC reads its constants at 80260658.., the mirrored 8015D584 at 80260638..:
   +0 default x/z, +8 (double 2^52+2^31) int conversion, +10 header unit,
   +14 unit test, +18 height threshold, +1C ball unit. */
#define PLAIN 0x80260658u
#define MIRROR 0x80260638u
static uint8_t *mem(context *c,uint32_t a,uint32_t n){
 if(a>=NFLSTREET_POSE_LOCAL&&a<NFLSTREET_POSE_LOCAL+NFLSTREET_POSE_LOCAL_BYTES)
  return (uint64_t)a-NFLSTREET_POSE_LOCAL+n<=NFLSTREET_POSE_LOCAL_BYTES?c->local+(a-NFLSTREET_POSE_LOCAL):NULL;
 return a?c->span(c->memory,a,n):NULL;}
static uint32_t r32(context *c,uint32_t a,int *ok){const uint8_t *p=mem(c,a,4);if(!p){*ok=0;return 0;}return nflstreet_rd32(p);}
static uint16_t r16(context *c,uint32_t a,int *ok){const uint8_t *p=mem(c,a,2);if(!p){*ok=0;return 0;}return nflstreet_rd16(p);}
static int16_t s16(context *c,uint32_t a,int *ok){return (int16_t)r16(c,a,ok);}
static uint8_t r8(context *c,uint32_t a,int *ok){const uint8_t *p=mem(c,a,1);if(!p){*ok=0;return 0;}return *p;}
static float rf(context *c,uint32_t a,int *ok){return nflstreet_as_float(r32(c,a,ok));}
static void w32(context *c,uint32_t a,uint32_t v,int *ok){uint8_t *p=mem(c,a,4);if(!p)*ok=0;else nflstreet_wr32(p,v);}
static void w16(context *c,uint32_t a,uint32_t v,int *ok){uint8_t *p=mem(c,a,2);if(!p)*ok=0;else nflstreet_wr16(p,v);}
static void wf(context *c,uint32_t a,float f,int *ok){w32(c,a,nflstreet_float_bits(f),ok);}
static int32_t asr(int32_t v,unsigned s){return v>=0?v>>s:-1-((-1-v)>>s);}
static uint32_t shl8(int32_t v){return (uint32_t)v<<8;}
/* 8017BC7C: forward byte copy. */
static void copy(context *c,uint32_t to,uint32_t from,uint32_t n,int *ok){
 for(uint32_t i=0;i<n&&*ok;++i){uint8_t v=r8(c,from+i,ok);uint8_t *p=mem(c,to+i,1);if(!p)*ok=0;else *p=v;}}
/* 8015D104(pose, data, descriptor, scatter): MSB-first fields, widths in the
   second byte of the descriptor's +14 pairs (destinations in the first when
   scattered); five fields per 64-bit read (80151ABC), the rest singly. */
static void decode(context *c,uint32_t pose,uint32_t data,uint32_t desc,int scatter,int *ok){
 const uint32_t count=r32(c,desc,ok),pairs=r32(c,desc+0x14,ok);uint64_t bit=0;if(!*ok)return;
 if(count>0x10000u){*ok=0;return;}
 for(uint32_t i=0;i<count&&*ok;){const uint32_t group=i+5<count?5u:1u;unsigned width[5],total=0;
  for(unsigned k=0;k<group;++k){width[k]=r8(c,pairs+2*(i+k)+1,ok);if(width[k]>16)*ok=0;total+=width[k];}
  if(total>64)*ok=0;if(!*ok)return;
  int16_t value[5];for(unsigned k=0;k<group;++k){uint32_t v=0;
   for(unsigned b=0;b<width[k];++b,++bit){uint8_t byte=r8(c,data+(uint32_t)(bit>>3),ok);v=v<<1|((byte>>(7-(bit&7)))&1u);}
   value[k]=(int16_t)(uint16_t)(v<<(16-width[k]));}
  for(unsigned k=group;k--;){const uint32_t at=scatter?r8(c,pairs+2*(i+k),ok):i+k;w16(c,pose+2*at,(uint16_t)value[k],ok);}
  i+=group;}}
/* 801667CC(to, from, n, base, scale). */
static void dequantize(context *c,uint32_t to,uint32_t from,int32_t n,uint32_t base,uint32_t scale,int *ok){
 for(int32_t i=0;i<n&&*ok;++i){int32_t v=(int32_t)((uint32_t)s16(c,from+2*i,ok)*(uint32_t)s16(c,scale+2*i,ok));
  w16(c,to+2*i,(uint16_t)(r16(c,base+2*i,ok)+(uint32_t)asr(v,15)),ok);}}
/* 80166730(to, a, b, weight, n): b + ((a-b)*weight >> 12). */
static void mix(context *c,uint32_t to,uint32_t a,uint32_t b,int32_t weight,int32_t n,int *ok){
 for(int32_t i=0;i<n&&*ok;++i){uint16_t bv=r16(c,b+2*i,ok),av=r16(c,a+2*i,ok);int32_t d=(int16_t)(uint16_t)(av-bv);
  w16(c,to+2*i,(uint16_t)((uint32_t)(int16_t)asr((int32_t)((uint32_t)d*(uint32_t)weight),12)+bv),ok);}}
/* 80166770(to, a, b, weight, n, base, scale): mix with a dequantized. */
static void mix_quantized(context *c,uint32_t to,uint32_t a,uint32_t b,int32_t weight,int32_t n,uint32_t base,uint32_t scale,int *ok){
 for(int32_t i=0;i<n&&*ok;++i){int32_t q=asr((int32_t)((uint32_t)s16(c,a+2*i,ok)*(uint32_t)s16(c,scale+2*i,ok)),15)+s16(c,base+2*i,ok);
  uint16_t bv=r16(c,b+2*i,ok);int32_t d=(int16_t)(uint16_t)((uint32_t)(int16_t)q-bv);
  w16(c,to+2*i,(uint16_t)((uint32_t)(int16_t)asr((int32_t)((uint32_t)d*(uint32_t)weight),12)+bv),ok);}}
/* 8015D3F0(value, bone, axis, descriptor, arrays): dequantize when compressed. */
static int16_t component(context *c,int32_t v,uint32_t bone,uint32_t axis,uint32_t desc,uint32_t arrays,int *ok){
 if(desc){const uint32_t at=2*(axis+bone*3);v=asr((int32_t)((uint32_t)v*(uint32_t)s16(c,r32(c,arrays+4,ok)+at,ok)),15)+s16(c,r32(c,arrays,ok)+at,ok);}
 return (int16_t)v;}
/* 8015CFA0: the frame header (bone count, flags, four halfwords, two
   optional three-halfword blocks); returns the components' address. */
typedef struct {uint8_t bones,flags;uint16_t h[4],a[3],b[3];} header;
static uint32_t parse(context *c,uint32_t frame,header *h,int *ok){
 const uint8_t first=r8(c,frame,ok);uint32_t at=frame+10;memset(h,0,sizeof *h);
 h->bones=first&0x3f;h->flags=r8(c,frame+1,ok);for(unsigned i=0;i<4;++i)h->h[i]=r16(c,frame+2+2*i,ok);
 if(first&0x40){for(unsigned i=0;i<3;++i)h->a[i]=r16(c,at+2*i,ok);at+=6;}
 if(first&0x80){for(unsigned i=0;i<3;++i)h->b[i]=r16(c,at+2*i,ok);at+=6;}
 return at;}
/* int -> double -> single -> times unit, as fsub/frsp/fmuls. */
static float unit(int32_t v,float k){return (float)v*k;}
/* The 0x80 header form: 18-bit x (h[1]) and z (h[3]) extended by flag bits. */
static void extended(const header *h,int32_t *x,int32_t *z){
 uint32_t vx=h->h[1]|((uint32_t)h->flags&3u)<<16,vz=h->h[3]|((uint32_t)h->flags<<13&0x30000u);
 if((h->flags&4)&&vx)vx|=0xfffc0000u;if((h->flags&0x20)&&vz)vz|=0xfffc0000u;*x=(int32_t)vx;*z=(int32_t)vz;}
/* 8015DB28 / 8015D49C: three ball angles (<<8) from a bone of the source. */
static void ball(context *c,uint32_t out,uint32_t src,uint32_t bone,uint32_t desc,int mirror,int *ok){
 const uint32_t mapped=r32(c,TABLE_MIRROR+4*bone,ok);
 if(desc){const uint32_t b=mirror?mapped:bone;for(unsigned k=0;k<3;++k)w32(c,out+4*k,shl8(component(c,s16(c,src+b*6+2*k,ok),b,k,desc,desc+(mirror?0xcu:4u),ok)),ok);return;}
 for(unsigned k=0;k<3;++k){int32_t v=s16(c,src+bone*6+2*k,ok);w32(c,out+4*k,shl8(mirror&&k?-v:v),ok);}}
/* 8015D42C: copy n bones through the mirror map, negating y and z. */
static void mirror_copy(context *c,uint32_t to,uint32_t from,uint32_t n,int *ok){
 for(uint32_t i=0;i<n&&*ok;++i){const uint32_t at=from+r32(c,TABLE_MIRROR+4*i,ok)*6u;
  w16(c,to+6*i,r16(c,at,ok),ok);w16(c,to+6*i+2,(uint16_t)-r16(c,at+2,ok),ok);w16(c,to+6*i+4,(uint16_t)-r16(c,at+4,ok),ok);}}
/* 8015DBFC (mirror 0) / 8015D584 (mirror 1): one entry's frame into the
   descriptor d (+8 root offset, +14 ball vector, +20 heading, +24 ball
   angles, +30 pose) and the entry e (+C..+14 root angles, +18/+1C root x/z
   under flag 4). */
static void sample(context *c,float scale,uint32_t d,uint32_t e,int32_t mode,uint32_t desc,int mirror,int *ok){
 const uint32_t k=mirror?MIRROR:PLAIN;const float u=rf(c,k+0x10,ok),one=rf(c,k+0x14,ok),threshold=rf(c,k+0x18,ok),u2=rf(c,k+0x1c,ok);
 const uint8_t flags=r8(c,e+8,ok);const uint32_t pose=r32(c,d+0x30,ok);header h;
 const uint32_t data=parse(c,r32(c,e+0x28,ok),&h,ok);if(!*ok)return;
 uint32_t src=data;if(desc){decode(c,pose,data,desc,mirror,ok);src=pose;}
 const uint32_t n=h.bones>0x40?0x40u:h.bones;int32_t x,z;
 if(h.flags&0x80)extended(&h,&x,&z);else{x=(int16_t)h.h[1];z=(int16_t)h.h[3];}
 if(flags&4){const float zero=rf(c,k,ok);wf(c,d+8,zero,ok);wf(c,d+0x10,zero,ok);wf(c,d+0xc,unit((int16_t)h.h[2],u),ok);
  wf(c,e+0x18,unit(z,u),ok);wf(c,e+0x1c,mirror?(-(float)x)*u:unit(x,u),ok);}
 else{wf(c,d+8,unit(x,u),ok);wf(c,d+0x10,unit(z,u),ok);wf(c,d+0xc,unit((int16_t)h.h[2],u),ok);w32(c,d+0x20,shl8((int16_t)h.h[0]),ok);}
 float y=rf(c,d+0xc,ok);if(!(scale==one)&&!(y==threshold||y<threshold))y=nflstreet_fmadds(y-threshold,scale,threshold);wf(c,d+0xc,y,ok);
 if(!mode)mode=flags&0x10?1:flags&0x20?2:0;
 const int block=mirror?(mode==1?2:mode==2?1:0):mode; /* the mirrored form swaps the two blocks and bones */
 if(block==1||block==2){const uint16_t *v=block==1?h.a:h.b;for(unsigned i=0;i<3;++i)wf(c,d+0x14+4*i,unit((int16_t)v[i],u2),ok);
  ball(c,d+0x24,src,block==1?0x19u:0x13u,desc,mirror,ok);}
 else{for(unsigned i=0;i<3;++i)w32(c,d+0x14+4*i,r32(c,DEFAULT_VECTOR+4*i,ok),ok);
  for(unsigned i=0;i<3;++i){int32_t v=(int32_t)r32(c,DEFAULT_VECTOR+12+4*i,ok);w32(c,d+0x24+4*i,shl8(mirror&&i?-v:v),ok);}}
 if(!desc){if(mirror)mirror_copy(c,pose,src,n,ok);else copy(c,pose,src,n*6u,ok);}
 if(mirror){wf(c,d+8,-rf(c,d+8,ok),ok);wf(c,d+0x14,-rf(c,d+0x14,ok),ok);}
 const int32_t rx=component(c,s16(c,src,ok),0,0,desc,desc+4,ok);w32(c,e+0xc,shl8(rx),ok);
 w32(c,e+0x10,shl8(mirror?-(int32_t)(int16_t)h.h[0]:(int16_t)h.h[0]),ok);
 const int32_t rz=component(c,s16(c,src+4,ok),0,2,desc,desc+4,ok);w32(c,e+0x14,shl8(mirror?-rz:rz),ok);}
int nflstreet_actor_pose_stream(context *c,uint32_t to,uint32_t from,uint32_t desc,int mirror){
 int ok=c&&c->span&&desc;if(!ok)return 0;const uint8_t first=r8(c,from,&ok);
 const uint32_t n=10u+((first&0x40)?6u:0u)+((first&0x80)?6u:0u),arrays=desc+(mirror?0xcu:4u);
 copy(c,to,from,n,&ok);decode(c,to+n,from+n,desc,mirror,&ok);
 dequantize(c,to+n,to+n,(int32_t)r32(c,desc,&ok),r32(c,arrays,&ok),r32(c,arrays+4,&ok),&ok);return ok;}
/* 80188758: b + ((a-b) * fctiwz(t*2^24) >> 24). */
static uint32_t lerp_angle(context *c,float t,uint32_t a,uint32_t b,int *ok){
 const int32_t w=nflstreet_fctiwz(t*rf(c,LERP_SCALE,ok));return b+(uint32_t)(int32_t)(((int64_t)(int32_t)(a-b)*w)>>24);}
/* The shortest 24-bit turn (8018879C per word, 80188834). */
static uint32_t turn(context *c,float t,uint32_t a,uint32_t b,int *ok){
 a&=0xffffffu;b&=0xffffffu;const int32_t d=(int32_t)(a-b);if(d>0x800000)b+=0x1000000u;else if(d< -0x800000)a+=0x1000000u;
 return lerp_angle(c,t,a,b,ok);}
/* 801DE858(t, out, a, b): b + (a-b)*t per component (fused). */
static void lerp3(context *c,float t,uint32_t out,uint32_t a,uint32_t b,int *ok){
 float bv[3],av[3];for(unsigned i=0;i<3;++i){bv[i]=rf(c,b+4*i,ok);av[i]=rf(c,a+4*i,ok);}
 for(unsigned i=0;i<3;++i)wf(c,out+4*i,nflstreet_fmadds(av[i]-bv[i],t,bv[i]),ok);}
int nflstreet_actor_pose_blend(context *c,uint32_t visual,uint32_t list){
 int ok=c&&c->span&&visual&&list;if(!ok)return 0;
 const uint32_t local=NFLSTREET_POSE_LOCAL,d=visual+0x2c;w32(c,local+0x30,local+0x38,&ok); /* the buffer pointer, set on entry */
 const uint32_t count=r16(c,list,&ok);if(!ok||!count)return ok;
 const float scale=rf(c,visual+0x1c,&ok);const int32_t n=(int32_t)r16(c,d+4,&ok)*3;uint32_t e=list+4;
 for(uint32_t i=0;i<count&&ok;++i){e=list+4+0x2c*i;
  const uint8_t flags=r8(c,e+8,&ok);const uint32_t desc=r32(c,e+0x24,&ok),arrays=desc+((flags&1)?0xcu:4u),target=i?local:d;
  sample(c,scale,target,e,(int32_t)r32(c,visual+0x20,&ok),desc,flags&1,&ok);if(!ok)break;
  const uint32_t pose=r32(c,d+0x30,&ok);
  if(!i){if(desc)dequantize(c,pose,pose,n,r32(c,arrays,&ok),r32(c,arrays+4,&ok),&ok);continue;}
  const float t=rf(c,e+4,&ok);
  lerp3(c,t,visual+0x34,local+8,visual+0x34,&ok);lerp3(c,t,visual+0x40,local+0x14,visual+0x40,&ok);
  for(unsigned k=0;k<3;++k){const uint32_t a=r32(c,local+0x24+4*k,&ok),b=r32(c,visual+0x50+4*k,&ok);w32(c,visual+0x50+4*k,turn(c,t,a,b,&ok),&ok);}
  w32(c,visual+0x4c,turn(c,t,r32(c,local+0x20,&ok),r32(c,visual+0x4c,&ok),&ok),&ok);
  const int32_t weight=nflstreet_fctiwz(t*rf(c,WEIGHT_SCALE,&ok));
  if(desc)mix_quantized(c,pose,local+0x38,pose,weight,n,r32(c,arrays,&ok),r32(c,arrays+4,&ok),&ok);
  else mix(c,pose,local+0x38,pose,weight,n,&ok);}
 if(!ok)return 0;
 const uint32_t mode=r32(c,visual+0x20,&ok);
 w16(c,visual+0x32,mode==0?((r8(c,e+8,&ok)&0x10)?0x18u:0x12u):mode==1?0x18u:0x12u,&ok);return ok;}
/* 80178440: the clip-set weight (+50) of the first of four 100-byte entries
   in use (+6) with the clip key (+4). */
static float clip_weight(context *c,uint32_t clips,uint32_t key,int *ok){
 for(unsigned i=0;i<4;++i,clips+=100)if(r16(c,clips+6,ok)&&r16(c,clips+4,ok)==key)return rf(c,clips+0x50,ok);
 return rf(c,CLIP_DEFAULT,ok);}
int nflstreet_actor_pose_root(context *c,uint32_t visual,uint32_t channels,unsigned count,uint32_t clips){
 int ok=c&&c->span&&visual;if(!ok)return 0;
 uint32_t angle[3]={0,0,0};const float zero=rf(c,ROOT_ZERO,&ok);float total=zero;
 for(unsigned i=0;i<count&&ok;++i){const uint32_t ch=channels+0x7cu*i;
  if(!r8(c,ch+1,&ok)||!(rf(c,ch+0x48,&ok)==zero))continue;
  const float w=clip_weight(c,clips,r16(c,ch+8,&ok),&ok)*rf(c,ch+0x30,&ok);total=total+w;const float t=w/total;
  for(unsigned k=0;k<3;++k)angle[k]=turn(c,t,r32(c,ch+0x10+4*k,&ok),angle[k],&ok);}
 if(!ok)return 0;
 w32(c,visual+0x4c,angle[1],&ok);const uint32_t pose=r32(c,visual+0x5c,&ok);
 w16(c,pose,(uint16_t)asr((int32_t)angle[0],8),&ok);w16(c,pose+2,0,&ok);w16(c,pose+4,(uint16_t)asr((int32_t)angle[2],8),&ok);
 const uint32_t last=(r32(c,visual+0x14,&ok)&0x80u)?0x1au:0x1du;
 for(uint32_t b=0x1a;ok&&b<r16(c,r32(c,visual+0x64,&ok)+6,&ok);){const uint32_t at=r32(c,visual+0x5c,&ok)+0x9cu+6u*(b-0x1a);
  ++b;w16(c,at+2,0,&ok);w16(c,at+4,0,&ok);w16(c,at,0,&ok);if(b>last)break;}
 return ok;}
int nflstreet_actor_pose_twist(context *c,uint32_t gameplay){
 int ok=c&&c->span&&gameplay;if(!ok)return 0;const uint32_t d=r32(c,gameplay+4,&ok)+0x2c;
 static const uint16_t at[4]={0x38,0x3e,0x44,0x4a};
 for(unsigned k=0;k<2&&ok;++k){const int32_t v=asr((int32_t)(r32(c,gameplay+0x12c+4*k,&ok)<<8),8);
  for(unsigned i=k?3:0;i<(k?4u:3u);++i){const uint32_t pose=r32(c,d+0x30,&ok);const int32_t f=(int32_t)r32(c,TWIST+4*i,&ok);
   const int32_t delta=asr((int32_t)(uint32_t)(((int64_t)f*v)>>24),8);w16(c,pose+at[i],(uint16_t)(r16(c,pose+at[i],&ok)+(uint32_t)delta),&ok);}}
 return ok;}
