#include "light_rig.h"
#include <math.h>
static uint8_t *at(nflstreet_draw *d,uint32_t a,uint32_t n){return nflstreet_draw_at(d,a,n);}
static float fword(const uint8_t *p){return nflstreet_as_float(nflstreet_rd32(p));}
static void put(uint8_t *p,float f){nflstreet_wr32(p,nflstreet_float_bits(f));}
static void put3(uint8_t *p,const float v[3]){for(unsigned i=0;i<3;++i)put(p+4*i,v[i]);}
/* 802165C8 (a correctly rounded square root; -x gives the PowerPC default NaN). */
static float root(float x){if(x<0.0f)return nflstreet_as_float(0x7fc00000u);return sqrtf(x);}
/* 801DE118 with 801DE094/80188EAC (epsilon 802617D8, one, zero). */
static void normalize(float v[3]){
 const float n=nflstreet_recip_sqrt(nflstreet_fmadds(v[2],v[2],nflstreet_fmadds(v[0],v[0],v[1]*v[1])),nflstreet_as_float(0x33d6bf95u),1.0f,0.0f);
 for(unsigned i=0;i<3;++i)v[i]*=n;}
/* fctiwz of a double, with the PowerPC saturation and NaN result. */
static int32_t fctiwz(double v){if(v!=v||v<-2147483648.0)return INT32_MIN;if(v>=2147483648.0)return INT32_MAX;return (int32_t)v;}
/* The unsigned conversion of 801ED580/801ED024 (bound 80268DE8/80268DD0 = 2^31). */
static uint32_t to_unsigned(float f){const double v=f;return v>=2147483648.0?(uint32_t)fctiwz(v-2147483648.0)^0x80000000u:(uint32_t)fctiwz(v);}
/* The Gekko frsqrte estimate (the hardware's 32-entry table; ModernGekko and
   Dolphin implement it the same way), for a positive finite value. */
static double rsqrte(double value){
 static const int32_t base[32]={0x1a7e800,0x17cb800,0x1552800,0x130c000,0x10f2000,0x0eff000,0x0d2e000,0x0b7c000,
  0x09e5000,0x0867000,0x06ff000,0x05ab800,0x046a000,0x0339800,0x0218800,0x0105800,0x3ffa000,0x3c29000,0x38aa000,0x3572000,
  0x3279000,0x2fb7000,0x2d26000,0x2ac0000,0x2881000,0x2665000,0x2468000,0x2287000,0x20c1000,0x1f12000,0x1d79000,0x1bf4000};
 static const int32_t dec[32]={-0x568,-0x4f3,-0x48d,-0x435,-0x3e7,-0x3a2,-0x365,-0x32e,-0x2fc,-0x2d0,-0x2a8,-0x283,-0x261,-0x243,
  -0x226,-0x20b,-0x7a4,-0x700,-0x670,-0x5f2,-0x584,-0x524,-0x4cc,-0x47e,-0x43a,-0x3fa,-0x3c2,-0x38e,-0x35e,-0x332,-0x30a,-0x2e6};
 uint64_t bits;memcpy(&bits,&value,8);uint64_t mantissa=bits&0x000fffffffffffffull;int64_t exponent=(int64_t)(bits&0x7ff0000000000000ull);
 if(exponent==0x7ff0000000000000ll)return 0.0;
 if(!exponent){do{exponent-=0x0010000000000000ll;mantissa<<=1;}while(!(mantissa&0x0010000000000000ull));
  mantissa&=0x000fffffffffffffull;exponent+=0x0010000000000000ll;}
 const uint64_t lsb=(uint64_t)exponent&0x0010000000000000ull;
 exponent=(0x3ff0000000000000ll-(exponent-0x3fe0000000000000ll)/2)&0x7ff0000000000000ll;
 const uint32_t i=(uint32_t)((lsb|mantissa)>>37);
 bits=(uint64_t)exponent|((uint64_t)(base[i/2048u]+dec[i/2048u]*(int32_t)(i%2048u))<<26);memcpy(&value,&bits,8);return value;}
/* 8020505C GXInitSpecularDir(n): half-angle direction and a far position. */
static void specular_dir(uint8_t *o,const float n[3]){
 const float x=-n[0],y=-n[1],z=1.0f-n[2],m=z*z+(x*x+y*y);float r=m;
 if(m!=0.0f){float s=m;
  if(m>0.0f){double e=rsqrte(m),k;
   for(unsigned i=0;i<2;++i){k=e*e;e*=0.5;k=(double)m*k;k=3.0-k;e*=k;}
   k=e*e;e*=0.5;k=(double)m*k;k=3.0-k;s=(float)((double)m*(e*k));}
  r=1.0f/s;}
 put(o+0x34,x*r);put(o+0x38,y*r);put(o+0x3c,z*r);
 const float far=nflstreet_as_float(0xdd5e0b6bu); /* r13-4BF8 */
 put(o+0x28,far*n[0]);put(o+0x2c,far*n[1]);put(o+0x30,far*n[2]);}
static void put6(uint8_t *p,float a,float b,float c,float e,float f,float g){put(p,a);put(p+4,b);put(p+8,c);put(p+12,e);put(p+16,f);put(p+20,g);}
/* 801ED998: one light set from five entries. Ambient (type 1) sets the
   ambient color; types 2 and 3 add a light far along the view direction
   (GXInitLightPos/Dir, spot and distance attenuation off); type 4 a
   specular light (GXInitSpecularDir, GXInitLightAttn(0,0,1,25,0,-24)). */
static int light_set(nflstreet_draw *d,uint32_t rig,uint32_t lights,uint32_t entries,uint32_t to){
 uint8_t *out=at(d,to,0x108),*e;const uint8_t *g=at(d,rig+0x94,16);if(!out||!g)return 0;
 nflstreet_wr32(out,0);out[4]=out[5]=out[6]=out[7]=0;
 for(unsigned k=0;k<5;++k){const uint8_t *l;if(!(e=at(d,entries+0x14u*k,0x14))||!(l=at(d,lights+0x30u*nflstreet_rd32(e),0x30)))return 0;
  uint32_t color=0;for(unsigned c=0;c<4;++c)color|=(to_unsigned(fword(l+0x10+4*c)*fword(e+4+4*c)*fword(g+4*c))&0xffu)<<(24-8*c);
  const uint32_t type=nflstreet_rd32(l),count=nflstreet_rd32(out);
  if(type==1){nflstreet_wr32(out+4,color);continue;}
  if(type<2||type>4||count>3)continue;
  nflstreet_wr32(out,count+1);uint8_t *o=out+8+0x40*count;const float v[3]={fword(l+0x24),fword(l+0x28),fword(l+0x2c)};
  if(type==4){nflstreet_wr32(o+0xc,color);const float n[3]={-v[0],-v[1],-v[2]};specular_dir(o,n);put6(o+0x10,0.0f,0.0f,1.0f,25.0f,0.0f,-24.0f);}
  else{const float far=10000.0f; /* 80268E10 */
   for(unsigned i=0;i<3;++i){put(o+0x28+4*i,v[i]*far);put(o+0x34+4*i,v[i]);}
   put6(o+0x10,1.0f,0.0f,0.0f,1.0f,0.0f,0.0f);nflstreet_wr32(o+0xc,color);}}
 return 1;}
/* 801ED024: shadow color, each channel x128 (80268DDC), the words OR'ed
   at shifts 0/8/16/24 without masking
   to bytes. */
static uint32_t shadow_color(const uint8_t *c){
 uint32_t w[4];for(unsigned i=0;i<4;++i)w[i]=to_unsigned(fword(c+4*i)*128.0f);
 return w[0]|w[1]<<8|w[2]<<16|w[3]<<24;}
/* The projections' fixed part: 1 at [0][0], [1][1] and [3][3], 0 elsewhere. */
static void flat(float m[16]){for(unsigned i=0;i<16;++i)m[i]=i==0||i==5||i==15?1.0f:0.0f;}
/* 801ED230: shadow projections (type 1: 801ECEA4 from a light's world
   direction, the x/y shear capped at r13-6228 by 801DE1B4; type 2:
   801ECF80 from an angle and length) and colors. */
static int shadows(nflstreet_draw *d,const nflstreet_light_rig_context *c,uint32_t rig,uint32_t lights,uint32_t count){
 uint32_t limit_word;if(!nflstreet_draw_word(d,c->shadow_limit,&limit_word))return 0;const float limit=nflstreet_as_float(limit_word);
 for(uint32_t i=0;i<count;++i){const uint8_t *r=at(d,rig+0x14+0x20*i,0x20);uint8_t *m=at(d,c->shadows+0x40*i,0x40),*w=at(d,c->shadow_colors+4*i,4);
  if(!r||!m||!w)return 0;const uint32_t type=nflstreet_rd32(r);float t[16];
  if(type==1){const uint8_t *l=at(d,lights+0x30u*nflstreet_rd32(r+0x14),0x30);if(!l)return 0;
   const float x=fword(l+4),y=fword(l+8),z=fword(l+12);float s[2]={-x/z,-y/z};
   const float len=root(nflstreet_fmadds(s[0],s[0],s[1]*s[1]));
   if(!(len<=limit)){const float k=limit/root(nflstreet_fmadds(s[0],s[0],s[1]*s[1]));s[0]*=k;s[1]*=k;}
   flat(t);t[2]=s[0];t[6]=s[1];for(unsigned k=0;k<16;++k)put(m+4*k,t[k]);}
  else if(type==2){const float scale=fword(r+0x1c);float sine,cosine;
   const uint32_t angle=(uint32_t)nflstreet_fctiwz(fword(r+0x18)*nflstreet_as_float(0x47360b61u)); /* 80268DD8 */
   if(!nflstreet_sine(d,&c->stacks,angle,0,&sine)||!nflstreet_sine(d,&c->stacks,angle,1,&cosine))return 0;
   flat(t);t[2]=cosine*scale;t[6]=sine*scale;for(unsigned k=0;k<16;++k)put(m+4*k,t[k]);}
  nflstreet_wr32(w,shadow_color(r+4));}
 return 1;}
static int known_callback(uint32_t f){return !f||f==0x8015b858u||f==0x8015b8a4u||f==0x8015b8c4u;}
int nflstreet_light_rig_update(const nflstreet_light_rig_context *c,nflstreet_render_state *s){
 nflstreet_draw d={c->x,s};uint32_t rig,lights,count,sets,set_count,shadow_count;
 if(!nflstreet_draw_word(&d,c->rig,&rig))return 0;if(!rig)return 1;
 const uint8_t *h=at(&d,rig,0x14);if(!h)return 0;
 count=nflstreet_rd32(h);lights=nflstreet_rd32(h+4);set_count=nflstreet_rd32(h+8);sets=nflstreet_rd32(h+12);shadow_count=nflstreet_rd32(h+16);
 for(uint32_t i=0;i<count;++i){const uint8_t *l=at(&d,lights+0x30*i,0x30);if(!l||!known_callback(nflstreet_rd32(l+0x20)))return 0;}
 const uint8_t *sun=at(&d,c->sun,12);if(!sun||!nflstreet_matrix_select(&d,&c->stacks))return 0;
 /* 801ED168: callbacks, the world direction normalized in place, then the
    view direction (types 2 and 4 rotated by the view, 801DEB54; type 3
    fixed to the camera). */
 for(uint32_t i=0;i<count;++i){uint8_t *l=at(&d,lights+0x30*i,0x30);const uint32_t f=nflstreet_rd32(l+0x20);float v[3];
  if(f==0x8015b858u||f==0x8015b8a4u)for(unsigned k=0;k<3;++k)nflstreet_wr32(l+4+4*k,nflstreet_rd32(sun+4*k));
  else if(f==0x8015b8c4u){v[0]=-fword(sun);v[1]=-fword(sun+4);v[2]=0.0f; /* 80260590 */normalize(v);put3(l+4,v);}
  for(unsigned k=0;k<3;++k)v[k]=fword(l+4+4*k);normalize(v);put3(l+4,v);
  const uint32_t type=nflstreet_rd32(l);
  if(type==2||type==4){float r[3];if(!nflstreet_matrix_rotate_vector(&d,&c->stacks,v,r))return 0;put3(l+0x24,r);}
  else if(type==3)put3(l+0x24,v);}
 for(uint32_t i=0;i<set_count;++i)if(!light_set(&d,rig,lights,sets+100*i,c->light_sets+0x108*i))return 0;
 if(!shadows(&d,c,rig,lights,shadow_count))return 0;
 s->light_set=0;return 1;}
