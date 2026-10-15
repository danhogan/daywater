#include "actor_dynamics.h"
#define rd32 nflstreet_rd32
static uint8_t *at(nflstreet_actor_dynamics_context *c,uint32_t a,uint32_t n){return c->x->span(c->x->memory,a,n);}
static float f(uint8_t *p){return nflstreet_as_float(rd32(p));}
static void pf(uint8_t *p,float v){nflstreet_wr32(p,nflstreet_float_bits(v));}
static int word(nflstreet_actor_dynamics_context *c,uint32_t a,uint32_t *v){uint8_t *p=at(c,a,4);if(!p)return 0;*v=rd32(p);return 1;}
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
 if(!s[2]){for(unsigned i=0;i<3;++i){pf(s+4+4*i,pos[i]);pi(c,state+0x28+4*i,(int32_t)(int16_t)nflstreet_rd16(at(c,model+0x416+2*i,2))*256);pi(c,state+0x34+4*i,0);}}
 nflstreet_dynamics_sub(delta,pos,(float[]){f(s+4),f(s+8),f(s+12)});if(!s[2]){for(unsigned i=0;i<3;++i)pf(s+0x10+4*i,delta[i]);s[2]=1;}
 nflstreet_dynamics_sub(error,delta,(float[]){f(s+0x10),f(s+0x14),f(s+0x18)});float dot=-nflstreet_dynamics_dot(error,base),length=nflstreet_dynamics_length(error);int16_t z=(int16_t)nflstreet_rd16(at(c,model+0x41a,2));float k=f(at(c,z<1?0x80246d0cu:0x80246d08u,4));
 int32_t turns[3]={(int32_t)(rd32(s+0x28)+rd32(s+0x34)),(int32_t)(rd32(s+0x2c)+rd32(s+0x38)),(int32_t)(rd32(s+0x30)+rd32(s+0x3c))};
 int32_t a=nflstreet_fctiwz(length*dot*f(at(c,0x803a1ef0u,4))*k),b=nflstreet_fctiwz(f(at(c,0x803a1ef4u,4))*base[2]*k);pi(c,state+0x3c,(int32_t)(rd32(s+0x3c)+(uint32_t)a+(uint32_t)b));
 uint8_t *limits=at(c,z<1?0x8026da34u:0x8026da1cu,8);if(!limits)return 0;if(turns[2]<(int32_t)rd32(limits)){turns[2]=(int32_t)rd32(limits);pi(c,state+0x3c,0);}else if(turns[2]>(int32_t)rd32(limits+4)){turns[2]=(int32_t)rd32(limits+4);pi(c,state+0x3c,0);}
 for(unsigned i=0;i<3;++i){nflstreet_wr16(at(c,angles+6+2*i,2),(uint16_t)(turns[i]>>8));pf(s+0x10+4*i,delta[i]);pf(s+4+4*i,pos[i]);pi(c,state+0x28+4*i,turns[i]);}
 return 1;
}
/* Original locals are host locals.  This span overlay lets the verified
   RAM-output angle helper fill a local buffer without borrowing game RAM. */
typedef struct {const nflstreet_player_draw_context *x;uint8_t bytes[12];} angle_local;
static uint8_t *angle_span(void *memory,uint32_t a,uint32_t n){angle_local *l=memory;
 if(a>=0xffffffe0u&&(uint64_t)a-0xffffffe0u+n<=sizeof l->bytes)return l->bytes+(a-0xffffffe0u);
 return l->x->span(l->x->memory,a,n);}
static int local_angles(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t out[3]){
 angle_local l={d->x,{0}};nflstreet_player_draw_context x=*d->x;x.memory=&l;x.span=angle_span;nflstreet_draw dd={&x,d->s};
 if(!nflstreet_dynamics_angles(&dd,k,0xffffffe0u))return 0;for(unsigned i=0;i<3;++i)out[i]=rd32(l.bytes+4*i);return 1;}
/* 8004BED8 on a host local, high bound first as in the instruction stream. */
static int limit_local(nflstreet_actor_dynamics_context *c,uint32_t table,uint32_t *value){
 uint8_t *p=at(c,table,8);if(!p)return -1;int32_t x=(int32_t)*value,hi=(int32_t)rd32(p+4),lo=(int32_t)rd32(p);
 if(x>hi){*value=(uint32_t)hi;return 1;}if(x<lo){*value=(uint32_t)lo;return 1;}return 0;}
/* Probable from 8004C268 disassembly; compiled, pending original check. */
int nflstreet_actor_head_dynamics(nflstreet_actor_dynamics_context *c,uint32_t actor,uint32_t state,int enabled){
 nflstreet_draw d={c?c->x:NULL,NULL};uint8_t *s;uint32_t skeleton,pose,rest,ang[3];
 if(!c||!c->x||!c->x->span||!(s=at(c,state,0x13c)))return 0;
 if(!enabled){s[2]=0;return 1;}const int first=s[2]==0;
 if(!word(c,actor+0x60,&skeleton)||!word(c,state+0x130,&pose)||!word(c,state+0x138,&rest)||!at(c,rest+0x41c,0x66)||!at(c,pose+12,0x66))return 0;
 float unit[2]={f(at(c,0x803a3958u,4)),f(at(c,0x803a395cu,4))},axes[2][3],pos[3],delta[3],error[3];
 if(!nflstreet_matrix_push(&d,&c->stacks)||!nflstreet_matrix_concat(&d,&c->stacks,skeleton+0x40u*s[1],unit))return 0;
 for(unsigned axis=0;axis<2;++axis){float source[3];for(unsigned i=0;i<3;++i)source[i]=f(at(c,(axis?0x80246d10u:0x80246cf8u)+4*i,4));
  if(!nflstreet_dynamics_rotate(&d,&c->stacks,axes[axis],source))return 0;}
 if(!nflstreet_dynamics_top_translation(&d,&c->stacks,pos)||!local_angles(&d,&c->stacks,ang))return 0;
 if(first)for(unsigned i=0;i<3;++i){pf(s+4+4*i,pos[i]);nflstreet_wr32(s+0x1c+4*i,ang[i]);}
 nflstreet_dynamics_sub(delta,pos,(float[]){f(s+4),f(s+8),f(s+12)});
 if(first)for(unsigned i=0;i<3;++i)pf(s+0x10+4*i,delta[i]);
 nflstreet_dynamics_sub(error,delta,(float[]){f(s+0x10),f(s+0x14),f(s+0x18)});
 float length=nflstreet_dynamics_length(error),dot_z=-nflstreet_dynamics_dot(error,axes[0]),dot_x=-nflstreet_dynamics_dot(error,axes[1]);
 float sn,cs,look[3],turned[3],zero=f(at(c,0x80246d1cu,4)),gravity=-f(at(c,0x803a1efcu,4));
 if(!nflstreet_sine(&d,&c->stacks,rd32(s+0x40),0,&sn)||!nflstreet_sine(&d,&c->stacks,rd32(s+0x40),1,&cs))return 0;
 look[0]=zero;look[1]=sn;look[2]=-cs;if(!nflstreet_dynamics_rotate(&d,&c->stacks,turned,look))return 0;float force_x=gravity*turned[2];
 if(!nflstreet_sine(&d,&c->stacks,rd32(s+0x48),1,&cs)||!nflstreet_sine(&d,&c->stacks,rd32(s+0x48),0,&sn))return 0;
 look[0]=-cs;look[1]=sn;look[2]=zero;if(!nflstreet_dynamics_rotate(&d,&c->stacks,turned,look))return 0;float force_z=gravity*turned[2];
 if(!nflstreet_matrix_pop(&d,&c->stacks))return 0;
 float spring=f(at(c,0x803a1ef8u,4)),scale=f(at(c,0x80246d20u,4));
 uint32_t target[3]={(uint32_t)nflstreet_fctiwz(((length*dot_x)*spring)*scale)+(uint32_t)nflstreet_fctiwz(force_x*scale),0,
  (uint32_t)nflstreet_fctiwz(((length*dot_z)*spring)*scale)+(uint32_t)nflstreet_fctiwz(force_z*scale)};
 for(unsigned joint=0;joint<9;++joint){uint8_t *p=s+12*joint;uint32_t turns[3];
  if(first)for(unsigned i=0;i<3;++i){nflstreet_wr32(p+0x28+4*i,(uint32_t)((int32_t)(int16_t)nflstreet_rd16(at(c,rest+0x41c+12*joint+2*i,2))*256));nflstreet_wr32(p+0x94+4*i,0);}
  for(unsigned i=0;i<3;++i)turns[i]=rd32(p+0x28+4*i)+rd32(p+0x94+4*i);
  /* 8004C634/6B4/6FC: integer -> single, fused damping, fctiwz. */
  for(unsigned i=0;i<3;++i){uint32_t velocity=(uint32_t)nflstreet_fctiwz(nflstreet_fmadds((float)(int32_t)rd32(p+0x94+4*i),f(at(c,0x803a1f00u,4)),(float)(int32_t)target[i]));
   if(limit_local(c,0x8026da04u,&velocity)<0)return 0;nflstreet_wr32(p+0x94+4*i,velocity);}
  for(unsigned i=0;i<3;++i){int clipped=limit_local(c,0x8026d91cu+24*joint+8*i,turns+i);if(clipped<0)return 0;if(clipped)nflstreet_wr32(p+0x94+4*i,0);
   nflstreet_wr16(at(c,pose+12+12*joint+2*i,2),(uint16_t)(turns[i]>>8));nflstreet_wr32(p+0x28+4*i,turns[i]);}
 }
 for(unsigned i=0;i<3;++i){pf(s+0x10+4*i,delta[i]);pf(s+4+4*i,pos[i]);nflstreet_wr32(s+0x1c+4*i,ang[i]);}
 if(first)s[2]=1;return 1;
}
