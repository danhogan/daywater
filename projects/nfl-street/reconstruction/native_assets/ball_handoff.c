#include "ball_handoff.h"
#include "../ball/ball_ownership.h"
#include "gekko_float.h"
#include <math.h>
#include <string.h>
static uint8_t *at(nflstreet_draw *d,uint32_t a,uint32_t n){return nflstreet_draw_at(d,a,n);}
static uint32_t word(nflstreet_draw *d,uint32_t a){uint32_t v=0;nflstreet_draw_word(d,a,&v);return v;}
static float real(nflstreet_draw *d,uint32_t a){return nflstreet_as_float(word(d,a));}
static void put(uint8_t *p,uint32_t v){nflstreet_wr32(p,v);}
static void pf(uint8_t *p,float v){put(p,nflstreet_float_bits(v));}
static uint32_t elapsed(nflstreet_draw *d,uint32_t r13){uint32_t p=word(d,r13-0x61f0u);return p?word(d,p+4)-word(d,p):0;}
static int old_supported(uint32_t s){return s<=9&&s!=4&&s!=7;}
static uint32_t resolve(nflstreet_draw *d,uint32_t r13,uint32_t h){
 if((h&255)!=1)return 0;uint32_t t=word(d,r13-0x7c30u);const uint8_t *p=t?at(d,t,12):NULL;if(!p)return 0;
 return nflstreet_rd32(p)+(((h>>8)&255)*nflstreet_rd16(p+8)+((h>>16)&255))*0x14d4u;}
/* Both original attachment passes write the popped matrix slot. */
static int scratch(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t root,uint32_t model,int selector,int scaled){
 if(selector==-1)return 1;float unit[2]={real(d,0x803a3958u),real(d,0x803a395cu)},s=real(d,model+0x18);
 if(!nflstreet_matrix_push(d,k)||!nflstreet_matrix_load(d,k,root)||(scaled&&!nflstreet_matrix_scale(d,k,s,s,s))||
 !nflstreet_matrix_concat(d,k,word(d,model+0x60)+64u*(unsigned)selector,unit)||
 !nflstreet_matrix_translate(d,k,real(d,model+0x40),real(d,model+0x44),real(d,model+0x48)))return 0;
 for(unsigned i=0;i<3;++i)if(!nflstreet_matrix_rotate(d,k,2-i,word(d,model+0x58-4*i)))return 0;
 return nflstreet_matrix_pop(d,k);}
int nflstreet_ball_handoff_transition(const nflstreet_ball_update_context *c,uint32_t ball,uint32_t state,uint32_t arg){
 if(!c||!c->memory)return 0;nflstreet_draw d={c->memory,NULL};uint8_t *r=at(&d,ball,244);if(!r)return 0;
 uint32_t old=nflstreet_rd32(r+0xa4);if(!old_supported(old)||(state>3&&(state!=5||arg)))return 0;
 put(r+0xa4,state);put(r+0xb0,nflstreet_rd32(r+0xa8));put(r+0xa8,arg);put(r+0xac,old);
 if(old==1){put(r+0xc0,elapsed(&d,c->r13));put(r+0xcc,nflstreet_rd32(r+0xb4));put(r+0xbc,0);}
 else if(old==6||old==8){put(r+0xe0,word(&d,0x802521c0u));put(r+8,nflstreet_rd32(r+8)&~8u);}
 if(state==1){put(r+0xbc,0);put(r+0xc4,1);put(r+8,nflstreet_rd32(r+8)&~0x28u);}
 /* 800C1874 returns without event services when entry-5 argument is zero. */
 return 1;}
int nflstreet_ball_handoff_release(const nflstreet_ball_update_context *c,uint32_t ball,uint32_t state,uint32_t arg,unsigned keep){
 if(keep>1||!nflstreet_ball_handoff_transition(c,ball,state,arg))return 0;nflstreet_draw d={c->memory,NULL};uint8_t *r=at(&d,ball,244);
 uint32_t zero=word(&d,keep?0x80251facu:0x80251fa8u);
 if(!keep)for(unsigned i=0;i<3;++i)put(r+0x54+4*i,zero);
 for(unsigned i=0;i<3;++i)put(r+0x7c+4*i,zero);put(r+0xb4,0);return 1;}
int nflstreet_ball_handoff_assign(const nflstreet_ball_update_context *c,uint32_t ball,uint32_t actor){
 if(!c||!c->memory)return 0;nflstreet_draw d={c->memory,NULL};uint8_t *r=at(&d,ball,244),*a=at(&d,actor,0x2c0);if(!r||!a)return 0;
 uint32_t header=nflstreet_rd32(a),model=word(&d,actor+4),query=word(&d,c->r13-0x7cf8u);
 if(resolve(&d,c->r13,header)!=actor||!old_supported(nflstreet_rd32(r+0xa4))||!at(&d,query,4))return 0;
 const uint8_t *m=at(&d,model,0x3ec);uint8_t *v=at(&d,nflstreet_rd32(r),0x7c);if(!m||!v)return 0;
 int selector=(int16_t)nflstreet_rd16(m+0x32);if(selector< -1||selector>=256)return 0;unsigned count=selector<0?0:(unsigned)selector+1;
 float world[12],root[12],local[3],bones[256][12];int32_t rotation[3];uint32_t clusters=word(&d,model+0x60);
 if(count&&!at(&d,clusters,64*count))return 0;
 for(unsigned i=0;i<12;++i){world[i]=real(&d,actor+0x280+4*i);root[i]=real(&d,model+0x3ac+4*i);}
 for(unsigned i=0;i<3;++i){local[i]=real(&d,model+0x40+4*i);rotation[i]=(int32_t)word(&d,model+0x50+4*i);}
 for(unsigned b=0;b<count;++b)for(unsigned i=0;i<12;++i)bones[b][i]=real(&d,clusters+64*b+4*i);
 nflstreet_ball_attachment_inputs inputs={world,root,real(&d,model+0x18),bones,count,selector,local,rotation};
 nflstreet_ball_assignment_context context={word(&d,query),word(&d,c->r13-0x7cf4u),word(&d,c->r13-0x6a00u)};
 uint32_t flags=nflstreet_rd32(a+12);uint8_t next[244];memcpy(next,r,244);nflstreet_ball_pose pose;
 if(!nflstreet_ball_assign(next,244,header,&flags,&context,elapsed(&d,c->r13),&inputs,&pose))return 0;
 if(!scratch(&d,&c->stacks,actor+0x280,model,selector,0)||!scratch(&d,&c->stacks,model+0x3ac,model,selector,1))return 0;
 memcpy(r,next,244);put(a+12,flags);put(at(&d,c->r13-0x7cf4u,4),context.cached_value);
 if(context.value==3)put(at(&d,c->r13-0x6a00u,4),context.actor_word);
 for(unsigned i=0;i<3;++i)pf(v+4+4*i,pose.position[i]);for(unsigned i=0;i<4;++i)pf(v+0x6c+4*i,pose.quaternion[i]);return 1;}
int nflstreet_ball_handoff_snap(const nflstreet_ball_update_context *c,uint32_t actor){
 if(!c||!c->memory)return 0;nflstreet_draw d={c->memory,NULL};uint8_t *a=at(&d,actor,0xbbe);uint32_t pool=word(&d,c->r13-0x73d0u);
 const uint8_t *p=at(&d,pool,8);uint32_t field=word(&d,c->r13-0x7078u);if(!a||!p||!at(&d,field,0x14))return 0;
 /* 800815B4 uses 8008D278's explicit speed branch. Reject a changed
    constant selecting its unreconstructed random/angle branch. */
 float speed=real(&d,0x80250508u);if(speed==real(&d,0x80250a2cu))return 0;
 float scale=real(&d,c->r13-0x70c4u),rate=real(&d,actor+0x1c4);if(!isfinite(scale)||!scale||!isfinite(rate))return 0;
 if(!nflstreet_ball_handoff_assign(c,pool+8+0xf4u*p[1],actor))return 0;
 uint8_t *target=a+0x150;nflstreet_wr16(target,0);
 pf(target+8,nflstreet_gk_adds(real(&d,field+12),real(&d,0x80250500u)));
 pf(target+12,nflstreet_gk_subs(real(&d,field+16),real(&d,0x80250504u)));
 put(target+0x14,word(&d,0x80250a28u));put(target+0x10,0x400000);
 pf(target+0x18,nflstreet_gk_divs(rate,scale));pf(target+0x1c,speed);
 target[3]=0;for(unsigned i=0;i<3;++i)put(target+0x20+4*i,word(&d,0x80250a34u));put(target+4,0);target[2]=0;
 a[0x20e]=1;put(a+0x204,nflstreet_rd32(a+0x1c0));put(a+0x208,nflstreet_rd32(a+0x1c0));put(a+0x200,nflstreet_rd32(target+0x18));
 if((int16_t)nflstreet_rd16(a+0xbbc)<150)put(target+0x1c,word(&d,0x8025050cu));a[0x180]=0;return 1;}
int nflstreet_ball_handoff_event(const nflstreet_ball_update_context *c,uint32_t ball,uint32_t actor){
 if(!ball)return 1;if(!c||!c->memory)return 0;nflstreet_draw d={c->memory,NULL};uint32_t q=word(&d,c->r13-0x7cf8u);
 if(!at(&d,q,4)||word(&d,q)==3)return 0;
 if(!nflstreet_ball_handoff_assign(c,ball,actor))return 0;put(at(&d,ball+0xc4,4),0);put(at(&d,c->r13-0x7cf4u,4),word(&d,q));return 1;}
