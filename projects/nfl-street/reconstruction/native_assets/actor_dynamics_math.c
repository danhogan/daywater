#include "actor_dynamics_math.h"
#include <math.h>
#define rd32 nflstreet_rd32
static float f(const uint8_t *p){return nflstreet_as_float(rd32(p));}
static int constant(nflstreet_draw *d,uint32_t a,float *v){const uint8_t *p=nflstreet_draw_at(d,a,4);if(!p)return 0;*v=f(p);return 1;}
static float absf(float x){return x<0.0f?-x:x;}
uint32_t nflstreet_dynamics_atan(nflstreet_draw *d,float y,float x){
 float zero,scale,c[9];if(!constant(d,0x802617d0u,&zero)||!constant(d,0x802617d4u,&scale))return 0;
 for(unsigned i=0;i<9;++i)if(!constant(d,0x8029194cu+4*i,c+i))return 0;
 float ay=absf(y),ax=absf(x),q=(ay-ax)/(ay+ax);int32_t quadrant=0;
 if(y>=zero){if(x<zero){quadrant=0x400000;q=-q;}}else{quadrant=0x800000;if(x>=zero){quadrant=-0x400000;q=-q;}}
 float q2=q*q,q3=q*q2,q5=q3*q2,q7=q5*q2,q9=q7*q2,q11=q9*q2,q13=q11*q2,q15=q13*q2;
 float p=nflstreet_fmadds(q,c[0],q3*c[1]);p=nflstreet_fmadds(q5,c[2],p);p=nflstreet_fmadds(q7,c[3],p);p=nflstreet_fmadds(q9,c[4],p);
 p=nflstreet_fmadds(q11,c[5],p);p=nflstreet_fmadds(q13,c[6],p);p=nflstreet_fmadds(q15,c[7],p);p=p+c[8];
 int32_t result=nflstreet_fctiwz(p*scale)+quadrant;return (uint32_t)((int32_t)((uint32_t)result<<8)>>8);
}
float nflstreet_dynamics_length(const float v[3]){return sqrtf(nflstreet_fmadds(v[2],v[2],nflstreet_fmadds(v[0],v[0],v[1]*v[1])));}
void nflstreet_dynamics_sub(float out[3],const float a[3],const float b[3]){for(unsigned i=0;i<3;++i)out[i]=a[i]-b[i];}
float nflstreet_dynamics_dot(const float a[3],const float b[3]){return nflstreet_fmadds(a[2],b[2],nflstreet_fmadds(a[0],b[0],a[1]*b[1]));}
int nflstreet_dynamics_normalize(nflstreet_draw *d,float out[3],const float in[3]){
 float epsilon,one,zero;if(!constant(d,0x802617d8u,&epsilon)||!constant(d,0x802617dcu,&one)||!constant(d,0x802617e0u,&zero))return 0;
 float n=nflstreet_dynamics_length(in),k=n>epsilon?one/n:zero;for(unsigned i=0;i<3;++i)out[i]=in[i]*k;return 1;
}
int nflstreet_dynamics_top_translation(nflstreet_draw *d,const nflstreet_matrix_stacks *k,float out[3]){uint32_t a;const uint8_t *p;if(!nflstreet_matrix_top(d,k,&a)||!(p=nflstreet_draw_at(d,a,0x30)))return 0;out[0]=f(p+12);out[1]=f(p+28);out[2]=f(p+44);return 1;}
int nflstreet_dynamics_rotate(nflstreet_draw *d,const nflstreet_matrix_stacks *k,float out[3],const float in[3]){return nflstreet_matrix_rotate_vector(d,k,in,out);}
int nflstreet_dynamics_angles(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t out){
 uint32_t a;const uint8_t *p;float one,cut,zero;if(!nflstreet_matrix_top(d,k,&a)||!(p=nflstreet_draw_at(d,a,0x30))||!constant(d,0x80261814u,&one)||!constant(d,0x80261818u,&cut)||!constant(d,0x8026181cu,&zero))return 0;
 float m[12];for(unsigned i=0;i<12;++i)m[i]=f(p+4*i);float z=-m[8],q=z*z;if(q>one)q=one;float root=sqrtf(one-q),first,second,third,fourth;
 if(root>cut){float max,limit,scale;if(!constant(d,0x802617e4u,&max)||!constant(d,0x802617e8u,&limit)||!constant(d,0x802617ecu,&scale))return 0;float k=root>limit?scale/root:max;first=m[9]*k;second=m[10]*k;third=m[4]*k;fourth=m[0]*k;}else{first=-m[6];second=m[5];third=zero;fourth=root;}
 uint8_t *qout=nflstreet_draw_at(d,out,12);if(!qout)return 0;nflstreet_wr32(qout,nflstreet_dynamics_atan(d,first,second));nflstreet_wr32(qout+4,nflstreet_dynamics_atan(d,z,root));nflstreet_wr32(qout+8,nflstreet_dynamics_atan(d,third,fourth));return 1;
}
int nflstreet_dynamics_clamp(nflstreet_draw *d,uint32_t limits,int32_t offset,uint32_t value){uint8_t *p=nflstreet_draw_at(d,value,4);const uint8_t *t=nflstreet_draw_at(d,limits,8);if(!p||!t)return 0;int32_t x=(int32_t)rd32(p)-offset,lo=(int32_t)rd32(t),hi=(int32_t)rd32(t+4);if(x>hi){nflstreet_wr32(p,(uint32_t)(hi+offset));return 1;}if(x<lo){nflstreet_wr32(p,(uint32_t)(lo+offset));return 1;}nflstreet_wr32(p,(uint32_t)(x+offset));return 0;}
