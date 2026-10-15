#include "actor_channels.h"
#include "draw_engine.h"
#include "gekko_float.h"
#include <string.h>
typedef nflstreet_actor_channels C;
#define R13 0x803a9a40u
#define LIST 0x8032bf28u        /* 80176650's pose list: u16 count, u16 limit (0x18), 0x2C-byte entries */
#define RESOURCES 0x8032c34cu   /* 80175AF8's resource list for the group callbacks */
#define ACTOR_STACK 0x8032bf10u /* 80173ACC (index r13-0x579C) */
#define KEY_STACK 0x8032bf1cu   /* 80173AA8 (index r13-0x579B) */
#define READ_BUFFERS 0x8030b8e0u /* 801519A8's two 0xC0-byte ARAM read buffers (toggle r13-0x5968) */
#define CURVE_NONE 0x80178e78u
#define CURVE_LINEAR 0x80178e7cu
#define CURVE_DELAY 0x80178f04u
#define CURVE_PHASES 0x80178f28u
#define SINE 0x8039e100u        /* 8018889C's eight coefficients */
#define ATAN 0x8029194cu        /* 80188D98's nine coefficients */
#define WINDOW (NFLSTREET_CHANNEL_DIRECT+0x18u*NFLSTREET_CHANNEL_DIRECT_SLOT)
#ifdef NFLSTREET_CHANNELS_DEBUG
#include <stdio.h>
#define DEBUG_FAIL(a,n) fprintf(stderr,"channels: no memory at %08x (%u)\n",(unsigned)(a),(unsigned)(n))
#else
#define DEBUG_FAIL(a,n) ((void)0)
#endif
/* ---- memory ---- */
static uint8_t *view(void *m,uint32_t a,uint32_t n){C *c=m;
 if(a>=NFLSTREET_CHANNEL_DIRECT&&a<NFLSTREET_CHANNEL_DIRECT+sizeof c->host)
  return (uint64_t)a-NFLSTREET_CHANNEL_DIRECT+n<=sizeof c->host?c->host+(a-NFLSTREET_CHANNEL_DIRECT):NULL;
 return a?c->span(c->memory,a,n):NULL;}
static uint8_t *mem(C *c,uint32_t a,uint32_t n){uint8_t *p=view(c,a,n);if(!p){c->ok=0;DEBUG_FAIL(a,n);}return p;}
static uint32_t r32(C *c,uint32_t a){const uint8_t *p=mem(c,a,4);return p?nflstreet_rd32(p):0;}
static uint16_t r16(C *c,uint32_t a){const uint8_t *p=mem(c,a,2);return p?nflstreet_rd16(p):0;}
static uint8_t r8(C *c,uint32_t a){const uint8_t *p=mem(c,a,1);return p?*p:0;}
static float rf(C *c,uint32_t a){return nflstreet_as_float(r32(c,a));}
static double rd(C *c,uint32_t a){const uint8_t *p=mem(c,a,8);return p?nflstreet_gk_value((uint64_t)nflstreet_rd32(p)<<32|nflstreet_rd32(p+4)):0.0;}
static void w32(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,4);if(p)nflstreet_wr32(p,v);}
static void w16(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,2);if(p)nflstreet_wr16(p,v);}
static void w8(C *c,uint32_t a,uint32_t v){uint8_t *p=mem(c,a,1);if(p)*p=(uint8_t)v;}
static void wf(C *c,uint32_t a,float v){w32(c,a,nflstreet_float_bits(v));}
static void copy(C *c,uint32_t to,uint32_t from,uint32_t n){ /* 8017BC7C: forward byte copy */
 for(uint32_t i=0;i<n&&c->ok;++i){const uint8_t v=r8(c,from+i);w8(c,to+i,v);}}
static int32_t sext24(uint32_t v){return (int32_t)(v<<8)>>8;}
static int32_t asr(int32_t v,unsigned s){return v>=0?v>>s:-1-((-1-v)>>s);}
static void prepare(C *c){for(unsigned i=0;i<NFLSTREET_CHANNEL_ACTORS;++i){c->pose[i].span=view;c->pose[i].memory=c;}}
/* ---- angles and rotations ---- */
/* 80188758: b + ((a-b) * fctiwz(t*2^24) >> 24). */
static uint32_t lerp_angle(C *c,float t,uint32_t a,uint32_t b){
 const int32_t w=nflstreet_gk_fctiwz(nflstreet_gk_muls(t,rf(c,0x80261748u)));return b+(uint32_t)(int32_t)(((int64_t)(int32_t)(a-b)*w)>>24);}
/* 80188834 / 8018879C per word: the shortest 24-bit turn from b toward a. */
static uint32_t turn(C *c,float t,uint32_t a,uint32_t b){
 a&=0xffffffu;b&=0xffffffu;const int32_t d=(int32_t)(a-b);if(d>0x800000)b+=0x1000000u;else if(d< -0x800000)a+=0x1000000u;
 return lerp_angle(c,t,a,b);}
/* 80188A70 (cosine 0) / 80188AEC (cosine 1) with 8018889C (paired singles). */
static float sine(C *c,uint32_t angle,int cosine){
 const int32_t v=sext24(cosine?angle+0x400000u:angle);
 const float x=nflstreet_gk_convert((uint32_t)v^0x80000000u,rd(c,cosine?0x802617a8u:0x80261798u)),f2=nflstreet_gk_muls(x,rf(c,cosine?0x802617b0u:0x802617a0u));
 float k[8];for(unsigned i=0;i<8;++i)k[i]=rf(c,SINE+4*i);
 const float x2=nflstreet_gk_muls(f2,f2),x3=nflstreet_gk_muls(f2,x2),x4=nflstreet_gk_muls(x2,x2);
 const float a0=f2,a1=x3,b0=nflstreet_gk_muls(a0,x4),b1=nflstreet_gk_muls(a1,x4),e0=nflstreet_gk_muls(b0,x4),e1=nflstreet_gk_muls(b1,x4);
 const float g0=nflstreet_gk_muls(e0,x4),g1=nflstreet_gk_muls(e1,x4);
 float s0=nflstreet_gk_muls(k[0],a0),s1=nflstreet_gk_muls(k[1],a1);
 s0=nflstreet_gk_fmadds(k[2],b0,s0);s1=nflstreet_gk_fmadds(k[3],b1,s1);s0=nflstreet_gk_fmadds(k[4],e0,s0);s1=nflstreet_gk_fmadds(k[5],e1,s1);
 s0=nflstreet_gk_fmadds(k[6],g0,s0);s1=nflstreet_gk_fmadds(k[7],g1,s1);return nflstreet_gk_adds(s0,s1);}
/* 802165C8: correctly rounded sqrtf (fdlibm bitwise); negative -> NaN. */
static float root(float x){if(x<0.0f)return nflstreet_as_float(0x7fc00000u);return sqrtf(x);}
/* 80215844: acosf (fdlibm, single precision with fused forms). */
static float arccos(C *c,float x){
 const uint32_t bits=nflstreet_float_bits(x),ix=bits&0x7fffffffu;
#define K(o) rf(c,R13-(o))
 if(ix==0x3f800000u)return (int32_t)bits>0?K(0x6000):K(0x5ffc);
 if((int32_t)ix>0x3f800000){const float d=nflstreet_gk_subs(x,x);return nflstreet_gk_divs(d,d);}
 const float pS0=K(0x5fe0),pS1=K(0x5fe4),pS2=K(0x5fe8),pS3=K(0x5fec),pS4=K(0x5ff0),pS5=K(0x5ff4),qS1=K(0x5fd0),qS2=K(0x5fd4),qS3=K(0x5fd8),qS4=K(0x5fdc);
 if((int32_t)ix<=0x3effffff){if((int32_t)ix<=0x23000000)return K(0x5ff8);
  const float z=nflstreet_gk_muls(x,x);float p=nflstreet_gk_fmadds(z,pS5,pS4),q=nflstreet_gk_fmadds(z,qS4,qS3);
  p=nflstreet_gk_fmadds(z,p,pS3);q=nflstreet_gk_fmadds(z,q,qS2);p=nflstreet_gk_fmadds(z,p,pS2);q=nflstreet_gk_fmadds(z,q,qS1);p=nflstreet_gk_fmadds(z,p,pS1);
  const float qq=nflstreet_gk_fmadds(z,q,K(0x5fcc));p=nflstreet_gk_fmadds(z,p,pS0);
  const float r=nflstreet_gk_divs(nflstreet_gk_muls(z,p),qq);
  return nflstreet_gk_subs(K(0x5fc4),nflstreet_gk_subs(x,nflstreet_gk_subs(K(0x5fc8),nflstreet_gk_muls(x,r))));}
 const float one=K(0x5fcc);
 if((int32_t)bits<0){const float z=nflstreet_gk_muls(nflstreet_gk_adds(x,one),K(0x5fc0));
  float p=nflstreet_gk_fmadds(z,pS5,pS4),q=nflstreet_gk_fmadds(z,qS4,qS3);p=nflstreet_gk_fmadds(z,p,pS3);q=nflstreet_gk_fmadds(z,q,qS2);
  p=nflstreet_gk_fmadds(z,p,pS2);q=nflstreet_gk_fmadds(z,q,qS1);p=nflstreet_gk_fmadds(z,p,pS1);const float qq=nflstreet_gk_fmadds(z,q,one);
  p=nflstreet_gk_fmadds(z,p,pS0);const float pz=nflstreet_gk_muls(z,p),s=root(z),r=nflstreet_gk_divs(pz,qq);
  const float w=nflstreet_gk_fmsubs(r,s,K(0x5fc8)),a=nflstreet_gk_adds(s,w);return nflstreet_gk_subs(K(0x5fbc),nflstreet_gk_adds(a,a));}
 const float z=nflstreet_gk_muls(nflstreet_gk_subs(one,x),K(0x5fc0)),s=root(z);
 float p=nflstreet_gk_fmadds(z,pS5,pS4),q=nflstreet_gk_fmadds(z,qS4,qS3);p=nflstreet_gk_fmadds(z,p,pS3);q=nflstreet_gk_fmadds(z,q,qS2);
 p=nflstreet_gk_fmadds(z,p,pS2);q=nflstreet_gk_fmadds(z,q,qS1);p=nflstreet_gk_fmadds(z,p,pS1);
 const float df=nflstreet_as_float(nflstreet_float_bits(s)&0xfffff000u),cc=nflstreet_gk_divs(nflstreet_gk_subs(z,nflstreet_gk_muls(df,df)),nflstreet_gk_adds(s,df));
 const float qq=nflstreet_gk_fmadds(z,q,one);p=nflstreet_gk_fmadds(z,p,pS0);const float r=nflstreet_gk_divs(nflstreet_gk_muls(z,p),qq);
 const float w=nflstreet_gk_fmadds(r,s,cc),a=nflstreet_gk_adds(df,w);return nflstreet_gk_adds(a,a);}
/* 80216EF4 __kernel_sinf(x, y, iy). */
static float kernel_sin(C *c,float x,float y,int iy){
 const uint32_t ix=nflstreet_float_bits(x)&0x7fffffffu;if((int32_t)ix<=0x31ffffff&&nflstreet_gk_fctiwz(x)==0)return x;
 const float z=nflstreet_gk_muls(x,x),v=nflstreet_gk_muls(z,x);
 float r=nflstreet_gk_fmadds(z,K(0x5e84),K(0x5e80));r=nflstreet_gk_fmadds(z,r,K(0x5e7c));r=nflstreet_gk_fmadds(z,r,K(0x5e78));r=nflstreet_gk_fmadds(z,r,K(0x5e74));
 if(!iy)return nflstreet_gk_fmadds(v,nflstreet_gk_fmadds(z,r,K(0x5e70)),x);
 const float vr=nflstreet_gk_muls(v,r);float f=nflstreet_gk_fmsubs(y,K(0x5e6c),vr);const float s1=nflstreet_gk_muls(v,K(0x5e70));
 f=nflstreet_gk_fmsubs(z,f,y);f=nflstreet_gk_subs(f,s1);return nflstreet_gk_subs(x,f);}
/* 80216E0C __kernel_cosf(x, y). */
static float kernel_cos(C *c,float x,float y){
 const uint32_t ix=nflstreet_float_bits(x)&0x7fffffffu;if((int32_t)ix<=0x31ffffff&&nflstreet_gk_fctiwz(x)==0)return K(0x5ea8);
 const float z=nflstreet_gk_muls(x,x);float r=nflstreet_gk_fmadds(z,K(0x5ea4),K(0x5ea0));
 r=nflstreet_gk_fmadds(z,r,K(0x5e9c));r=nflstreet_gk_fmadds(z,r,K(0x5e98));r=nflstreet_gk_fmadds(z,r,K(0x5e94));r=nflstreet_gk_fmadds(z,r,K(0x5e90));
 const float zr=nflstreet_gk_muls(z,r);
 if((int32_t)ix<=0x3e999999){const float f=nflstreet_gk_fmsubs(z,zr,nflstreet_gk_muls(x,y));
  return nflstreet_gk_subs(K(0x5ea8),nflstreet_gk_fmsubs(z,K(0x5e8c),f));}
 const float qx=(int32_t)ix>0x3f480000?K(0x5e88):nflstreet_as_float(ix-0x01000000u);
 const float f=nflstreet_gk_fmsubs(z,zr,nflstreet_gk_muls(x,y)),hz=nflstreet_gk_fmsubs(z,K(0x5e8c),qx),a=nflstreet_gk_subs(K(0x5ea8),qx);
 return nflstreet_gk_subs(a,nflstreet_gk_subs(hz,f));}
/* 802171C4 __ieee754_rem_pio2f; the large-argument path (80217548) is not
   reconstructed (|x| > 0x43490F80 never reaches it from the slerp). */
static int rem_pio2(C *c,float x,float y[2]){
 const uint32_t hx=nflstreet_float_bits(x),ix=hx&0x7fffffffu;
 if((int32_t)ix<=0x3f490fd8){y[0]=x;y[1]=K(0x5e48);return 0;}
 if((int32_t)ix<=0x4016cbe3){const int pos=(int32_t)hx>0;float z=pos?nflstreet_gk_subs(x,K(0x5e44)):nflstreet_gk_adds(x,K(0x5e44)),t;
  if((hx&0x7ffffff0u)!=0x3fc90fd0u)t=K(0x5e40);else{z=pos?nflstreet_gk_subs(z,K(0x5e3c)):nflstreet_gk_adds(z,K(0x5e3c));t=K(0x5e38);}
  if(pos){y[0]=nflstreet_gk_subs(z,t);y[1]=nflstreet_gk_subs(nflstreet_gk_subs(z,y[0]),t);return 1;}
  y[0]=nflstreet_gk_adds(z,t);y[1]=nflstreet_gk_adds(nflstreet_gk_subs(z,y[0]),t);return -1;}
 if((int32_t)ix>0x43490f80){c->ok=0;y[0]=y[1]=0.0f;return 0;}
 const float t=nflstreet_as_float(ix),fnr=nflstreet_gk_fmadds(t,K(0x5e34),K(0x5e30));const int32_t n=nflstreet_gk_fctiwz(fnr);
 const float fn=nflstreet_gk_convert((uint32_t)n^0x80000000u,rd(c,R13-0x5e28u));
 float r=nflstreet_gk_subs(t,nflstreet_gk_muls(fn,K(0x5e44))),w=nflstreet_gk_muls(fn,K(0x5e40));
 if(n<32&&(hx&0x7fffff00u)!=r32(c,R13-0x41ecu+4u*(uint32_t)(n-1)))y[0]=nflstreet_gk_subs(r,w);
 else{const int32_t j=(int32_t)ix>>23;y[0]=nflstreet_gk_subs(r,w);
  if(j-(int32_t)((nflstreet_float_bits(y[0])>>23)&0xffu)>8){const float t2=r;w=nflstreet_gk_muls(fn,K(0x5e3c));r=nflstreet_gk_subs(r,w);
   w=nflstreet_gk_fmsubs(fn,K(0x5e38),nflstreet_gk_subs(nflstreet_gk_subs(t2,r),w));y[0]=nflstreet_gk_subs(r,w);
   if(j-(int32_t)((nflstreet_float_bits(y[0])>>23)&0xffu)>25){const float t3=r;w=nflstreet_gk_muls(fn,K(0x5e20));r=nflstreet_gk_subs(r,w);
    w=nflstreet_gk_fmsubs(fn,K(0x5e1c),nflstreet_gk_subs(nflstreet_gk_subs(t3,r),w));y[0]=nflstreet_gk_subs(r,w);}}}
 y[1]=nflstreet_gk_subs(nflstreet_gk_subs(r,y[0]),w);
 if((int32_t)hx<0){y[0]=-y[0];y[1]=-y[1];return -n;}
 return n;}
/* 80216B94 sinf. */
static float sinf_(C *c,float x){
 const uint32_t ix=nflstreet_float_bits(x)&0x7fffffffu;
 if((int32_t)ix<=0x3f490fd8)return kernel_sin(c,x,K(0x5ec0),0);
 if((int32_t)ix>0x7f7fffff)return nflstreet_gk_subs(x,x);
 float y[2]={0.0f,0.0f};const int n=rem_pio2(c,x,y);if(!c->ok)return 0.0f;
 switch(n&3){case 0:return kernel_sin(c,y[0],y[1],1);case 1:return kernel_cos(c,y[0],y[1]);case 2:return -kernel_sin(c,y[0],y[1],1);default:return -kernel_cos(c,y[0],y[1]);}}
#undef K
/* 801A3794: the quaternion of three half angles (x, y, z words, order as called). */
static void quaternion(C *c,float q[4],uint32_t a1,uint32_t a2,uint32_t a0){
 const uint32_t ang[3]={(uint32_t)((int32_t)a0>>1),(uint32_t)((int32_t)a1>>1),(uint32_t)((int32_t)a2>>1)};float s[3],k[3];
 for(unsigned i=0;i<3;++i){s[i]=sine(c,ang[i],0);k[i]=sine(c,ang[i],1);}
 const float f6=nflstreet_gk_muls(k[2],k[1]),f13=nflstreet_gk_muls(s[2],k[1]),f0=nflstreet_gk_muls(s[2],s[1]),f9=nflstreet_gk_muls(k[2],s[1]);
 const float f12=nflstreet_gk_muls(s[0],f0),f7=nflstreet_gk_muls(s[0],f13),f8=nflstreet_gk_muls(s[0],f9),g0=nflstreet_gk_muls(k[0],f0);
 q[3]=nflstreet_gk_fmsubs(k[0],f6,f12);q[2]=nflstreet_gk_fmsubs(k[0],f13,f8);q[0]=nflstreet_gk_fmadds(s[0],f6,g0);q[1]=nflstreet_gk_fmadds(k[0],f9,f7);}
/* 801A2CE0 with 80188EAC: the inverse length. */
static float inverse_length(C *c,const float q[4]){
 const float f=nflstreet_gk_fmadds(q[3],q[3],nflstreet_gk_fmadds(q[2],q[2],nflstreet_gk_fmadds(q[0],q[0],nflstreet_gk_muls(q[1],q[1])))),s=root(f);
 return s<=rf(c,0x802617d8u)?rf(c,0x802617e0u):nflstreet_gk_divs(rf(c,0x802617dcu),s);}
/* 801A383C(t, out, p, q): slerp from q (weight 1-t) toward p (weight t). */
static void slerp(C *c,float t,float out[4],const float p[4],const float q[4]){
 const float dot=nflstreet_gk_fmadds(q[3],p[3],nflstreet_gk_fmadds(q[2],p[2],nflstreet_gk_fmadds(q[0],p[0],nflstreet_gk_muls(q[1],p[1]))));
 float cosv=nflstreet_gk_muls(dot,nflstreet_gk_muls(inverse_length(c,q),inverse_length(c,p))),l[4];
 if(cosv>=rf(c,0x80263514u))for(unsigned i=0;i<4;++i)l[i]=p[i];else{cosv=-cosv;for(unsigned i=0;i<4;++i)l[i]=-p[i];}
 float k0,k1;const float one=rf(c,0x8026351cu);
 if(cosv>rf(c,0x80263518u)){k1=t;k0=nflstreet_gk_subs(one,t);}
 else{const float th=arccos(c,cosv),inv=nflstreet_gk_divs(one,sinf_(c,th));
  k0=nflstreet_gk_muls(sinf_(c,nflstreet_gk_muls(th,nflstreet_gk_subs(one,t))),inv);k1=nflstreet_gk_muls(sinf_(c,nflstreet_gk_muls(th,t)),inv);}
 for(unsigned i=0;i<4;++i)out[i]=nflstreet_gk_fmadds(k0,q[i],nflstreet_gk_muls(k1,l[i]));}
/* 80188D98: atan2 as a 24-bit angle. */
static uint32_t angle_of(C *c,float y,float x){
 const float zero=rf(c,0x802617d0u),ay=fabsf(y),ax=fabsf(x);float f7=nflstreet_gk_divs(nflstreet_gk_subs(ay,ax),nflstreet_gk_adds(ay,ax));uint32_t base=0;
 if(y>=zero){if(!(x>=zero)){base=0x400000u;f7=-f7;}}
 else if(x<zero)base=0x800000u;else{base=0xffc00000u;f7=-f7;}
 float k[9];for(unsigned i=0;i<9;++i)k[i]=rf(c,ATAN+4*i);
 const float x2=nflstreet_gk_muls(f7,f7);float x3=nflstreet_gk_muls(f7,x2),p=nflstreet_gk_muls(x3,x2);
 float s=nflstreet_gk_fmadds(f7,k[0],nflstreet_gk_muls(x3,k[1]));s=nflstreet_gk_fmadds(p,k[2],s);
 for(unsigned i=3;i<8;++i){p=nflstreet_gk_muls(p,x2);s=nflstreet_gk_fmadds(p,k[i],s);}
 s=nflstreet_gk_muls(nflstreet_gk_adds(s,k[8]),rf(c,0x802617d4u));
 return (uint32_t)sext24((uint32_t)nflstreet_gk_fctiwz(s)+base);}
/* 801A3580: three 24-bit angles from a quaternion. */
static void euler(C *c,uint32_t out,const float q[4]){
 const float x=q[0],y=q[1],z=q[2],w=q[3],one=rf(c,0x80263500u);
 const float zz=nflstreet_gk_muls(z,z),xx=nflstreet_gk_muls(x,x),s=nflstreet_gk_fmadds(x,y,nflstreet_gk_muls(w,z)),f25=nflstreet_gk_adds(s,s);
 const float f26=root(nflstreet_gk_subs(one,nflstreet_gk_muls(f25,f25))),yy=nflstreet_gk_muls(y,y);float a0y,a0x,a1y,a1x;
 if(fabsf(f26)<=rf(c,0x80263504u)){const float g=nflstreet_gk_fmsubs(y,z,nflstreet_gk_muls(w,x)),d=nflstreet_gk_subs(xx,zz);
  a0y=nflstreet_gk_muls(g,rf(c,0x8026350cu));a0x=nflstreet_gk_subs(one,nflstreet_gk_adds(d,d));a1y=rf(c,0x80263510u);a1x=one;}
 else{float inv=f26<=rf(c,0x802617e8u)?rf(c,0x802617e4u):nflstreet_gk_divs(rf(c,0x802617ecu),f26);const float k=rf(c,0x80263508u); /* 80188EF8 */
  inv=nflstreet_gk_adds(inv,inv);
  const float yz=nflstreet_gk_muls(y,z),xz=nflstreet_gk_muls(x,z),f0=nflstreet_gk_subs(nflstreet_gk_subs(k,yy),zz),f13=nflstreet_gk_subs(nflstreet_gk_subs(k,xx),zz);
  const float f11=nflstreet_gk_fmsubs(w,y,xz),f12=nflstreet_gk_fmsubs(w,x,yz);
  a1x=nflstreet_gk_muls(f0,inv);a0y=nflstreet_gk_muls(f12,inv);a0x=nflstreet_gk_muls(f13,inv);a1y=nflstreet_gk_muls(f11,inv);}
 w32(c,out,angle_of(c,a0y,a0x));w32(c,out+4,angle_of(c,a1y,a1x));w32(c,out+8,angle_of(c,f25,f26));}
/* 8017650C(t, out, a, b): b toward a by t, as the shortest turn per word
   (8018879C) when the three differences sum to at most a quarter turn,
   else through quaternions. */
static void blend_roots(C *c,float t,uint32_t out,uint32_t a,const uint32_t b[3]){
 uint32_t av[3];int32_t sum=0;for(unsigned k=0;k<3;++k){av[k]=r32(c,a+4*k);const int32_t d=sext24(av[k]-b[k]);sum+=d<0?-d:d;}
 if(sum<=0x400000){for(unsigned k=0;k<3;++k)w32(c,out+4*k,turn(c,t,av[k],b[k]));return;}
 float q1[4],q2[4];quaternion(c,q1,av[1],av[2],av[0]);quaternion(c,q2,b[1],b[2],b[0]);slerp(c,t,q1,q1,q2);euler(c,out,q1);}
/* ---- curves (8017924C) ---- */
static void curve(C *c,float dt,uint32_t b){
 const uint32_t table=r32(c,R13-0x6714u);if(!table||!c->ok)return;const uint32_t fn=r32(c,table+4*r32(c,b+8));
 if(fn==CURVE_NONE)return;
 if(fn==CURVE_LINEAR){const float cur=rf(c,b),tgt=rf(c,b+4);if(cur==tgt)return;const float init=rf(c,b+0xc);
  const float n=nflstreet_gk_convert(r32(c,b+0x10),rd(c,0x802611c0u)),v=nflstreet_gk_fmadds(nflstreet_gk_divs(nflstreet_gk_subs(tgt,init),n),dt,cur);wf(c,b,v);
  if(init>=tgt){if(!(v>=tgt))wf(c,b,tgt);}else if(!(v<=tgt))wf(c,b,tgt);return;}
 if(fn==CURVE_DELAY){const float v=nflstreet_gk_subs(rf(c,b+0xc),dt),k=rf(c,0x802611c8u);wf(c,b+0xc,v);if(!(v>k))wf(c,b,k);return;}
 if(fn==CURVE_PHASES){const double f6=rd(c,0x802611d8u),f8=rd(c,0x802611d0u);const float f7=rf(c,0x802611ccu);float f1=dt;
  for(unsigned guard=0;c->ok;++guard){if(guard>0x10000){c->ok=0;return;}
   const int8_t st=(int8_t)r8(c,b+0x15);float f9=f1;int r10=0;uint32_t r0=0;int jump=0;f1=rf(c,0x802611ccu);
   if(st==1){const float f0=nflstreet_gk_subs(rf(c,b+0x10),f9);wf(c,b+0x10,f0);if(!(f0>=f1)){r0=r8(c,b+0x14);f1=-f0;w8(c,b+0x15,2);jump=1;}}
   else if(st==0){r10=1;const float f0=nflstreet_gk_subs(rf(c,b+0x10),f9);wf(c,b+0x10,f0);if(!(f0>=f1)){r0=r16(c,b+0x16);f1=-f0;w8(c,b+0x15,1);jump=1;}}
   else if(st==2)r10=1;
   if(jump){f9=nflstreet_gk_subs(f9,f1);wf(c,b+0x10,nflstreet_gk_convert(r0,f8));}
   if(r10==1){float f10,f12;if(r8(c,b+0x15)==2){f10=rf(c,b+4);f12=rf(c,b+0xc);}else{f10=rf(c,b+0xc);f12=rf(c,b+4);}
    const float f11=nflstreet_gk_convert((uint32_t)r8(c,b+0x14)^0x80000000u,f6);
    if(f11!=f7)wf(c,b,nflstreet_gk_fmadds(nflstreet_gk_divs(nflstreet_gk_subs(f12,f10),f11),f9,rf(c,b)));
    const float f0=rf(c,b);if(f10>=f12){if(!(f0>f12))wf(c,b,f12);}else if(!(f0<f12))wf(c,b,f12);}
   if(!(f1>f7))return;}
  return;}
 c->ok=0;} /* a curve function outside the four: not reconstructed */
/* ---- banks and groups ---- */
static uint32_t bank_header(C *c,uint32_t i){ /* 80175F48 */
 if(i>=r32(c,R13-0x4f70u))return 0;const uint32_t e=r32(c,R13-0x4f6cu)+0x1cu*i;return r16(c,e+0x14)==0x8000u?0:r32(c,e+0xc);}
static uint32_t bank_cache(C *c,uint32_t i){return i>=r32(c,R13-0x4f70u)?0:r32(c,r32(c,R13-0x4f6cu)+0x1cu*i+0x18);} /* 80175F84 */
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
int nflstreet_actor_channels_manager(C *c,float dt,uint32_t actor){
 if(!c||!c->span||!actor)return 0;c->ok=1;prepare(c);const uint32_t m=actor+0x9bc,set=actor+0x4d8,ch=actor+0x4e4;
 const float f30=rf(c,0x8026117cu);
 for(unsigned k=0;k<=3&&c->ok;++k){const uint32_t rec=m+100u*k;if(!r16(c,rec+6))break;
  const float t=nflstreet_gk_subs(rf(c,rec+0xc),dt),zero=rf(c,0x80261178u);wf(c,rec+0xc,t);if(t>=zero)continue;
  wf(c,rec+0xc,zero);curve(c,dt,rec+0x50);
  if(rf(c,rec+0x50)<=f30){retire(c,m,set,ch,k,actor);continue;}
  if(r16(c,rec+8)&1)group_call(c,r16(c,set+2),r16(c,rec+4),actor,2,set,ch);
  if(playing(c,set,ch,r16(c,rec+4)))continue;
  if(r16(c,rec+6)==3)group_call(c,r16(c,set+2),r16(c,rec+4),actor,1,set,ch);else retire(c,m,set,ch,k,actor);}
 if(c->ok)compact(c,m);return c->ok;}
/* ---- clip programs ---- */
static uint32_t operand(C *c,uint32_t s){ /* 80176DD0 */
 const uint32_t i=r16(c,s+0x18),prog=r32(c,s)+4;const uint16_t w=r16(c,prog+2*i);w16(c,s+0x18,i+1);
 uint32_t v=w&0xfffu;if(w&0x800)v|=0xf000u;if(!(w&0x1000))return v;
 const uint16_t x=r16(c,prog+((2u*(i+1u))&0x1fffeu));w16(c,s+0x18,i+2);return (v<<16)+x;}
int nflstreet_actor_channels_operand(C *c,uint32_t *value){
 if(!c||!value)return 0;const int was=c->ok;c->ok=1;*value=operand(c,r32(c,R13-0x4f58u));const int ok=c->ok;c->ok=was&&ok;return ok;}
static uint32_t event(C *c,uint32_t s,uint32_t command){ /* 80173A24 */
 w32(c,R13-0x4f58u,s);const int32_t sel=(int8_t)r8(c,R13-0x579cu);
 const uint32_t key=r32(c,KEY_STACK+4u*(uint32_t)sel),actor=r32(c,ACTOR_STACK+4u*(uint32_t)sel);if(!c->ok)return 0;
 return c->event?c->event(c->user,c,actor,command,key):1u;}
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
static int timing(C *c,float dt,uint32_t s){
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
int nflstreet_actor_channels_step(C *c,float dt,uint32_t actor){
 if(!c||!c->span||!actor)return 0;c->ok=1;prepare(c);const uint32_t set=actor+0x4d8;uint32_t ch=actor+0x4e4;
 const float f30=rf(c,0x802610c8u);
 for(uint32_t i=0;i<r16(c,set+4)&&c->ok;++i,ch+=0x7c){
  if(r8(c,ch+1)==1)w8(c,ch+1,0);if(!r8(c,ch+1))continue;
  const float rem=nflstreet_gk_subs(rf(c,ch+0x48),dt),zero=rf(c,0x802610c4u);wf(c,ch+0x48,rem);if(rem>=zero)continue;
  wf(c,ch+0x48,zero);const uint32_t mode=r32(c,ch+0x38);curve(c,dt,ch+0x30);
  if(!(rf(c,ch+0x30)>=f30)&&mode)w8(c,ch+1,1);
  if(!r8(c,ch+1))continue;
  push(c,R13-0x579cu,ACTOR_STACK,actor);push(c,R13-0x579bu,KEY_STACK,r16(c,ch+8)); /* 80174374 */
  const int end=timing(c,dt,ch+0x4c);
  w8(c,R13-0x579bu,r8(c,R13-0x579bu)-1u);w8(c,R13-0x579cu,r8(c,R13-0x579cu)-1u);
  if(end==1)w8(c,ch+1,1);}
 return c->ok;}
/* ---- pose list ---- */
/* 801749E8 (with 80179B60 for streamed banks): the frame record. */
static uint32_t frame_of(C *c,uint32_t bank,uint32_t index,uint32_t frame,uint32_t variant,unsigned slot,unsigned actor){
 const uint32_t flags=r16(c,bank+6);uint32_t w;
 if(flags&4){w=r32(c,r32(c,variant*0xcu+r32(c,bank+0x1c)+4)+4*frame);if(flags&0x200u){c->ok=0;return 0;}return w;} /* flag 4: no shipped bank, its stream path not reconstructed */
 if(frame>=r16(c,bank+4))frame=0;
 w=r32(c,r32(c,bank+0x10)+4*frame);if(!(flags&0x200u)||!c->ok)return w;
 const uint32_t cache=bank_cache(c,index);if(!c->aram){c->ok=0;return 0;}
 if(c->direct){ /* the service: decode the file's frame into host memory, cache untouched */
  size_t avail;const uint8_t *src=nflstreet_anim_aram_frame(c->aram,index,frame,&avail);if(!src||slot>=0x18u){c->ok=0;return 0;}
  const uint32_t to=NFLSTREET_CHANNEL_DIRECT+NFLSTREET_CHANNEL_DIRECT_SLOT*slot;uint8_t *win=c->host+(WINDOW-NFLSTREET_CHANNEL_DIRECT);
  memset(win,0,0xc0);memcpy(win,src,avail<0xc0?avail:0xc0);
  if(flags&1){if(!nflstreet_actor_pose_stream(&c->pose[actor],to,WINDOW,r32(c,bank+0x14),0))c->ok=0;}
  else copy(c,to,WINDOW,r16(c,cache+2));
  return to;}
 const uint32_t records=r32(c,cache+4);uint32_t rec;
 if((w&0xf0000000u)==0xf0000000u){rec=records+(w&0x0fffffffu)*0x14u; /* 80179830: a hit, moved to the LRU tail (8017972C) */
  const uint32_t list=cache+0xc;
  if(rec==r32(c,list+4)){const uint32_t nx=r32(c,rec+4);w32(c,list+4,nx);w32(c,nx,0);}
  else if(rec==r32(c,list+8)){const uint32_t pv=r32(c,rec);w32(c,list+8,pv);w32(c,pv+4,0);}
  else{const uint32_t nx=r32(c,rec+4),pv=r32(c,rec);w32(c,pv+4,nx);w32(c,nx,pv);}
  const uint32_t tail=r32(c,cache+0x14);w32(c,rec,tail);w32(c,tail+4,rec);w32(c,cache+0x14,rec);w32(c,rec+4,0);
  return r32(c,cache+8)+(w&0x0fffffffu)*r16(c,cache+2);}
 /* 80179A14: evict the LRU head (801798CC), restore its frame word (80179900) */
 rec=r32(c,cache+0x10);{const uint32_t nx=r32(c,rec+4);w32(c,cache+0x10,nx);w32(c,nx,0);const uint32_t tail=r32(c,cache+0x14);
  w32(c,rec,tail);w32(c,tail+4,rec);w32(c,cache+0x14,rec);w32(c,rec+4,0);}
 if(r8(c,rec+0xf)==1){const uint32_t old=bank_header(c,r8(c,rec+0xe));if(!old){c->ok=0;return 0;}
  if(r16(c,old+6)&4)w32(c,r32(c,r16(c,rec+0x12)*0xcu+r32(c,old+0x1c)+4)+4u*r16(c,rec+0xc),r32(c,rec+8));
  else w32(c,r32(c,old+0x10)+4u*r16(c,rec+0xc),r32(c,rec+8));}
 w32(c,rec+8,w);w8(c,rec+0xe,index);w8(c,rec+0xf,1);w16(c,rec+0xc,frame);w32(c,rec+0x10,0xffffu); /* 801749E8 passes 0xFFFF without flag 4 */
 const uint32_t k=(uint32_t)((int32_t)((rec-records)*0xcccccccdu)>>2),slotw=k|0xf0000000u,to=r32(c,cache+8)+k*r16(c,cache+2);
 w32(c,r32(c,bank+0x10)+4*frame,slotw);
 /* 801519A8: the 0xC0-byte ARAM read (8017BB78), then decode or copy */
 const uint32_t t=r8(c,R13-0x5968u),buf=READ_BUFFERS+t*0xc0u;w8(c,R13-0x5968u,t==0);
 uint8_t *dst=mem(c,buf,0xc0);if(!dst)return 0;nflstreet_anim_aram_read(c->aram,w&~0x1fu,dst,0xc0);
 if(flags&1){if(!nflstreet_actor_pose_stream(&c->pose[actor],to,buf+(w&0x1fu),r32(c,bank+0x14),0))c->ok=0;}
 else copy(c,to,buf+(w&0x1fu),r16(c,cache+2));
 return to;}
static uint32_t descriptor(C *c,uint32_t bank,uint32_t variant){ /* 80176610 */
 const uint32_t f=r16(c,bank+6);if(!(f&1)||(f&0x200u))return 0;return (f&4)?r32(c,variant*0xcu+r32(c,bank+0x1c)+8):r32(c,bank+0x14);}
static uint32_t clip_entry(C *c,uint32_t clips,uint32_t key){ /* 80178404: the entry + 4 */
 for(unsigned i=0;i<4;++i,clips+=100)if(r16(c,clips+6)&&r16(c,clips+4)==key)return clips+4;return 0;}
/* 80176650(set, channels, clips): the pose list at 8032BF28. */
static void build_list(C *c,uint32_t set,uint32_t ch,uint32_t clips,unsigned actor){
 w16(c,LIST+2,0x18);uint32_t n=0;const float zero=rf(c,0x802610e0u);
 for(uint32_t i=0;i<r16(c,set+4)&&c->ok;++i,ch+=0x7c){
  if(n>=r16(c,LIST+2))break;
  if(!r8(c,ch+1)||!(rf(c,ch+0x48)==zero))continue;
  const uint32_t ce=clip_entry(c,clips,r16(c,ch+8));if(!ce){c->ok=0;return;} /* the original would read address 0x4C */
  const float w=nflstreet_gk_muls(rf(c,ce+0x4c),rf(c,ch+0x30));
  for(unsigned pass=0;pass<2&&c->ok;++pass){
   if(pass&&n>=r16(c,LIST+2))break;
   const uint32_t e=LIST+4+0x2cu*n;wf(c,e,w);
   const float k=pass?nflstreet_gk_muls(w,nflstreet_gk_subs(rf(c,0x802610e4u),rf(c,ch+0x60))):nflstreet_gk_muls(w,rf(c,ch+0x60));wf(c,e+4,k);
   if(k==zero)continue;const uint32_t bank=bank_header(c,r8(c,ch+6));if(!bank)continue;
   uint32_t variant=r16(c,ch+0xa);if(variant==0xffffu)variant=(r16(c,bank+6)&4)?r16(c,ce+6):0xffffu;
   const uint32_t frame=r16(c,ch+(pass?0x5c:0x5e));
   w32(c,e+0x28,frame_of(c,bank,r8(c,ch+6),frame,variant&0xffffu,n,actor));w32(c,e+0x24,descriptor(c,bank,variant));
   w16(c,e+0x20,r8(c,ch+6));w16(c,e+0x22,frame);w8(c,e+9,i);w8(c,e+8,r8(c,ch+7));n=(n+1)&0xffffu;}}
 if(!c->ok)return;
 float sum=rf(c,LIST+8);w16(c,LIST,n);wf(c,LIST+8,rf(c,0x802610e4u));
 if(n>1){const float z=rf(c,0x802610e0u);for(uint32_t k=1;k<n;++k){const uint32_t at=LIST+8+0x2cu*k;const float v=rf(c,at);sum=nflstreet_gk_adds(sum,v);
   wf(c,at,sum==z?z:nflstreet_gk_divs(v,sum));}}}
/* 80176904: the channel roots and the heading travel. */
static int roots(C *c,uint32_t set,uint32_t ch,uint32_t out,uint32_t key,uint32_t flag){
 const float zero=rf(c,0x802610e8u);float total=zero;uint32_t acc=0;int any=0;
 for(uint32_t i=0;i<r16(c,LIST)&&c->ok;++i){const uint32_t e=LIST+4+0x2cu*i,idx=r8(c,e+9),b=ch+0x7cu*idx;
  if(i+1<r16(c,LIST)&&r8(c,e+0x2c+9)==idx){uint32_t loc[3]={0,0,0};
   if(!(r8(c,b+0x6a)&0x10))for(unsigned k=0;k<3;++k)loc[k]=r32(c,e+0x2c+0xc+4*k);
   blend_roots(c,rf(c,b+0x60),b+0x10,e+0xc,loc);++i;}
  else for(unsigned k=0;k<3;++k)w32(c,b+0x10+4*k,r32(c,e+0xc+4*k));
  if(!(r8(c,e+8)&2))continue;
  uint8_t bits=r8(c,b+0x6a);if(bits&8){w32(c,b+0x1c,0);w32(c,b+0xc,r32(c,set+8));w8(c,b+0x6a,(bits&0xf7u)|0x10u);}
  bits=r8(c,b+0x6a);if(bits&0x20)w8(c,b+0x6a,bits&0xc7u);
  const int32_t v=sext24(r32(c,b+0x14)),d=v-(int32_t)r32(c,b+0x1c),scale=(int32_t)r32(c,b+0x20);w32(c,b+0x14,0);w32(c,b+0x1c,(uint32_t)v);
  const uint32_t delta=(uint32_t)(((int64_t)scale*d)>>24);
  const float wgt=rf(c,e);total=nflstreet_gk_adds(total,wgt);const float t=total==zero?zero:nflstreet_gk_divs(wgt,total);
  const uint32_t k=r16(c,b+8);uint32_t go=flag;if(k==key||k==0xffffu||key==0xffffu){any=1;go=1;}
  if(go)acc=turn(c,t,delta,acc);}
 if(any)w32(c,out,acc);return any;}
/* 80176B3C: the root travel (flag 4 entries' x/z at +18/+1C). */
static int travel(C *c,uint32_t ch,uint32_t out,uint32_t key,uint32_t flag){
 const float zero=rf(c,0x802610ecu);float acc[2]={zero,zero},total=zero;int any=0;
 for(uint32_t i=0;i<r16(c,LIST)&&c->ok;++i){const uint32_t e=LIST+4+0x2cu*i;if(!(r8(c,e+8)&4))continue;
  const uint32_t idx=r8(c,e+9),b=ch+0x7cu*idx,k=r16(c,b+8);uint32_t go=flag;if(k==key||k==0xffffu||key==0xffffu){any=1;go=1;}
  if(go!=1)continue;
  uint8_t bits=r8(c,b+0x6a);if(bits&1){wf(c,b+0x24,rf(c,0x802610ecu));w8(c,b+0x6a,(bits&0xfeu)|2u);wf(c,b+0x28,rf(c,0x802610ecu));}
  bits=r8(c,b+0x6a);if(bits&4)w8(c,b+0x6a,bits&0xf8u);
  float loc[2];const float frac=rf(c,b+0x60);
  if(i+1<r16(c,LIST)&&r8(c,e+0x2c+9)==idx){
   if(!(r8(c,b+0x6a)&2)){const float b0=rf(c,e+0x2c+0x18),a0=rf(c,e+0x18),b1=rf(c,e+0x2c+0x1c);loc[0]=nflstreet_gk_fmadds(nflstreet_gk_subs(a0,b0),frac,b0);
    loc[1]=nflstreet_gk_fmadds(nflstreet_gk_subs(rf(c,e+0x1c),b1),frac,b1);}
   else{loc[0]=nflstreet_gk_muls(rf(c,e+0x18),frac);loc[1]=nflstreet_gk_muls(rf(c,e+0x1c),frac);}
   ++i;}
  else{loc[0]=rf(c,e+0x18);loc[1]=rf(c,e+0x1c);}
  float tmp[2]={nflstreet_gk_subs(loc[0],rf(c,b+0x24)),nflstreet_gk_subs(loc[1],rf(c,b+0x28))};
  wf(c,b+0x24,nflstreet_gk_adds(rf(c,b+0x24),tmp[0]));wf(c,b+0x28,nflstreet_gk_adds(rf(c,b+0x28),tmp[1]));
  const float sc=rf(c,b+0x2c);tmp[0]=nflstreet_gk_muls(tmp[0],sc);tmp[1]=nflstreet_gk_muls(tmp[1],sc);
  {const uint32_t a=r32(c,b+0xc);const float s=sine(c,a,0),k2=sine(c,a,1),x=tmp[0],y=tmp[1];
   const float sx=nflstreet_gk_muls(s,x),sy=nflstreet_gk_muls(s,y);tmp[1]=nflstreet_gk_fmadds(k2,y,sx);tmp[0]=nflstreet_gk_fmsubs(k2,x,sy);}
  const float wgt=rf(c,e);total=nflstreet_gk_adds(total,wgt);const float t=total==zero?zero:nflstreet_gk_divs(wgt,total);
  const float b0=acc[0],b1=acc[1];acc[0]=nflstreet_gk_fmadds(nflstreet_gk_subs(tmp[0],b0),t,b0);acc[1]=nflstreet_gk_fmadds(nflstreet_gk_subs(tmp[1],b1),t,b1);}
 if(any){wf(c,out+4,acc[1]);wf(c,out,acc[0]);}return any;}
static uint32_t group_key(C *c,uint32_t manager){ /* 801784B8 */
 uint32_t key=0xffff;for(int k=3;k>=0;--k)if(r16(c,manager+100u*(uint32_t)k+6)==3)key=r16(c,manager+100u*(uint32_t)k+4);return key;}
int nflstreet_actor_channels_pose(C *c,unsigned index,uint32_t actor){
 if(!c||!c->span||!actor||index>=NFLSTREET_CHANNEL_ACTORS)return 0;c->ok=1;prepare(c);
 const uint32_t set=actor+0x4d8,ch=actor+0x4e4,m=actor+0x9bc,visual=r32(c,actor+4);
 w32(c,visual+0x1c,r32(c,actor+0x304));w32(c,visual+0x20,r8(c,actor+0x308));
 build_list(c,set,ch,m,index);if(!c->ok)return 0;
 if(!nflstreet_actor_pose_blend(&c->pose[index],visual,LIST))return 0; /* 800D4CBC */
 const uint32_t flag=((r32(c,actor+0xc)^0x200u)>>9)&1u;
 w8(c,actor+0x220,(uint32_t)roots(c,set,ch,actor+0x224,group_key(c,r32(c,actor+0x318)),flag));
 w8(c,actor+0x221,(uint32_t)travel(c,ch,actor+0x228,group_key(c,r32(c,actor+0x318)),flag));
 if(!c->ok)return 0;
 return nflstreet_actor_pose_root(&c->pose[index],visual,ch,r16(c,actor+0x4dc),m); /* 800D4CE4 */}
int nflstreet_actor_channels_update(C *c,float dt){
 if(!c||!c->span)return 0;c->ok=1;const uint32_t table=r32(c,R13-0x7c30u),n=r16(c,table+10);if(!c->ok||n>NFLSTREET_CHANNEL_ACTORS)return 0;
 for(uint32_t i=0;i<n;++i)if(!nflstreet_actor_channels_manager(c,dt,r32(c,r32(c,R13-0x7c30u))+0x14d4u*i))return 0;
 for(uint32_t i=0;i<n;++i)if(!nflstreet_actor_channels_step(c,dt,r32(c,r32(c,R13-0x7c30u))+0x14d4u*i))return 0;
 for(uint32_t i=0;i<n;++i)if(!nflstreet_actor_channels_pose(c,i,r32(c,r32(c,R13-0x7c30u))+0x14d4u*i))return 0;
 return 1;}
