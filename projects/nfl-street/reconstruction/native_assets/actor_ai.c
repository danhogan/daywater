#include "actor_ai.h"
#include "gekko_float.h"
#include "gekko_quaternion/quaternion_kernels.h"
static uint8_t *at(nflstreet_actor_ai *c,uint32_t a,uint32_t n){return c&&c->span?c->span(c->memory,a,n):NULL;}
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static float f(const uint8_t *p){uint32_t w=rd(p);float v;memcpy(&v,&w,4);return v;}
static void wf(uint8_t *p,float v){uint32_t w;memcpy(&w,&v,4);wr(p,w);}
static int cf(nflstreet_actor_ai *c,uint32_t a,float *v){const uint8_t *p=at(c,a,4);if(!p)return 0;*v=f(p);return isfinite(*v);}
/* 80188D98: polynomial order, signed 24-bit wrap, including coincident
   XY (0/0 -> fctiwz indefinite -> wrapped heading 0). No libc atan2. */
static int heading(nflstreet_actor_ai *c,float y,float x,uint32_t *out){
 float zero,scale,k[9];if(!cf(c,0x802617d0u,&zero)||!cf(c,0x802617d4u,&scale))return 0;
 for(unsigned i=0;i<9;++i)if(!cf(c,0x8029194cu+4*i,k+i))return 0;
 float ay=fabsf(y),ax=fabsf(x),q=nflstreet_gk_divs(nflstreet_gk_subs(ay,ax),nflstreet_gk_adds(ay,ax));uint32_t quadrant=0;
 if(y>=zero){if(x<zero){quadrant=0x400000u;q=-q;}}else{quadrant=0x800000u;if(x>=zero){quadrant=0xffc00000u;q=-q;}}
 float q2=nflstreet_gk_muls(q,q),power=nflstreet_gk_muls(q,q2);
 float p=nflstreet_gk_fmadds(q,k[0],nflstreet_gk_muls(power,k[1]));power=nflstreet_gk_muls(power,q2);
 for(unsigned i=2;i<8;++i){p=nflstreet_gk_fmadds(power,k[i],p);if(i!=7)power=nflstreet_gk_muls(power,q2);}
 p=nflstreet_gk_muls(nflstreet_gk_adds(p,k[8]),scale);
 uint32_t value=(uint32_t)nflstreet_gk_fctiwz(p)+quadrant;value&=0xffffffu;
 *out=value&0x800000u?value|0xff000000u:value;return 1;}
int nflstreet_actor_ai_decode(nflstreet_actor_ai *c,uint32_t actor,uint32_t output){
 const uint8_t *p=at(c,actor+0x310u,4),*bias=at(c,0x80250a58u,8);uint8_t *o=at(c,output,8);float scale;
 if(!p||!bias||!o||!cf(c,0x80250a60u,&scale))return 0;p=at(c,rd(p),3);if(!p)return 0;
 double b=nflstreet_gk_value((uint64_t)rd(bias)<<32|rd(bias+4));float v[2];
 for(unsigned i=0;i<2;++i){uint32_t low=(uint32_t)(int32_t)(int8_t)p[1+i]^0x80000000u;
  v[i]=nflstreet_gk_muls(nflstreet_gk_convert(low,b),scale);if(!isfinite(v[i]))return 0;}
 for(unsigned i=0;i<2;++i)wf(o+4*i,v[i]);return 1;}
int nflstreet_actor_ai_enter(nflstreet_actor_ai *c,uint32_t actor){
 uint8_t *p=at(c,actor+0x150u,1);if(!p||!nflstreet_actor_ai_decode(c,actor,actor+0x154u))return 0;*p=0;return 1;}
int nflstreet_actor_ai_target(nflstreet_actor_ai *c,uint32_t actor,uint32_t other,uint32_t offset,uint32_t output){
 const uint8_t *a=at(c,actor+0x1a8u,4),*b=at(c,other+0x1a8u,8),*v=at(c,offset,8),*side=at(c,c->r13-0x7078u,4);
 uint8_t *o=at(c,output,8);float zero,one,outer,width;if(!a||!b||!v||!side||!o||
  !cf(c,rd(side)+0x90u,&width)||!cf(c,0x80250a68u,&zero)||!cf(c,0x80250a6cu,&one)||!cf(c,0x80250a70u,&outer))return 0;
 float x=f(a),ox=f(v),oy=f(v+4),bx=f(b),by=f(b+4);if(!isfinite(x)||!isfinite(ox)||!isfinite(oy)||!isfinite(bx)||!isfinite(by))return 0;
 float t=ox>zero?nflstreet_gk_subs(width,x):nflstreet_gk_adds(width,x);
 t=nflstreet_gk_divs(nflstreet_gk_subs(t,one),nflstreet_gk_subs(outer,one));
 if(t<zero)t=zero;else if(t>one)t=one;
 float tx=nflstreet_gk_adds(bx,nflstreet_gk_muls(ox,t)),ty=nflstreet_gk_adds(by,oy);
 if(!isfinite(tx)||!isfinite(ty))return 0;wf(o,tx);wf(o+4,ty);return 1;}
int nflstreet_actor_ai_project(nflstreet_actor_ai *c,uint32_t actor,uint32_t target){
 const uint8_t *a=at(c,actor+0x1a8u,8),*t=at(c,target,8);uint8_t *o=at(c,actor+0x200u,0x30);float k[8];
 if(!a||!t||!o)return 0;for(unsigned i=0;i<8;++i)if(!cf(c,0x80250a74u+4*i,k+i))return 0;
 float dx=nflstreet_gk_subs(f(t),f(a)),dy=nflstreet_gk_subs(f(t+4),f(a+4));
 if(!isfinite(dx)||!isfinite(dy))return 0;uint32_t angle;if(!heading(c,dy,dx,&angle))return 0;
 float square=nflstreet_gk_fmadds(dx,dx,nflstreet_gk_muls(dy,dy));if(!isfinite(square))return 0;
 float distance=nflstreet_802165c8_sqrt(square),strength;
 if(distance<=k[0])strength=distance>k[4]?k[5]:k[6];else{
  float scaled=nflstreet_gk_muls(nflstreet_gk_subs(distance,k[0]),k[1]);if(scaled>k[0])scaled=k[0];
  strength=nflstreet_gk_fmadds(scaled,k[2],k[3]);}
 if(!isfinite(strength))return 0;wr(o+4,angle);o[0x0e]=1;wr(o+8,angle);wf(o,strength);
 if(strength>k[7])o[0x0f]=1;return 1;}
