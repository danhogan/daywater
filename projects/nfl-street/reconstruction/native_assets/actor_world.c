#include "actor_world.h"
#define rd32 nflstreet_rd32
static float fword(const uint8_t *p){return nflstreet_as_float(rd32(p));}
int nflstreet_actor_world(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t visual){
 const uint8_t *v=nflstreet_draw_at(d,visual,0x50);if(!v)return 0;
 if(!nflstreet_matrix_push_identity(d,k)||!nflstreet_matrix_translate(d,k,fword(v+4),fword(v+8),fword(v+0xc))||
    !nflstreet_matrix_rotate(d,k,2,rd32(v+0x24)+0x400000u)||!nflstreet_matrix_rotate(d,k,0,0x400000u))return 0;
 if((rd32(v+0x14)&0x2000)&&(!nflstreet_matrix_translate(d,k,fword(v+0x34),fword(v+0x38),fword(v+0x3c))||
    !nflstreet_matrix_rotate(d,k,1,rd32(v+0x4c))))return 0;
 return nflstreet_matrix_store(d,k,visual+0x3ac)&&nflstreet_matrix_pop(d,k);}
int nflstreet_actor_place(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t actor,uint32_t side){
 const uint8_t *a=nflstreet_draw_at(d,actor,0x310);uint32_t record;if(!a||!nflstreet_draw_word(d,side,&record))return 0;
 const uint8_t *flip=record?nflstreet_draw_at(d,record+0x39,1):NULL;if(record&&!flip)return 0;
 const uint32_t visual=rd32(a+4);uint8_t *v=nflstreet_draw_at(d,visual,0x3f0);if(!v)return 0;
 const int mirrored=flip&&*flip==1;
 nflstreet_wr32(v+4,rd32(a+0x1a8)^(mirrored?0x80000000u:0));nflstreet_wr32(v+8,rd32(a+0x1ac)^(mirrored?0x80000000u:0));
 nflstreet_wr32(v+0xc,rd32(a+0x1b0));nflstreet_wr32(v+0x24,mirrored?(rd32(a+0x1c0)-0x800000u)&0xffffffu:rd32(a+0x1c0));
 nflstreet_wr32(v+0x3ec,rd32(a+0x30c));return nflstreet_actor_world(d,k,visual);}
