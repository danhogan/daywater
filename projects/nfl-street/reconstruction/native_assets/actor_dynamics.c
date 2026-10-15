#include "actor_dynamics.h"
#define rd32 nflstreet_rd32
static uint8_t *at(nflstreet_actor_dynamics_context *c,uint32_t a,uint32_t n){return c->x->span(c->x->memory,a,n);}
static float f(uint8_t *p){return nflstreet_as_float(rd32(p));}
static void pf(uint8_t *p,float v){nflstreet_wr32(p,nflstreet_float_bits(v));}
static int word(nflstreet_actor_dynamics_context *c,uint32_t a,uint32_t *v){uint8_t *p=at(c,a,4);if(!p)return 0;*v=rd32(p);return 1;}
static int32_t si(nflstreet_actor_dynamics_context *c,uint32_t a){uint32_t v;return word(c,a,&v)?(int32_t)v:0;}
static void pi(nflstreet_actor_dynamics_context *c,uint32_t a,int32_t v){uint8_t *p=at(c,a,4);if(p)nflstreet_wr32(p,(uint32_t)v);}
static int stack(nflstreet_actor_dynamics_context *c,nflstreet_draw *d,uint32_t actor,uint32_t state,float base[3],float pos[3],uint32_t angles){
 uint32_t skeleton;float unit[2];if(!word(c,actor+0x60,&skeleton)||!(at(c,0x803a3958u,8)))return 0;unit[0]=f(at(c,0x803a3958u,4));unit[1]=f(at(c,0x803a395cu,4));
 float source[3]={base[0],base[1],base[2]};if(!nflstreet_matrix_push(d,&c->stacks)||!nflstreet_matrix_concat(d,&c->stacks,skeleton+0x40u*at(c,state+1,1)[0],unit)||!nflstreet_dynamics_rotate(d,&c->stacks,base,source)||!nflstreet_dynamics_top_translation(d,&c->stacks,pos)||!nflstreet_dynamics_angles(d,&c->stacks,angles)||!nflstreet_matrix_pop(d,&c->stacks))return 0;return 1;
}
int nflstreet_actor_hand_dynamics(nflstreet_actor_dynamics_context *c,uint32_t actor,uint32_t state,int enabled){
 static nflstreet_render_state unused;nflstreet_draw d={c?c->x:NULL,&unused};uint8_t *s;uint32_t model,angles;float base[3],pos[3],delta[3],error[3];
 if(!c||!c->x||!c->x->span||!(s=at(c,state,0x7c)))return 0;if(!enabled){s[2]=0;return 1;}if(!word(c,state+0x78,&model)||!word(c,state+0x70,&angles))return 0;
 for(unsigned i=0;i<3;++i)base[i]=f(at(c,0x80246cf8u+4*i,4));if((int16_t)nflstreet_rd16(at(c,model+0x41a,2))<0)base[0]=f(at(c,0x80246d04u,4));
 if(!stack(c,&d,actor,state,base,pos,state+0x1c)||!nflstreet_dynamics_normalize(&d,base,base))return 0;
 if(!s[2]){for(unsigned i=0;i<3;++i){pf(s+4+4*i,pos[i]);pi(c,state+0x28+4*i,(int32_t)(int16_t)nflstreet_rd16(at(c,model+0x416+2*i,2))<<8);pi(c,state+0x34+4*i,0);}}
 nflstreet_dynamics_sub(delta,pos,(float[]){f(s+4),f(s+8),f(s+12)});if(!s[2]){for(unsigned i=0;i<3;++i)pf(s+0x10+4*i,delta[i]);s[2]=1;}
 nflstreet_dynamics_sub(error,delta,(float[]){f(s+0x10),f(s+0x14),f(s+0x18)});float dot=-nflstreet_dynamics_dot(error,base),length=nflstreet_dynamics_length(error);int16_t z=(int16_t)nflstreet_rd16(at(c,model+0x41a,2));float k=f(at(c,z<1?0x80246d0cu:0x80246d08u,4));
 int32_t turns[3]={si(c,state+0x28)+si(c,state+0x34),si(c,state+0x2c)+si(c,state+0x38),si(c,state+0x30)+si(c,state+0x3c)};
 int32_t a=nflstreet_fctiwz(length*dot*f(at(c,0x803a1ef0u,4))*k),b=nflstreet_fctiwz(f(at(c,0x803a1ef4u,4))*base[2]*k);pi(c,state+0x3c,si(c,state+0x3c)+a+b);
 uint8_t *limits=at(c,z<1?0x8026da34u:0x8026da1cu,8);if(!limits)return 0;if(turns[2]<(int32_t)rd32(limits)){turns[2]=(int32_t)rd32(limits);pi(c,state+0x3c,0);}else if(turns[2]>(int32_t)rd32(limits+4)){turns[2]=(int32_t)rd32(limits+4);pi(c,state+0x3c,0);}
 for(unsigned i=0;i<3;++i){nflstreet_wr16(at(c,angles+6+2*i,2),(uint16_t)(turns[i]>>8));pf(s+0x10+4*i,delta[i]);pf(s+4+4*i,pos[i]);pi(c,state+0x28+4*i,turns[i]);}
 return 1;
}
int nflstreet_actor_head_dynamics(nflstreet_actor_dynamics_context *c,uint32_t actor,uint32_t state,int enabled){(void)c;(void)actor;(void)state;(void)enabled;return 0;}
