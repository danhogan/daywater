#include "field_overlay.h"
#include "scenery_shared.h"
#include "gekko_float.h"
#define GX NFLSTREET_GX
#define add nflstreet_gk_adds
#define sub nflstreet_gk_subs
#define mul nflstreet_gk_muls
#define divs nflstreet_gk_divs
#define madd nflstreet_gk_fmadds
#define msub nflstreet_gk_fmsubs
#define bits nflstreet_float_bits
#define rd32 nflstreet_rd32
static uint8_t *at(nflstreet_draw *d,uint32_t a,uint32_t n){return nflstreet_draw_at(d,a,n);}
/* Fixed tokens below are evidenced by the GNNE69 r0 disassembly. */
static float f(nflstreet_draw *d,uint32_t a){const uint8_t *p=at(d,a,4);return p?nflstreet_as_float(rd32(p)):NAN;}
static double df(nflstreet_draw *d,uint32_t a){const uint8_t *p=at(d,a,8);return p?nflstreet_gk_value((uint64_t)rd32(p)<<32|rd32(p+4)):NAN;}
static uint32_t word(nflstreet_draw *d,uint32_t a){const uint8_t *p=at(d,a,4);return p?rd32(p):0;}
static int put(nflstreet_draw *d,uint32_t a,float v){uint8_t *p=at(d,a,4);if(!p)return 0;nflstreet_wr32(p,bits(v));return 1;}
static float k(nflstreet_draw *d,unsigned off){return f(d,0x80260000u+off);}
static int projection(nflstreet_draw *d,const nflstreet_field_overlay_context *c,uint32_t v[7]){
 uint32_t g=word(d,c->r13-0x4c90u);const uint8_t *p=at(d,g+0x420,28);if(!p)return 0;
 v[0]=bits(nflstreet_gk_convert(rd32(p),df(d,c->r13-0x4b18u)));for(unsigned i=1;i<7;++i)v[i]=rd32(p+4*i);return 1;}
static int setv(nflstreet_draw *d,const uint32_t v[7]){return nflstreet_draw_gx(d,NFLSTREET_GX_SET_PROJECTION_V,0,NULL,v,7);}
/* 801DED68, or 801DEC94 when w is implicit: preserve the scalar instruction order. */
static int transform(nflstreet_draw *d,const nflstreet_matrix_stacks *s,const float v[3],float w,int implicit,float o[4]){
 uint32_t top;if(!nflstreet_matrix_top(d,s,&top))return 0;const uint8_t *p=at(d,top,64);if(!p)return 0;
 for(unsigned r=0;r<4;++r){float m[4];for(unsigned i=0;i<4;++i)m[i]=nflstreet_as_float(rd32(p+16*r+4*i));
  float a=mul(m[1],v[1]);a=madd(m[0],v[0],a);a=madd(m[2],v[2],a);
  o[r]=implicit?add(a,r==3?f(d,0x80263cbcu):m[3]):madd(m[3],w,a);}
 return 1;}
static int slot(nflstreet_draw *d,const nflstreet_field_overlay_context *c,unsigned n){
 return nflstreet_matrix_select(d,c->stacks)&&nflstreet_matrix_push(d,c->stacks)&&nflstreet_matrix_load(d,c->stacks,0x80331ec0u+64*n);}
/* 80159BB4: clipping leaves the caller's projected coordinates unchanged. */
static int project(nflstreet_draw *d,const nflstreet_field_overlay_context *c,uint32_t pos,float q[3],float *fade){
 float v[3],o[4];for(unsigned i=0;i<3;++i)v[i]=f(d,pos+4*i);
 if(!slot(d,c,4)||!transform(d,c->stacks,v,k(d,0x48c),0,o))return 0;
 float inv=divs(k(d,0x48c),o[3]),x=mul(o[0],inv),y=mul(o[1],inv);*fade=k(d,0x48c);
 if(o[2]<-o[3]||x<k(d,0x490)||x>k(d,0x494)||y<k(d,0x490)||y>k(d,0x494))*fade=k(d,0x4a0);
 else{q[0]=x;q[1]=y;q[2]=mul(o[2],inv);float edge=fabsf(x)>fabsf(y)?fabsf(x):fabsf(y);
  if(!(edge<k(d,0x498)))*fade=sub(*fade,mul(sub(edge,k(d,0x498)),k(d,0x49c)));}
 return nflstreet_matrix_pop(d,c->stacks);}
static uint32_t uint_depth(double v,double split){return v>=split?(uint32_t)nflstreet_gk_fctiwz(v-split)^0x80000000u:(uint32_t)nflstreet_gk_fctiwz(v);}
static int visible(nflstreet_draw *d,const nflstreet_field_overlay_context *c,uint32_t rec,float q[3],float fade){
 uint8_t *r=at(d,rec,0xc4);if(!r)return 0;uint32_t mode=rd32(r+0xc);
 /* 80159E88 tests depth before its final fade check, including clipped
    points. q retains the exact caller-local residue seeded by 8015A084's
    identity and then by preceding records (including the aspect y store). */
 if(mode==1){float z=add(q[2],k(d,0x4a4));if(z<k(d,0x4a8))z=k(d,0x4a8);else if(!(z<=k(d,0x4a4)))z=k(d,0x4a4);
  uint32_t depth=uint_depth(mul(z,k(d,0x4ac)),df(d,0x802604b0u));int ok=0;
  if(!(q[0]<=k(d,0x4b8)||q[0]>=k(d,0x4a4)||q[1]<=k(d,0x4b8)||q[1]>=k(d,0x4a4))){
   /* 8014E29C(7) returns the table in every mode, including mode 6;
      80187B34 is the constant 0xffff. The surrounding configuration query
      machinery is outside this renderer (no side effect in saved contexts). */
   if(word(d,0x8030b31cu)==1)q[1]=mul(q[1],divs(nflstreet_gk_convert(0x150u^0x80000000u,df(d,0x802604c0u)),
    nflstreet_gk_convert(word(d,0x8039e07cu)^0x80000000u,df(d,0x802604c0u))));
   uint32_t sampled=0;
   if(!(q[0]<=k(d,0x4e0)||q[1]<=k(d,0x4e0)||q[0]>=k(d,0x4e4)||q[1]>=k(d,0x4e4))){
    float width=nflstreet_gk_convert(word(d,0x8039e078u)^0x80000000u,df(d,0x802604e8u));
    float height=nflstreet_gk_convert(word(d,0x8039e07cu)^0x80000000u,df(d,0x802604e8u));
    uint32_t x=(uint32_t)nflstreet_gk_fctiwz(mul(mul(add(q[0],k(d,0x4e4)),width),k(d,0x4f0)))&0xffffu;
    uint32_t y=(uint32_t)nflstreet_gk_fctiwz(mul(mul(sub(k(d,0x4e4),q[1]),height),k(d,0x4f0)))&0xffffu;
    if(!c->depth||!c->depth(c->depth_user,x,y,&sampled)||!nflstreet_draw_gx(d,NFLSTREET_FIELD_GX_PEEK_Z,2,(uint32_t[]){x,y},&sampled,1))return 0;}
   ok=depth<=sampled&&f(d,0x8025269cu)<q[0]&&q[0]<f(d,0x802526a0u)&&f(d,0x8025269cu)<q[1]&&q[1]<f(d,0x802526a0u);}
  r[0x11]=(uint8_t)ok;
 }else if(mode==0||mode==2)r[0x11]=1;
 if(fade<=k(d,0x4a8))r[0x11]=0;
 return 1;}
/* 801EE1C0(1) with the saved kind-0 stream: seed always advances, even
   when the replay byte overrides the result. Other stream kinds rejected. */
static int random1(nflstreet_draw *d,const nflstreet_field_overlay_context *c,float *out){
 uint32_t m=word(d,c->r13-0x61f8u),tab=word(d,m+0x14),p=word(d,tab+4);uint8_t *s=at(d,p,8),*manager=at(d,m,2);
 if(!s||!manager||rd32(s)!=0)return 0;uint32_t next=rd32(s+4)*0x19660du+0x3c6ef35fu;nflstreet_wr32(s+4,next);
 float v=mul(nflstreet_gk_convert(next,df(d,0x80261700u)),f(d,0x80261708u));if(!(v<f(d,0x8026170cu)))v=f(d,0x8026170cu);
 if(manager[1])v=f(d,manager[1]&0x80?0x80268e2cu:0x80268e30u);*out=v;return 1;}
static float clamp(nflstreet_draw *d,float v){if(v<k(d,0x458))return k(d,0x458);return v<=k(d,0x454)?v:k(d,0x454);}
/* 80200000 + 80207CA8: SDK projection call emitted as the full matrix,
   as with other intercepted SDK calls the GX shadow is not emulated. */
static int ortho(nflstreet_draw *d,const nflstreet_field_overlay_context *c){
 float one=f(d,c->r13-0x4cf8u),two=f(d,c->r13-0x4cf4u),zero=f(d,c->r13-0x4cf0u),neg=f(d,c->r13-0x4cecu);
 float a=divs(one,sub(k(d,0x47c),k(d,0x478))),b=divs(one,sub(k(d,0x470),k(d,0x474))),z=divs(one,sub(k(d,0x480),k(d,0x454)));
 float m[16]={mul(two,a),zero,zero,mul(a,-add(k(d,0x47c),k(d,0x478))),zero,mul(two,b),zero,mul(b,-add(k(d,0x470),k(d,0x474))),zero,zero,mul(neg,z),mul(-k(d,0x480),z),zero,zero,zero,one};
 /* 80159538 explicitly replaces translation words with its own zero. */
 m[3]=m[7]=m[11]=k(d,0x458);uint32_t data[16];for(unsigned i=0;i<16;++i)data[i]=bits(m[i]);
 return nflstreet_draw_gx(d,NFLSTREET_FIELD_GX_SET_PROJECTION,1,(uint32_t[]){1},data,16);}
static int quad(nflstreet_draw *d,const nflstreet_field_overlay_context *c,uint32_t rec){
 uint8_t *r=at(d,rec,0xc4);if(!r)return 0;uint32_t pv[7];float q[3]={0},fade,angle=k(d,0x454);
 if(!projection(d,c,pv)||!project(d,c,rec+0x18,q,&fade))return 0;
 if(rd32(r+4)==1){float v[3],o[3];for(unsigned i=0;i<3;++i)v[i]=f(d,rec+0x24+4*i);
  if(!slot(d,c,3)||!nflstreet_matrix_rotate_vector(d,c->stacks,v,o))return 0;
  /* rotate_vector ignores translation and the fourth row */
  angle=madd(o[2],k(d,0x450),madd(o[0],k(d,0x448),mul(o[1],k(d,0x44c))));
  if(!(angle>=k(d,0x458)))angle=k(d,0x458);if(!nflstreet_matrix_pop(d,c->stacks))return 0;}
 if(r[0x34]){r[0x35]=(uint8_t)(r[0x35]+1);if(r[0x35]>=r[0x34]){r[0x35]=0;float a,b=f(d,rec+0x30),one=k(d,0x454);uint32_t main=rd32(r);
   if(!random1(d,c,&a))return 0;float limit=add(msub(a,add(b,b),b),one),value=f(d,main+0x14);
   if(limit<=value){if(!random1(d,c,&a))return 0;b=f(d,rec+0x30);value=add(msub(a,add(b,b),b),one);}if(!put(d,main+0x14,value))return 0;}}
 float alpha=clamp(d,r[0x11]?add(f(d,rec+0x14),f(d,rec+0x38)):sub(f(d,rec+0x14),f(d,rec+0x38)));
 if(!put(d,rec+0x14,alpha))return 0;
 if(alpha!=k(d,0x458)){
  float A=clamp(d,mul(mul(mul(f(d,rd32(r)+0x14),madd(angle,k(d,0x45c),k(d,0x460))),fade),alpha));uint32_t previous=0xffffffffu;
  for(unsigned i=0;i<=r[0x36];++i){uint32_t desc;float scale;
   if(i==r[0x36]){desc=rd32(r);scale=k(d,0x454);}else{desc=rd32(r+0x3c+8*i);scale=f(d,rec+0x40+8*i);}
   const uint8_t *b=at(d,desc,0x18);if(!b)return 0;float xy[2];for(unsigned j=0;j<2;++j){float center=f(d,c->r13-0x68e8u+4*j);xy[j]=madd(sub(q[j],center),scale,center);}
   uint32_t texture=rd32(b),obj=0x802aa450u+0x68u*texture;const uint8_t *tex=at(d,obj,0x68);if(!tex)return 0;
   if(texture!=previous){if(tex[0x64]&&(!GX(d,NFLSTREET_GX_LOAD_TLUT,obj+0x40,0)||!GX(d,NFLSTREET_GX_INIT_TEX_OBJ_TLUT,obj+0x10,0)))return 0;
    if(!GX(d,NFLSTREET_GX_LOAD_TEX_OBJ,obj+0x10,0))return 0;}previous=texture;
   uint32_t mode=rd32(r+0xc);if(!GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,0)||!GX(d,NFLSTREET_GX_SET_Z_MODE,mode==2?1:0,mode==2?3:7,0))return 0;
   float size=mul(madd(angle,k(d,0x464),k(d,0x468)),f(d,desc+4)),half=mul(size,k(d,0x46c)),x0,x1,y0,y1,z;
   if(rd32(r+8)==1&&i==r[0x36]){float v[3],o[4];for(unsigned j=0;j<3;++j)v[j]=f(d,rec+0x18+4*j);
    if(!setv(d,pv)||!slot(d,c,3)||!transform(d,c->stacks,v,1,1,o)||!nflstreet_matrix_pop(d,c->stacks))return 0;
    x0=sub(o[0],half);x1=add(o[0],half);y0=sub(o[1],half);y1=add(o[1],half);z=o[2];
   }else{if(!ortho(d,c))return 0;float x=mul(xy[0],k(d,0x47c)),y=mul(xy[1],k(d,0x470));x0=sub(x,size);x1=add(x,size);y0=sub(y,size);y1=add(y,size);z=k(d,0x484);}
   uint32_t color=0;for(unsigned j=0;j<3;++j)color|=((uint32_t)nflstreet_gk_fctiwz(mul(f(d,desc+8+4*j),k(d,0x488)))&255u)<<(24-8*j);
   color|=(uint32_t)nflstreet_gk_fctiwz(mul(A,k(d,0x488)))&255u;
   if(!nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},&color,1)||!GX(d,NFLSTREET_GX_BEGIN,0x98,0,4))return 0;
   float zero=k(d,0x458),one=k(d,0x454);const float verts[4][5]={{x0,y0,z,zero,zero},{x0,y1,z,one,zero},{x1,y0,z,zero,one},{x1,y1,z,one,one}};
   for(unsigned v=0;v<4;++v)for(unsigned j=0;j<5;++j)if(!GX(d,NFLSTREET_GX_WRITE,4,bits(verts[v][j])))return 0;
  }}
 return setv(d,pv)&&GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,1);}
int nflstreet_field_overlay_draw(const nflstreet_field_overlay_context *c,nflstreet_render_state *s){
 if(!c||!c->scene||!c->scene->draw||!c->stacks||!s)return 0;nflstreet_draw dd={c->scene->draw,s},*d=&dd;
 const uint8_t *enabled=at(d,c->r13-0x71c0u,1);if(!enabled)return 0;if(!*enabled)return 1;
 nflstreet_draw_reset(d);
 if(!GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0)||!nflstreet_matrix_push_identity(d,c->stacks)||!nflstreet_matrix_load_top(d,c->stacks)||
 !nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,9,1)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,0xd,1)||
 !GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)||!GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,0,0,1,2,2)||!GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||
 !GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)||!GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0)||!GX(d,NFLSTREET_GX_SET_ALPHA_COMPARE,4,0,0,4,0)||
 !GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)||!GX(d,NFLSTREET_GX_SET_TEV_OP,0,0)||!GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)||
 !GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,0,4,0x3c,0,0x7d)||!GX(d,NFLSTREET_GX_SET_BLEND_MODE,1,4,1,5))return 0;
 uint32_t saved[7];if(!projection(d,c,saved))return 0;uint8_t *p=at(d,0x8030bdd4u,28);if(!p)return 0;for(unsigned i=0;i<7;++i)nflstreet_wr32(p+4*i,saved[i]);
 uint32_t previous;uint8_t *fog=at(d,c->r13-0x592cu,1);if(!fog||!nflstreet_scenery_fog_switch(d,c->scene,0,&previous))return 0;*fog=(uint8_t)previous;
 uint32_t count=word(d,0x802aa440u),records=word(d,0x802aa448u);if(count>65535u)return 0;
 /* With A = 800D1118's stack pointer, 8015A084 writes its identity at
    A-0x40. 80159E88's output locals are A-0x38/-0x34/-0x30: matrix words
    2/3/4, all zero. Its nested callees do not overwrite those caller locals.
    A clipped projection therefore reuses the previous record's coordinates,
    or these zeros on the first record; no incoming stack bytes are needed. */
 float q[3]={0};
 for(unsigned i=0;i<count;++i){uint32_t rec=records+0xc4u*i;uint8_t *r=at(d,rec,0xc4);if(!r)return 0;if(!r[0x10])continue;
  float fade;if(!project(d,c,rec+0x18,q,&fade)||!visible(d,c,rec,q,fade))return 0;
  if(!(fade<=k(d,0x4a8))&&!quad(d,c,rec))return 0;}
 if(!nflstreet_matrix_pop(d,c->stacks)||!nflstreet_matrix_load_top(d,c->stacks)||!GX(d,NFLSTREET_GX_SET_BLEND_MODE,0,0,0,5))return 0;
 p=at(d,0x8030bdd4u,28);if(!p)return 0;for(unsigned i=0;i<7;++i)saved[i]=rd32(p+4*i);
 return setv(d,saved)&&nflstreet_scenery_fog_switch(d,c->scene,*fog,&previous);}
