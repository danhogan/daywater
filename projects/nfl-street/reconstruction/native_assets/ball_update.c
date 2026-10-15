#include "ball_update.h"
#include "../ball/ball_frame.h"
#include "../ball/ball_orientation.h"
#include "gekko_float.h"
#include "gekko_quaternion/quaternion_kernels.h"
#include <math.h>
#define rd32 nflstreet_rd32
#define put nflstreet_wr32
static uint8_t *at(nflstreet_draw *d,uint32_t a,uint32_t n){return nflstreet_draw_at(d,a,n);}
static uint32_t word(nflstreet_draw *d,uint32_t a){uint32_t v=0;nflstreet_draw_word(d,a,&v);return v;}
static float real(nflstreet_draw *d,uint32_t a){return nflstreet_as_float(word(d,a));}
static void pf(uint8_t *p,float f){put(p,nflstreet_float_bits(f));}
/* Original 80020944 -> 8003A4A0: the high byte of a handle is ignored. */
static uint32_t resolve(nflstreet_draw *d,uint32_t r13,uint32_t h){
 if((h&255)!=1)return 0;uint32_t t=word(d,r13-0x7c30u);if(!t)return 0;
 const uint8_t *p=at(d,t,12);if(!p)return 0;
 return rd32(p)+(((h>>8)&255)*nflstreet_rd16(p+8)+((h>>16)&255))*0x14d4u;}
static void cross(const float a[3],const float b[3],float o[3]){
 o[0]=nflstreet_gk_fmsubs(a[1],b[2],nflstreet_gk_muls(a[2],b[1]));
 o[1]=nflstreet_gk_fmsubs(a[2],b[0],nflstreet_gk_muls(a[0],b[2]));
 o[2]=nflstreet_gk_fmsubs(a[0],b[1],nflstreet_gk_muls(a[1],b[0]));}
/* 8010846C: full 3D cross/dot, although the source vertices have z=0. */
static int halfspace(const float p[3],const float a[3],const float b[3],const float c[3],float epsilon){
 float edge[3],pb[3],ab[3],x[3],y[3];for(unsigned i=0;i<3;++i){edge[i]=nflstreet_gk_subs(c[i],b[i]);pb[i]=nflstreet_gk_subs(p[i],b[i]);ab[i]=nflstreet_gk_subs(a[i],b[i]);}
 cross(edge,pb,x);cross(edge,ab,y);float v=nflstreet_gk_muls(x[1],y[1]);v=nflstreet_gk_fmadds(x[0],y[0],v);v=nflstreet_gk_fmadds(x[2],y[2],v);return v>=epsilon;}
static int spatial(nflstreet_draw *d,uint32_t r13,const float p[3],uint32_t *result){
 uint32_t world=word(d,r13-0x7054u),node=word(d,world+0x98);if(!world||!node)return 0;
 for(unsigned steps=0;steps<65536;++steps){const uint8_t *n=at(d,node,32);if(!n)return 0;uint32_t count=rd32(n+24);
  if(count){uint32_t ids=rd32(n+28),base=word(d,world+0x1c),vertices=word(d,world+0x14);
   if(count>65536)return 0;for(uint32_t i=0;i<count;++i){uint32_t t=base+word(d,ids+4*i)*0x24;const uint8_t *tri=at(d,t,6);if(!tri)return 0;
    float v[3][3];for(unsigned j=0;j<3;++j){uint32_t a=vertices+8u*nflstreet_rd16(tri+2*j);v[j][0]=real(d,a);v[j][1]=real(d,a+4);v[j][2]=real(d,0x80254ff8u);}
    float e=real(d,0x80254ff4u);if(halfspace(p,v[0],v[1],v[2],e)&&halfspace(p,v[1],v[0],v[2],e)&&halfspace(p,v[2],v[0],v[1],e)){*result=t;return 1;}}
   *result=0;return 1;}
  unsigned axis=rd32(n)==0?0:1;node=rd32(n+(p[axis]>nflstreet_as_float(rd32(n+4))?16:20));}
 return 0;}
/* The original push/load/scale/concat/translate/rotate/pop writes the next
   stack slot even after popping. Reproduce both attachment passes. */
static int scratch(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t root,uint32_t model,int selector,int scaled){
 if(selector==-1)return 1;float unit[2]={real(d,0x803a3958u),real(d,0x803a395cu)},scale=real(d,model+0x18);
 if(!nflstreet_matrix_push(d,k)||!nflstreet_matrix_load(d,k,root)||(scaled&&!nflstreet_matrix_scale(d,k,scale,scale,scale))||
 !nflstreet_matrix_concat(d,k,word(d,model+0x60)+64u*(unsigned)selector,unit)||
 !nflstreet_matrix_translate(d,k,real(d,model+0x40),real(d,model+0x44),real(d,model+0x48)))return 0;
 for(unsigned i=0;i<3;++i)if(!nflstreet_matrix_rotate(d,k,2-i,word(d,model+0x58-4*i)))return 0;
 return nflstreet_matrix_pop(d,k);}
int nflstreet_ball_update(const nflstreet_ball_update_context *c,uint32_t record,double delta){
 if(!c||!c->memory)return 0;nflstreet_draw d={c->memory,NULL};uint8_t *r=at(&d,record,244);if(!r)return 0;
 uint32_t visual=rd32(r);uint8_t *v=at(&d,visual,0x298);if(!v)return 0;
 nflstreet_ball_pose previous;for(unsigned i=0;i<3;++i)previous.position[i]=real(&d,visual+4+4*i);for(unsigned i=0;i<4;++i)previous.quaternion[i]=real(&d,visual+0x6c+4*i);
 uint32_t actor=resolve(&d,c->r13,rd32(r+0xb4));nflstreet_ball_attachment_inputs inputs,*attachment=NULL;
 float world[12],root[12],local[3],bones[256][12];int32_t rotation[3];int selector=-1;uint32_t model=0;
 if(actor){model=word(&d,actor+4);const uint8_t *m=at(&d,model,0x3ec);if(!m)return 0;selector=(int16_t)nflstreet_rd16(m+0x32);
  if(selector< -1||selector>=256)return 0;unsigned count=selector<0?0:(unsigned)selector+1;
  for(unsigned i=0;i<12;++i){world[i]=real(&d,actor+0x280+4*i);root[i]=real(&d,model+0x3ac+4*i);}
  for(unsigned i=0;i<3;++i){local[i]=real(&d,model+0x40+4*i);rotation[i]=(int32_t)word(&d,model+0x50+4*i);}
  uint32_t clusters=word(&d,model+0x60);if(count&&!at(&d,clusters,64*count))return 0;
  for(unsigned b=0;b<count;++b)for(unsigned i=0;i<12;++i)bones[b][i]=real(&d,clusters+64*b+4*i);
  inputs=(nflstreet_ball_attachment_inputs){world,root,real(&d,model+0x18),bones,count,selector,local,rotation};attachment=&inputs;}
 /* Frame kernel's pointer/nonzero check is stricter than 80020944: invalid
    class and null-table handles legitimately select the detached path. */
 uint8_t next[244];memcpy(next,r,244);if(!actor)memset(next+0xb4,0,4);
 nflstreet_ball_frame_output output;if(!nflstreet_ball_frame(next,244,delta,real(&d,c->r13-0x73b0u),attachment,&previous,&output))return 0;
 memcpy(next+0xb4,r+0xb4,4);
 uint32_t lighting;if(!spatial(&d,c->r13,output.visual_pose.position,&lighting))return 0;
 if(actor&&(!scratch(&d,&c->stacks,actor+0x280,model,selector,0)||!scratch(&d,&c->stacks,model+0x3ac,model,selector,1)))return 0;
 memcpy(r,next,244);if(actor){for(unsigned i=0;i<3;++i)pf(v+4+4*i,output.visual_pose.position[i]);for(unsigned i=0;i<4;++i)pf(v+0x6c+4*i,output.visual_pose.quaternion[i]);}
 put(v+0x294,lighting);return 1;}
static void multiply(const float a[4],const float b[4],float q[4]){
 q[0]=nflstreet_gk_subs(nflstreet_gk_fmadds(a[1],b[2],nflstreet_gk_fmadds(a[3],b[0],nflstreet_gk_muls(a[0],b[3]))),nflstreet_gk_muls(a[2],b[1]));
 q[1]=nflstreet_gk_subs(nflstreet_gk_fmadds(a[2],b[0],nflstreet_gk_fmadds(a[3],b[1],nflstreet_gk_muls(a[1],b[3]))),nflstreet_gk_muls(a[0],b[2]));
 q[2]=nflstreet_gk_subs(nflstreet_gk_fmadds(a[0],b[1],nflstreet_gk_fmadds(a[3],b[2],nflstreet_gk_muls(a[2],b[3]))),nflstreet_gk_muls(a[1],b[0]));
 q[3]=nflstreet_gk_subs(nflstreet_gk_subs(nflstreet_gk_fmsubs(a[3],b[3],nflstreet_gk_muls(a[0],b[0])),nflstreet_gk_muls(a[1],b[1])),nflstreet_gk_muls(a[2],b[2]));}
int nflstreet_ball_update_visual(const nflstreet_ball_update_context *c,uint32_t record){
 if(!c||!c->memory)return 0;nflstreet_draw d={c->memory,NULL};const uint8_t *r=at(&d,record,244);if(!r)return 0;uint8_t *v=at(&d,rd32(r),0x7c);if(!v)return 0;
 uint32_t side=word(&d,c->r13-0x7078u);const uint8_t *s=side?at(&d,side+0x39,1):NULL;if(side&&!s)return 0;int flip=s&&*s==1;
 float coefficients[8],q[4],a[4],b[4],out[4];for(unsigned i=0;i<8;++i)coefficients[i]=real(&d,0x8039e100u+4*i);for(unsigned i=0;i<4;++i)q[i]=nflstreet_as_float(rd32(r+0x18+4*i));
 if(flip){nflstreet_801a36ec_quaternion(0x800000,0,0,coefficients,a);multiply(a,q,b);memcpy(q,b,sizeof q);}
 nflstreet_801a36ec_quaternion(0x400000,0,0x400000,coefficients,a);multiply(q,a,out);
 for(unsigned i=0;i<3;++i)put(v+4+4*i,rd32(r+12+4*i)^((flip&&i<2)?0x80000000u:0));for(unsigned i=0;i<4;++i)pf(v+0x6c+4*i,out[i]);return 1;}
int nflstreet_ball_update_matrix(const nflstreet_ball_update_context *c,uint32_t visual){
 if(!c||!c->memory)return 0;nflstreet_draw d={c->memory,NULL};uint8_t *v=at(&d,visual,0x7c);if(!v)return 0;
 float q[4];for(unsigned i=0;i<4;++i)q[i]=real(&d,visual+0x6c+4*i);
 float x=q[0]+q[0],y=q[1]+q[1],z=q[2]+q[2],xx=x*q[0],yy=y*q[1],zz=z*q[2],xw=x*q[3],yw=y*q[3],zw=z*q[3],xy=x*q[1],xz=x*q[2],yz=y*q[2];
 float m[12]={1-(yy+zz),xy-zw,xz+yw,real(&d,visual+4),xy+zw,1-(xx+zz),yz-xw,real(&d,visual+8),xz-yw,yz+xw,1-(xx+yy),real(&d,visual+12)};
 for(unsigned i=0;i<12;++i)pf(v+0x20+4*i,m[i]);return 1;}
