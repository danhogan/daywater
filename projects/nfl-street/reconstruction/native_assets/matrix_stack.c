#include "matrix_stack.h"
#include <math.h>
#define rd32 nflstreet_rd32
static uint8_t *at(nflstreet_draw *d,uint32_t token,uint32_t size){return nflstreet_draw_at(d,token,size);}
static int word(nflstreet_draw *d,uint32_t token,uint32_t *v){return nflstreet_draw_word(d,token,v);}
static float fword(const uint8_t *p){return nflstreet_as_float(rd32(p));}
static void put(uint8_t *p,float f){nflstreet_wr32(p,nflstreet_float_bits(f));}
int nflstreet_matrix_select(nflstreet_draw *d,const nflstreet_matrix_stacks *k){
 const uint8_t *index=at(d,k->index,1);uint8_t *current=at(d,k->current,4);if(!index||!current)return 0;
 nflstreet_wr32(current,*index<k->count?k->table+12u**index:0);return 1;}
static int stack(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t *s){return word(d,k->current,s)&&*s;}
int nflstreet_matrix_top(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t *top){
 uint32_t s,used,depth,slots;if(!stack(d,k,&s)||!word(d,s,&used)||!used||!word(d,s+4,&depth)||!word(d,s+8,&slots))return 0;
 *top=slots+0x40u*depth;return 1;}
static int depth_add(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t step,uint32_t *depth,uint32_t *slots){
 uint32_t s;uint8_t *p;if(!stack(d,k,&s)||!(p=at(d,s+4,4))||!word(d,s+8,slots))return 0;*depth=rd32(p)+step;nflstreet_wr32(p,*depth);return 1;}
int nflstreet_matrix_push(nflstreet_draw *d,const nflstreet_matrix_stacks *k){
 uint32_t depth,slots;if(!depth_add(d,k,1,&depth,&slots))return 0;
 uint8_t *to=at(d,slots+0x40u*depth,0x40),*from=at(d,slots+0x40u*(depth-1),0x40);if(!to||!from)return 0;memmove(to,from,0x40);return 1;}
int nflstreet_matrix_pop(nflstreet_draw *d,const nflstreet_matrix_stacks *k){uint32_t depth,slots;return depth_add(d,k,0xffffffffu,&depth,&slots);}
/* 80189328: identity (0.0 from 80261800, 1.0 from 80261804). */
int nflstreet_matrix_reset(nflstreet_draw *d,const nflstreet_matrix_stacks *k){
 uint32_t s,slots;uint8_t *p,*m;if(!stack(d,k,&s)||!(p=at(d,s+4,4))||!word(d,s+8,&slots)||!(m=at(d,slots,0x40)))return 0;
 nflstreet_wr32(p,0);for(unsigned i=0;i<16;++i)put(m+4*i,i%5==0?1.0f:0.0f);return 1;}
int nflstreet_matrix_translate(nflstreet_draw *d,const nflstreet_matrix_stacks *k,float x,float y,float z){
 uint32_t top;uint8_t *m;if(!nflstreet_matrix_top(d,k,&top)||!(m=at(d,top,0x30)))return 0;
 for(unsigned r=0;r<3;++r){uint8_t *q=m+16*r;
  put(q+12,fword(q+12)+nflstreet_fmadds(z,fword(q+8),nflstreet_fmadds(x,fword(q),y*fword(q+4))));}
 return 1;}
int nflstreet_matrix_load(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t matrix){
 uint32_t top;uint8_t *t;const uint8_t *m;if(!nflstreet_matrix_top(d,k,&top)||!(t=at(d,top,0x40))||!(m=at(d,matrix,0x40)))return 0;
 memmove(t,m,0x40);return 1;}
int nflstreet_matrix_concat(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t matrix,const float unit[2]){
 uint32_t top;uint8_t *t;const uint8_t *m;if(!nflstreet_matrix_top(d,k,&top)||!(t=at(d,top,0x30))||!(m=at(d,matrix,0x30)))return 0;
 float a[12],b[12],r[12];for(unsigned i=0;i<12;++i){a[i]=fword(t+4*i);b[i]=fword(m+4*i);}
 for(unsigned row=0;row<3;++row){const float *q=a+4*row;
  for(unsigned c=0;c<4;++c){float acc=b[c]*q[0];acc=nflstreet_fmadds(b[4+c],q[1],acc);acc=nflstreet_fmadds(b[8+c],q[2],acc);
   if(c>=2)acc=nflstreet_fmadds(unit[c-2],q[3],acc);r[4*row+c]=acc;}}
 for(unsigned i=0;i<12;++i)put(t+4*i,r[i]);return 1;}
int nflstreet_matrix_rotate_vector(nflstreet_draw *d,const nflstreet_matrix_stacks *k,const float v[3],float out[3]){
 uint32_t top;const uint8_t *t;if(!nflstreet_matrix_top(d,k,&top)||!(t=at(d,top,0x30)))return 0;
 for(unsigned r=0;r<3;++r){const uint8_t *q=t+16*r;
  out[r]=nflstreet_fmadds(fword(q+8),v[2],nflstreet_fmadds(fword(q),v[0],fword(q+4)*v[1]));}
 return 1;}
float nflstreet_recip_sqrt(float x,float epsilon,float one,float zero_value){
 float s=sqrtf(x);if(s<=epsilon)return zero_value;return one/s;}
/* Columns scaled by x, y, z (a double product rounded to single equals the
   single product of two singles). */
int nflstreet_matrix_scale(nflstreet_draw *d,const nflstreet_matrix_stacks *k,float x,float y,float z){
 uint32_t top;uint8_t *m;if(!nflstreet_matrix_top(d,k,&top)||!(m=at(d,top,0x30)))return 0;
 const float s[3]={x,y,z};for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)put(m+16*r+4*c,fword(m+16*r+4*c)*s[c]);
 return 1;}
int nflstreet_sine(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t angle,int cosine,float *out){
 const uint8_t *c=at(d,k->sine,32);if(!c)return 0;
 if(cosine)angle+=0x400000u;
 const float t=(float)(int32_t)(angle<<8)/256.0f; /* the low 24 bits, sign-extended (exact) */
 float x=t*k->angle_scale[cosine?1:0],x2=x*x,x3=x*x2,x4=x2*x2;
 float a0=x,a1=x3,b0=a0*x4,b1=a1*x4,s0=fword(c)*a0,s1=fword(c+4)*a1;
 s0=nflstreet_fmadds(fword(c+8),b0,s0);s1=nflstreet_fmadds(fword(c+12),b1,s1);
 float e0=b0*x4,e1=b1*x4;s0=nflstreet_fmadds(fword(c+16),e0,s0);s1=nflstreet_fmadds(fword(c+20),e1,s1);
 s0=nflstreet_fmadds(fword(c+24),e0*x4,s0);s1=nflstreet_fmadds(fword(c+28),e1*x4,s1);
 *out=s0+s1;return 1;}
/* Axis 0 (80189854) mixes columns 1 and 2, axis 1 (80189944) 0 and 2,
   axis 2 (80189A34) 0 and 1: new_i = m_i*c + m_j*a, new_j = m_i*b + m_j*c
   with (a, b) = (s, -s), (-s, s), (s, -s). */
int nflstreet_matrix_rotate(nflstreet_draw *d,const nflstreet_matrix_stacks *k,unsigned axis,uint32_t angle){
 static const unsigned cols[3][2]={{1,2},{0,2},{0,1}};uint32_t top;uint8_t *m;float s,c;
 if(axis>2||!nflstreet_matrix_top(d,k,&top)||!(m=at(d,top,0x30))||!nflstreet_sine(d,k,angle,0,&s)||!nflstreet_sine(d,k,angle,1,&c))return 0;
 const float a=axis==1?-s:s,b=axis==1?s:-s;const unsigned i=cols[axis][0],j=cols[axis][1];
 for(unsigned r=0;r<3;++r){uint8_t *q=m+16*r;const float mi=fword(q+4*i),mj=fword(q+4*j);
  put(q+4*i,nflstreet_fmadds(mi,c,mj*a));put(q+4*j,nflstreet_fmadds(mi,b,mj*c));}
 return 1;}
int nflstreet_matrix_store(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t to){
 uint32_t top;uint8_t *t;const uint8_t *f;if(!nflstreet_matrix_top(d,k,&top)||!(f=at(d,top,0x40))||!(t=at(d,to,0x40)))return 0;
 memmove(t,f,0x40);return 1;}
int nflstreet_matrix_load_top(nflstreet_draw *d,const nflstreet_matrix_stacks *k){
 uint32_t top,m[12];const uint8_t *t;if(!nflstreet_matrix_top(d,k,&top)||!(t=at(d,top,0x30)))return 0;
 for(unsigned i=0;i<12;++i)m[i]=rd32(t+4*i);return nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12);}
