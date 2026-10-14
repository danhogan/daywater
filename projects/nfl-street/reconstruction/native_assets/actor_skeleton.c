#include "actor_skeleton.h"
#include "compact_pose.h"
#include <string.h>
#define rd32 nflstreet_rd32
#define LOCKED 0xe0000000u
typedef nflstreet_actor_palette_context context;
static uint8_t *mem(context *c,uint32_t a,uint32_t n){
 if(a>=LOCKED)return (uint64_t)a-LOCKED+n<=sizeof c->locked?c->locked+(a-LOCKED):NULL;
 return a?c->x->span(c->x->memory,a,n):NULL;}
/* The sine table 801EAE1C passes to 80188BB0 (compact_pose.c has it built in). */
static int original_sine(context *c){
 static const uint32_t words[6]={0x40490fdbu,0xc0a55de8u,0x402335e5u,0xbf196969u,0x3da83c1du,0xbbf183adu};
 const uint8_t *p=mem(c,c->stacks.sine,24);if(!p)return 0;for(unsigned i=0;i<6;++i)if(rd32(p+4*i)!=words[i])return 0;return 1;}
/* 801EAE1C(desc, skeleton, 0xE0000000): the pose at desc+30, the matrices
   to desc+34 through the locked cache. */
static int evaluate(context *c,nflstreet_draw *d,uint32_t desc,uint32_t skeleton){
 const uint8_t *h=mem(c,skeleton,16),*q=mem(c,desc+0x30,8);uint32_t top;
 if(!h||!q||memcmp(h,"SKL1",4)||!original_sine(c))return 0;
 const unsigned bones=(unsigned)h[6]<<8|h[7];if(!bones||bones>256)return 0;
 const uint8_t *s=mem(c,skeleton,16+bones*16u),*p=mem(c,rd32(q),bones*6u),*m;uint8_t *on=mem(c,c->cache_on,1),*out=mem(c,LOCKED,bones*0x40u),*to=mem(c,rd32(q+4),bones*0x40u);
 if(!s||!p||!on||!out||!to||!nflstreet_matrix_top(d,&c->stacks,&top)||!(m=mem(c,top,0x30)))return 0;
 float root[12],matrices[256][12];int16_t angles[256*3];unsigned count;
 for(unsigned i=0;i<12;++i)root[i]=nflstreet_as_float(rd32(m+4*i));
 for(unsigned i=0;i<bones*3u;++i)angles[i]=(int16_t)(uint16_t)(p[2*i]<<8|p[2*i+1]);
 if(!nflstreet_compact_pose(s,16+bones*16u,angles,bones*3u,root,matrices,&count)||count!=bones)return 0;
 if(!*on)*on=1; /* 801EDE38 (801F0E80/801F0E8C wait on the DMA queue: no memory effect) */
 for(unsigned i=0;i<bones;++i)for(unsigned w=0;w<12;++w)nflstreet_wr32(out+0x40u*i+4*w,nflstreet_float_bits(matrices[i][w]));
 memmove(to,out,bones*0x40u); /* 801F0BA0 flush, then the DMA 801F0DD4 of whole 0x40 slots */
 return 1;}
int nflstreet_actor_skeleton(context *c,uint32_t visual){
 static nflstreet_render_state unused;if(!c||!c->x||!c->x->span||!visual)return 0;
 nflstreet_draw dd={c->x,&unused},*d=&dd;const nflstreet_matrix_stacks *k=&c->stacks;const uint8_t *a=mem(c,visual,0x68);if(!a)return 0;
 /* Validate before the first write (the push), so a rejection leaves memory as it was. */
 const uint8_t *h=mem(c,rd32(a+0x64),8);if(!h||memcmp(h,"SKL1",4)||!original_sine(c))return 0;
 if(!nflstreet_matrix_push_identity(d,k))return 0;
 if(!(rd32(a+0x14)&0x2000u)&&(!nflstreet_matrix_translate(d,k,nflstreet_as_float(rd32(a+0x34)),nflstreet_as_float(rd32(a+0x38)),nflstreet_as_float(rd32(a+0x3c)))||
    !nflstreet_matrix_rotate(d,k,1,rd32(a+0x4c))))return 0;
 return evaluate(c,d,visual+0x2c,rd32(a+0x64))&&nflstreet_matrix_pop(d,k);}
int nflstreet_actor_skeleton_frame(context *c,uint32_t visual){
 static nflstreet_render_state unused;if(!c||!c->x||!c->x->span)return 0;nflstreet_draw dd={c->x,&unused};
 uint8_t *current=mem(c,c->stacks.current,4);if(!current)return 0;
 nflstreet_wr32(current,1u<c->stacks.count?c->stacks.table+12u:0u); /* 801893C8(1) via 80189068 */
 return nflstreet_matrix_reset(&dd,&c->stacks)&&nflstreet_actor_skeleton(c,visual);}
