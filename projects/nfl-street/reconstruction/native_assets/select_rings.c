#include "select_rings.h"
#include "marker_ram.h"
#define A_TABLE 0x8027be8cu /* nine ring objects, one per tracked slot */
#define B_TABLE 0x8027beb0u /* nine companion objects that mirror them */
#define POOL 0x802ac020u    /* five animated rings */
#define K_ZERO 0x802536f4u  /* 0.0: the ring's +0C */
#define K_CAP 0x802536f8u   /* 2.5: the pooled ring's size cap (800E44D8) */
#define K_ONE 0x802536fcu   /* 1.0 */
#define K_SHRINK 0x80253700u
#define K_GROW 0x80253704u
#define K_LIMIT 0x80253708u
#define K_FOLLOW 0x8025370cu
#define K_FADE 0x80253710u
#define K_RING_Z 0x80253718u /* the controlled ring's +0C */
#define K_START 0x80253758u  /* a pooled ring's starting size */
typedef struct {nflstreet_draw d;uint32_t r13;} R;
#define RD(r,a,v) mk_word(&(r)->d,a,v)
/* 8003D65C: pad n's team/slot byte (r13-7C10 +6), 0xFF when none or n > 1. */
static int pad(R *r,uint32_t n,uint8_t *v){
 uint32_t base;*v=0xff;if(n>1)return 1;return RD(r,r->r13-0x7c10u,&base)&&mk_byte(&r->d,base+6+n,v);}
/* 8003A4A0: the gameplay actor of (team, index) in the r13-7C30 table. */
static int slot_actor(R *r,uint32_t team,uint32_t index,uint32_t *actor){
 uint32_t table,base;const uint8_t *h;*actor=0;if(!RD(r,r->r13-0x7c30u,&table))return 0;if(!table)return 1;
 if(!(h=nflstreet_draw_at(&r->d,table+8,2))||!RD(r,table,&base))return 0;
 *actor=base+(team*nflstreet_rd16(h)+index)*0x14d4u;return 1;}
/* 80020944: the actor a packed reference word (kind +0, team +1, index +2) names. */
static int ref_actor(R *r,uint32_t ref,uint32_t *actor){
 uint32_t w;*actor=0;if(!RD(r,ref,&w))return 0;if((w&0xff)!=1)return 1;return slot_actor(r,w>>8&0xff,w>>16&0xff,actor);}
/* 8003D600: the actor of a pad-table entry (two bytes, 0xFF none). */
static int table_actor(R *r,uint32_t entry,uint32_t *actor){
 uint32_t t;uint8_t team,index;*actor=0;
 if(!RD(r,r->r13-0x7c10u,&t)||!RD(r,t,&t)||!mk_byte(&r->d,t+4*entry,&team)||!mk_byte(&r->d,t+4*entry+1,&index))return 0;
 return team==0xff||index==0xff?1:slot_actor(r,team,index,actor);}
/* 800BE3F0 (800BDDC4 + 800BE380): the actor of the current controller entry. */
static int controlled(R *r,uint32_t *actor){
 uint32_t base;uint8_t n;*actor=0;if(!RD(r,r->r13-0x73d0u,&base)||!mk_byte(&r->d,base+1,&n))return 0;
 const uint32_t entry=base+n*0xf4u+8u;return entry?ref_actor(r,entry+0xb4,actor):1;}
/* 80103E64: the side record (r13-7078) says the field is mirrored. */
static int mirrored(R *r,int *flag){uint32_t record;uint8_t v;*flag=0;if(!RD(r,r->r13-0x7078u,&record))return 0;
 if(!record)return 1;if(!mk_byte(&r->d,record+0x39,&v))return 0;*flag=v==1;return 1;}
/* 800E41A8: the ring takes the (mirrored) x and y at src, z 0, turns by
   0x16C16 and is shown. */
static int follow(R *r,uint32_t ring,uint32_t src,int negate){
 uint32_t x,y,angle,zero;const uint32_t sign=negate?0x80000000u:0;
 if(!RD(r,src,&x)||!RD(r,ring+0x14,&angle)||!RD(r,K_ZERO,&zero)||!RD(r,src+4,&y))return 0;
 return mk_put(&r->d,ring+4,x^sign)&&mk_put(&r->d,ring+0xc,zero)&&mk_put(&r->d,ring+0x14,(angle+0x16c16u)&0xffffffu)&&
  mk_put(&r->d,ring+8,y^sign)&&mk_mask(&r->d,ring+0x20,0,2);}
/* 800E3C48: the gameplay actor tracked in slot i (0 when none) and whether
   one was found; slot 8 follows the controller's actor and records it at
   r13-71D8. A reference that names no actor where the original would read
   it fails here. */
static int track(R *r,unsigned i,uint32_t ring,uint32_t controller,uint32_t *actor,uint8_t *found){
 uint32_t a=0,flags,state,bits,index,t;uint8_t p0,p1,team;*found=0;*actor=0;
 if(!ring){return 1;}
 if(!pad(r,0,&p0)||!pad(r,1,&p1))return 0;
 if(p0==0xff&&p1==0xff)return 1;
 if(!RD(r,ring+0x20,&flags))return 0;
 if(i==8){
  if(!(flags&4)){
   if(controller){uint32_t ref,cflags;
    if(!RD(r,controller+4,&t)||!RD(r,t+0x14,&bits)||!RD(r,controller,&ref)||!RD(r,controller+0xc,&cflags))return 0;
    if((bits&1)&&(ref&0xff)==1&&!(cflags&0x400)){if(!pad(r,ref>>8&0xff,&team))return 0;if(team==0xff)a=controller;}}}
  else{if(!ref_actor(r,ring+0x100,&t)||!t||!RD(r,t+4,&state)||!RD(r,state+0x14,&bits))return 0;if(bits&1)a=t;}
  if(!mk_put(&r->d,r->r13-0x71d8u,a))return 0;}
 else{
  if(!RD(r,ring+0x2c,&index))return 0;
  if(index==0xffffffffu)return 1;
  int by_table=1;
  if(flags&4){
   if(!ref_actor(r,ring+0x100,&t)||!t||!RD(r,t+0xc,&bits))return 0;
   if(!(bits&0x400)){if(!RD(r,t+4,&state)||!RD(r,state+0x14,&bits))return 0;if(bits&1)a=t;by_table=0;}
   else if(!RD(r,ring+0x2c,&index))return 0;}
  if(by_table){
   if(!table_actor(r,index,&a))return 0;
   if(a){if(!RD(r,a+4,&state)||!RD(r,state+0x14,&bits)||!RD(r,a+0xc,&flags))return 0;
    if((bits&1)&&!(flags&0x400))a=0;}}}
 *actor=a;*found=a!=0;return 1;}
/* 800E4498: the first pooled ring with no target. */
static int pool_free(R *r,uint32_t *ring){
 *ring=0;for(unsigned i=0;i<5;++i){uint32_t o,target;if(!RD(r,POOL+4*i,&o)||!RD(r,o+0xfc,&target))return 0;if(!target){*ring=o;return 1;}}
 return 1;}
/* 800E4F34: start a pooled ring on the target of ring. */
static int pool_start(R *r,uint32_t ring){
 uint32_t free_ring,target,w;int negate;if(!ring)return 1;if(!pool_free(r,&free_ring))return 0;if(!free_ring)return 1;
 if(!mk_mask(&r->d,free_ring+0x20,0,0x22)||!RD(r,ring+0xfc,&target)||!mk_put(&r->d,free_ring+0xfc,target)||!mk_put(&r->d,free_ring+0x14,0)||
    !mk_copy(&r->d,free_ring+0x18,K_START)||!mk_copy(&r->d,free_ring+0x38,K_CAP)||!RD(r,ring+0x30,&w)||!mk_put(&r->d,free_ring+0x30,w)||
    !mirrored(r,&negate))return 0;
 return follow(r,free_ring,target+0x1a8,negate);}
/* 800E44E4: the five pooled rings' size animation by flags 4, 8, 0x20, 0x40
   and the turn by 0x10. */
static int animate(R *r){
 float shrink,grow,limit,following,fade,v,one,cap;
 if(!mk_float(&r->d,K_GROW,&grow)||!mk_float(&r->d,K_SHRINK,&shrink)||!mk_float(&r->d,K_LIMIT,&limit)||!mk_float(&r->d,K_FOLLOW,&following)||
    !mk_float(&r->d,K_FADE,&fade))return 0;
 for(unsigned i=0;i<5;++i){
  uint32_t o,flags,target;int negate;if(!RD(r,POOL+4*i,&o)||!RD(r,o+0x20,&flags))return 0;if(!(flags&2))continue;
  if(!mk_float(&r->d,K_ONE,&one))return 0;
  if(flags&4){if(!mk_float(&r->d,o+0x18,&v))return 0;
   if(v<=one){if(!mk_putf(&r->d,o+0x18,one)||!mk_put(&r->d,o+0xfc,0))return 0;}else if(!mk_putf(&r->d,o+0x18,v*shrink))return 0;}
  if(!RD(r,o+0x20,&flags))return 0;
  if(flags&8){if(!mk_float(&r->d,o+0x18,&v))return 0;v=v*grow;if(!mk_putf(&r->d,o+0x18,v))return 0;
   if(!(v<=limit)&&(!mk_put(&r->d,o+0xfc,0)||!mk_put(&r->d,o+0x20,flags&~2u)))return 0;}
  if(!RD(r,o+0x20,&flags))return 0;
  if(flags&0x20){if(!mk_float(&r->d,o+0x18,&v)||!mk_putf(&r->d,o+0x18,v*following)||!RD(r,o+0xfc,&target)||!mirrored(r,&negate)||
    !follow(r,o,target+0x1a8,negate)||!mk_float(&r->d,o+0x38,&cap)||!mk_float(&r->d,o+0x18,&v))return 0;
   if(!(v<=cap)){if(!RD(r,o+0x20,&flags)||!mk_putf(&r->d,o+0x18,cap)||!mk_put(&r->d,o+0x20,(flags&~0x20u)|0x40u))return 0;}}
  if(!RD(r,o+0x20,&flags))return 0;
  if(flags&0x40){if(!RD(r,o+0xfc,&target)||!mirrored(r,&negate)||!follow(r,o,target+0x1a8,negate)||!mk_float(&r->d,o+0x18,&v))return 0;
   if(v<=one){if(!RD(r,o+0x20,&flags)||!mk_putf(&r->d,o+0x18,one)||!mk_put(&r->d,o+0xfc,0)||!mk_put(&r->d,o+0x20,flags&~0x42u))return 0;}
   else if(!mk_putf(&r->d,o+0x18,v*fade))return 0;}
  if(!RD(r,o+0x20,&flags))return 0;
  if(flags&0x10){uint32_t angle;if(!RD(r,o+0x14,&angle)||!mk_put(&r->d,o+0x14,(angle+0x16c16u)&0xffffffu))return 0;}}
 return 1;}
int nflstreet_rings_update(const nflstreet_rings_context *c){
 static nflstreet_render_state unused;
 if(!c||!c->x||!c->x->span)return 0;
 R r={{c->x,&unused},c->r13};uint32_t controller,ring2,z;
 if(!controlled(&r,&controller)||!RD(&r,c->r13-0x71e0u,&ring2)||!RD(&r,K_RING_Z,&z)||!mk_mask(&r.d,ring2+0x20,2,0))return 0;
 for(unsigned i=0;i<9;++i){
  uint32_t o,b,actor,w,flags;uint8_t found;int negate;
  if(!RD(&r,A_TABLE+4*i,&o)||!RD(&r,B_TABLE+4*i,&b))return 0;
  if(!o)continue;
  if(!track(&r,i,o,controller,&actor,&found))return 0;
  {uint8_t p0,p1;if(!pad(&r,0,&p0)||!pad(&r,1,&p1))return 0;if(!(p0==0xff&&p1==0xff)&&!mk_put(&r.d,o+0xfc,actor))return 0;}
  if(found){
   if(!RD(&r,actor+0xc,&flags))return 0;
   if(flags&0x4000000u){if(!pool_start(&r,o)||!mk_mask(&r.d,actor+0xc,0x4000000u,0))return 0;}
   if(!mirrored(&r,&negate)||!follow(&r,o,actor+0x1a8,negate)||!mk_mask(&r.d,b+0x20,0,2)||!mk_copy(&r.d,b+4,o+4)||!mk_copy(&r.d,b+8,o+8)||
      !mk_copy(&r.d,b+0xc,o+0xc))return 0;
   if(actor==controller){
    uint8_t mark;uint32_t ring,angle;
    if(!mk_byte(&r.d,o+0x2f,&mark)||!mk_putb(&r.d,c->r13-0x71dau,mark)||!RD(&r,c->r13-0x71e0u,&ring)||!RD(&r,ring+0x20,&flags)||
       !RD(&r,o+0x30,&w)||!mk_put(&r.d,ring+0x30,w)||!mk_put(&r.d,ring+0x20,flags|2)||!mk_copy(&r.d,ring+4,o+4)||!mk_put(&r.d,ring+0xc,z)||
       !mk_copy(&r.d,ring+8,o+8)||!RD(&r,ring+0x14,&angle))return 0;
    if(!mk_put(&r.d,ring+0x14,flags&0x10?(angle+0x16c16u)&0xffffffu:0))return 0;}}
  else if(!mk_mask(&r.d,o+0x20,2,0)||!mk_mask(&r.d,b+0x20,2,0))return 0;}
 return animate(&r);}
