#include "view_frustum.h"
#include "draw_engine.h"
static uint8_t *span(nflstreet_pool_ram r,uint32_t a,uint32_t n){
 if(!r.bytes||(a&3u)||a<r.base||(uint64_t)a+n>0x100000000ull||(uint64_t)(a-r.base)+n>r.size)return NULL;return r.bytes+(a-r.base);}
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static float asfloat(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static float load(const uint8_t *p){return asfloat(nflstreet_rd32(p));}
static void put(uint8_t *p,float f){nflstreet_wr32(p,bits(f));}
static float add(float a,float b){return a+b;}static float sub(float a,float b){return a-b;}
static float mul(float a,float b){float result=a*b;return result!=result&&a==a&&b==b?asfloat(0x7fc00000u):result;}static float divide(float a,float b){float result=a/b;return result!=result&&a==a&&b==b?asfloat(0x7fc00000u):result;}
static float constant(const nflstreet_view_frustum_context *x,int32_t offset){return load(x->ram.bytes+(x->r13+(uint32_t)offset-x->ram.base));}
static int valid(const nflstreet_view_frustum_context *x){
 return x&&x->r13>=0x5ebcu&&span(x->ram,x->r13-0x5ebcu,0x88)&&span(x->ram,x->r13-0x4538u,52)&&span(x->ram,x->angle_constants,12)&&span(x->ram,x->fov_scale,4);}
#define C(o) constant(x,o)
#define F(a,b,c) nflstreet_fmadds(a,b,c)
static float kernel(const nflstreet_view_frustum_context *x,float v,float y,int mode){
 uint32_t original=bits(v),absolute=original&0x7fffffffu;
 if(absolute<=0x317fffffu){if(!absolute&&mode==-1)return divide(C(-0x5e68),asfloat(absolute));return mode==1?v:divide(C(-0x5e64),v);}
 float a=v;
 if(absolute>0x3f2ca13fu){if(original&0x80000000u){v=-v;y=-y;}
  a=add(sub(C(-0x5e60),v),sub(C(-0x5e5c),y));y=C(-0x5e58);}
 float z=mul(a,a),w=mul(z,z),s=mul(z,a);
 float odd=F(w,C(-0x450c),C(-0x4514));float even=F(w,C(-0x4508),C(-0x4510));
 odd=F(w,odd,C(-0x451c));even=F(w,even,C(-0x4518));
 odd=F(w,odd,C(-0x4524));even=F(w,even,C(-0x4520));
 odd=F(w,odd,C(-0x452c));even=F(w,even,C(-0x4528));
 odd=F(w,odd,C(-0x4534));even=F(w,even,C(-0x4530));
 float r=F(z,even,odd);r=F(s,r,y);r=F(z,r,y);r=F(C(-0x4538),s,r);w=add(a,r);
 if(absolute>0x3f2ca13fu){float m=(float)mode;float t=divide(mul(w,w),add(w,m));t=sub(a,sub(t,r));t=add(t,t);return mul(original&0x80000000u?-1.0f:1.0f,sub(m,t));}
 if(mode==1)return w;
 float reciprocal=divide(C(-0x5e64),w),truncated=asfloat(bits(w)&0xfffff000u),residual=sub(r,sub(truncated,a));
 float high=asfloat(bits(reciprocal)&0xfffff000u);float correction=F(high,truncated,C(-0x5e68));correction=F(high,residual,correction);return F(reciprocal,correction,high);}
int nflstreet_view_tangent(const nflstreet_view_frustum_context *x,uint32_t angle,float *result){
 if(!result||!valid(x))return 0;uint8_t *p=span(x->ram,x->angle_constants,12);
 uint64_t raw=(uint64_t)nflstreet_rd32(p)<<32|nflstreet_rd32(p+4),converted=0x4330000000000000ull|(angle^0x80000000u);double bias,d;
 memcpy(&bias,&raw,8);memcpy(&d,&converted,8);float v=mul((float)(d-bias),load(p+8));uint32_t u=bits(v),a=u&0x7fffffffu;
 if(a>0x4016cbe3u)return 0;float tail=C(-0x5ebc);int mode=1;
 if(a>0x3f490fdau){float z,lo;
  if(u&0x80000000u){z=add(v,C(-0x5e44));if((u&0x7ffffff0u)==0x3fc90fd0u){z=add(z,C(-0x5e3c));lo=C(-0x5e38);}else lo=C(-0x5e40);
   v=add(z,lo);tail=add(sub(z,v),lo);}
  else{z=sub(v,C(-0x5e44));if((u&0x7ffffff0u)==0x3fc90fd0u){z=sub(z,C(-0x5e3c));lo=C(-0x5e38);}else lo=C(-0x5e40);
   v=sub(z,lo);tail=sub(sub(z,v),lo);}mode=-1;}
 *result=kernel(x,v,tail,mode);return 1;}
int nflstreet_view_frustum_set(const nflstreet_view_frustum_context *x,uint32_t node,float fov,float aspect,float near_z,float far_z){
 if(!valid(x)||(bits(fov)&0x7fffffffu)>0x7f7fffffu||(bits(aspect)&0x7fffffffu)>0x7f7fffffu||(bits(near_z)&0x7fffffffu)>0x7f7fffffu||(bits(far_z)&0x7fffffffu)>0x7f7fffffu)return 0;uint8_t *p=span(x->ram,node,0xf8);if(!p)return 0;
 int32_t angle=nflstreet_fctiwz(mul(fov,load(span(x->ram,x->fov_scale,4))));
 int32_t half=angle/2;uint32_t identity;memcpy(&identity,&half,4);float tangent;if(!nflstreet_view_tangent(x,identity,&tangent))return 0;
 put(p+0xd0,aspect);put(p+0xe8,near_z);put(p+0xec,far_z);nflstreet_wr32(p+0x1c,(nflstreet_rd32(p+0x1c)&0xfffffffbu)|2u);put(p+0xcc,fov);
 float top=mul(near_z,tangent),bottom=-top;put(p+0xe0,top);put(p+0xe4,bottom);put(p+0xdc,mul(top,aspect));put(p+0xd8,mul(bottom,aspect));return 1;}
int nflstreet_view_scale_set(const nflstreet_view_frustum_context *x,uint32_t node,float scale){
 if(!valid(x)||(bits(scale)&0x7fffffffu)>0x7f7fffffu)return 0;uint8_t *p=span(x->ram,node,0xf8);if(!p)return 0;float tangent;if(!nflstreet_view_tangent(x,0x100000u,&tangent))return 0;
 float ratio=divide(load(p+0xe0),load(p+0xe8));float gain=divide(tangent,ratio);put(p+0xf0,mul(scale,gain));return 1;}
int nflstreet_view_binding_set(nflstreet_pool_ram r,uint32_t node,uint32_t identity,uint8_t flag){
 uint8_t *p=span(r,node,0x4c);if(!p)return 0;p[0x45]=flag;nflstreet_wr32(p+0x48,identity);return 1;}
