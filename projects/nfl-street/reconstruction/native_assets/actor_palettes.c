#include "actor_palettes.h"
#include "cluster_palette.h"
#include "actor_dynamics.h"
#include "gekko_quaternion/actor_slerp.h"
#include <string.h>
#define rd32 nflstreet_rd32
#define LOCKED 0xe0000000u
typedef nflstreet_actor_palette_context context;
/* RAM through the span; the locked cache from the context. */
static uint8_t *mem(context *c,uint32_t a,uint32_t n){
 if(a>=LOCKED)return (uint64_t)a-LOCKED+n<=sizeof c->locked?c->locked+(a-LOCKED):NULL;
 return a?c->x->span(c->x->memory,a,n):NULL;}
static int word(context *c,uint32_t a,uint32_t *v){const uint8_t *p=mem(c,a,4);if(!p)return 0;*v=rd32(p);return 1;}
static float fword(const uint8_t *p){return nflstreet_as_float(rd32(p));}
static void put(uint8_t *p,float f){nflstreet_wr32(p,nflstreet_float_bits(f));}
/* 801F0DD4: the locked-cache DMA copies whole 32-byte blocks. */
static int dma(context *c,uint32_t to,uint32_t from,uint32_t bytes){
 const uint32_t n=(bytes+0x1fu)&~0x1fu;if(!n)return 1;uint8_t *t=mem(c,to,n);const uint8_t *f=mem(c,from,n);
 if(!t||!f)return 0;memmove(t,f,n);return 1;}
/* 801EDE38: enable the locked cache once. */
static int cache(context *c){uint8_t *on=mem(c,c->cache_on,1);if(!on)return 0;if(!*on)*on=1;return 1;}
/* 801CA4B0: this frame's next matrices (an even count) from the pool. */
static int allocate(context *c,uint32_t desc,uint32_t count){
 uint32_t used,index,base;uint8_t *u=mem(c,c->pool_used,4),*p=mem(c,desc,12);
 if(!u||!p||!word(c,c->pool_index,&index)||!word(c,c->pool+4*index,&base))return 0;
 used=rd32(u);const uint32_t n=(count+1u)&~1u;nflstreet_wr32(u,used+n);nflstreet_wr32(p+8,n);nflstreet_wr32(p,base+used*0x30u);return 1;}
/* 801CA4E8/801CA720: the stack top (16 words) at out, then each cluster. */
static int palette(context *c,nflstreet_draw *d,uint32_t out,uint32_t skel,uint32_t clst,uint32_t pose){
 uint32_t top,bones,translations,rotations,rows,count;const uint8_t *t,*poly;
 if(!nflstreet_matrix_top(d,&c->stacks,&top)||!(t=mem(c,top,0x40))||!word(c,skel,&bones)||!word(c,skel+8,&translations)||
    !word(c,skel+12,&rotations)||!word(c,clst,&rows)||!word(c,clst+4,&count)||!(poly=mem(c,c->stacks.sine,24)))return 0;
 if(!bones||bones>256||!count||count>256)return 0;
 const uint8_t *tr=mem(c,translations,bones*16u),*ro=mem(c,rotations,bones*6u),*rw=mem(c,rows,count*4u),*ps=pose?mem(c,pose,bones*6u):NULL;
 uint8_t *o=mem(c,out,count*0x30u+0x10u);if(!tr||!ro||!rw||(pose&&!ps)||!o)return 0;
 float root[12],coefficients[6],matrices[256][12];int16_t angles[256*3];
 for(unsigned i=0;i<12;++i)root[i]=fword(t+4*i);for(unsigned i=0;i<6;++i)coefficients[i]=fword(poly+4*i);
 if(ps)for(unsigned i=0;i<bones*3u;++i)angles[i]=(int16_t)(uint16_t)(ps[2*i]<<8|ps[2*i+1]);
 if(!nflstreet_cluster_palette_rows(tr,ro,rw,bones,count,ps?angles:NULL,root,coefficients,matrices))return 0;
 memmove(o,t,0x40);for(unsigned i=1;i<count;++i)for(unsigned w=0;w<12;++w)put(o+0x30u*i+4*w,matrices[i][w]);
 return 1;}
static int model(context *c,uint32_t m,uint32_t *skel,uint32_t *clst){
 *skel=*clst=0;return !m||(word(c,m+0xc,clst)&&word(c,m+0x10,skel));}
/* 801EAC78: the body form (to the descriptor's scratch, then copied out). */
static int body(context *c,nflstreet_draw *d,uint32_t m,uint32_t desc,uint32_t pose,int slerp){
 if(slerp)return nflstreet_801ead98_slerp_body(c,d,m,desc,pose,0x8028dd70u);
 uint32_t skel,clst,scratch,count,out;if(!model(c,m,&skel,&clst))return 0;if(!skel||!clst)return 1;
 if(!word(c,desc+4,&scratch)||(scratch&&!cache(c))||!word(c,clst+4,&count)||!allocate(c,desc,count)||!word(c,desc,&out))return 0;
 if(!palette(c,d,scratch?scratch:out,skel,clst,pose))return 0;
 return !scratch||dma(c,out,scratch,count*0x30u);}
/* 801EACF4: the attachment form, built in the locked cache (the caller copies it out). */
static int attachment(context *c,nflstreet_draw *d,uint32_t m,uint32_t desc,uint32_t pose,uint32_t *count){
 uint32_t skel,clst,scratch;*count=0;if(!model(c,m,&skel,&clst))return 0;if(!skel||!clst)return 1;
 return word(c,desc+4,&scratch)&&(!scratch||cache(c))&&word(c,clst+4,count)&&allocate(c,desc,*count)&&palette(c,d,LOCKED,skel,clst,pose);}
/* Size the rotation/scale part (not the translation column) of each matrix
   whose cluster row tag matches, from row first (or matrix 0 alone for a
   one-cluster model when single is set). */
static int scale_rows(context *c,uint32_t m,uint32_t base,int tag,unsigned first,int single,float k){
 uint32_t clst,rows,count;if(!word(c,m+0xc,&clst)||!word(c,clst,&rows)||!word(c,clst+4,&count))return 0;
 for(unsigned i=single&&count==1?0:first;i<count;++i){
  if(!(single&&count==1)){const uint8_t *r=mem(c,rows+4*i,1);if(!r)return 0;if(*r!=tag)continue;}
  uint8_t *p=mem(c,base+0x30u*i,0x30);if(!p)return 0;for(unsigned w=0;w<12;++w)if(w%4!=3)put(p+4*w,fword(p+4*w)*k);
  if(single&&count==1)break;}
 return 1;}
static int copy_out(context *c,uint32_t actor,uint32_t m,uint32_t desc){
 uint32_t to,clst,count;return word(c,actor+desc,&to)&&word(c,m+0xc,&clst)&&word(c,clst+4,&count)&&dma(c,to,LOCKED,count*0x30u);}
/* Copy three angles from the body pose into a sub-model's pose. */
static int angles(context *c,uint32_t to,uint32_t from){
 uint8_t *t=mem(c,to,6);const uint8_t *f=mem(c,from,6);if(!t||!f)return 0;memmove(t,f,6);return 1;}
int nflstreet_actor_palettes(context *c,uint32_t actor,const float *scale,uint32_t world,uint32_t items,uint32_t lod){
 static nflstreet_render_state unused;
 if(!c||!c->x||!c->x->span||!actor)return 0;
 nflstreet_draw dd={c->x,&unused},*d=&dd;const nflstreet_matrix_stacks *k=&c->stacks;
 uint32_t flags,pose,skeleton,m,count;const uint8_t *a=mem(c,actor,0x1360);float s;
 if(!a)return 0;flags=rd32(a+0x14);pose=rd32(a+0x5c);skeleton=rd32(a+0x60);s=fword(a+0x1358);
 const int dynamic=((flags^8u)>>3)&1u;nflstreet_actor_dynamics_context dc={c->x,c->stacks}; /* 8004C268/8004BF1C */
 if(!nflstreet_matrix_push(d,k)||!nflstreet_matrix_concat(d,k,world,c->unit)||!nflstreet_matrix_scale(d,k,fword(a+0x18),fword(a+0x18),fword(a+0x18)))return 0;
 if(scale&&!nflstreet_matrix_scale(d,k,scale[0],scale[1],scale[2]))return 0;
 if(!nflstreet_matrix_push(d,k))return 0;
 if(!(flags&0x2000)&&(!nflstreet_matrix_translate(d,k,fword(a+0x34),fword(a+0x38),fword(a+0x3c))||!nflstreet_matrix_rotate(d,k,1,rd32(a+0x4c))))return 0;
 if(!body(c,d,rd32(a+0x4b8),actor+0x1008,pose,(flags&0x4000)!=0))return 0;
 if(lod<2){m=rd32(a+0x5a8);if(!attachment(c,d,m,actor+0x1014,pose,&count))return 0;
  if(count&&(!scale_rows(c,m,LOCKED,0xd,1,0,s)||!copy_out(c,actor,m,0x1014)))return 0;}
 static const struct {uint16_t model,desc;} parts[2]={{0x788,0x102c},{0x968,0x1038}};
 for(unsigned i=0;i<2;++i){m=rd32(a+parts[i].model);if(!m)continue;if(!attachment(c,d,m,actor+parts[i].desc,pose,&count))return 0;
  uint32_t base=LOCKED;if(i==0&&!word(c,actor+0x1030,&base))return 0; /* the shoulder pads scale through the descriptor's scratch word */
  if(count&&((lod<2&&!scale_rows(c,m,base,0xd,1,1,s))||!copy_out(c,actor,m,parts[i].desc)))return 0;}
 if(!nflstreet_matrix_pop(d,k))return 0;
 if((m=rd32(a+0x698))){uint32_t head=rd32(a+0x344);
  if(!nflstreet_matrix_push(d,k)||!nflstreet_matrix_load(d,k,actor+0x3ac)||!nflstreet_actor_head_dynamics(&dc,actor,actor+0x214,dynamic)||!nflstreet_matrix_pop(d,k)||
     !nflstreet_matrix_push(d,k)||!nflstreet_matrix_concat(d,k,skeleton+0x340,c->unit)||!angles(c,head,pose+0x4e)||
     !attachment(c,d,m,actor+0x1020,head,&count))return 0;
  if(count&&((lod<2&&!scale_rows(c,m,LOCKED,0,0,1,s))||!copy_out(c,actor,m,0x1020)))return 0;
  if(!nflstreet_matrix_pop(d,k))return 0;}
 for(unsigned i=0;i<2;++i){const uint32_t state=actor+0x11c+0x7cu*i;
  if(lod){if(!nflstreet_actor_hand_dynamics(&dc,actor,state,0))return 0;continue;}
  const uint32_t bone=i?0xe:0x14,to=rd32(a+0x18c+0x7cu*i);
  if(!nflstreet_matrix_push(d,k)||!nflstreet_matrix_load(d,k,actor+0x3ac)||!nflstreet_actor_hand_dynamics(&dc,actor,state,dynamic)||!nflstreet_matrix_pop(d,k)||
     !nflstreet_matrix_push(d,k)||!nflstreet_matrix_concat(d,k,skeleton+0x40u*bone,c->unit)||!angles(c,to,pose+6u*bone)||
     !body(c,d,rd32(a+0xd28+0xf0u*i),actor+0x105c+0xcu*i,to,0)||!nflstreet_matrix_pop(d,k))return 0;}
 if(lod<2)for(unsigned i=0;i<2;++i){if(a[0x78+0x48u*i]!=1)continue;
  const uint32_t bone=i?0x11:0x17,to=rd32(a+0xb4+0x48u*i);uint32_t item;
  if(!word(c,items+4*i,&item)||!nflstreet_matrix_push(d,k)||!nflstreet_matrix_concat(d,k,skeleton+0x40u*bone,c->unit)||
     !angles(c,to,pose+6u*bone+6u)||!body(c,d,item,actor+0x1044+0xcu*i,to,0)||!nflstreet_matrix_pop(d,k))return 0;}
 return nflstreet_matrix_pop(d,k);}
