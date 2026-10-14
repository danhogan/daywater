#include "camera_slots.h"
#include "marker_ram.h"
#define rd32 nflstreet_rd32
#define UNIT 0x803a3958u /* the two concat lanes (803A3958, 803A395C) */
/* 801895F4 -> 80189514: top = top x slot, as 4x4 matrices (row r, column i:
   slot[4+i]*top[r][1], then fused slot[i]*top[r][0], slot[8+i]*top[r][2],
   slot[12+i]*top[r][3]). */
static int concat4(nflstreet_draw *d,const nflstreet_matrix_stacks *k,uint32_t slot){
 uint32_t top;uint8_t *t;const uint8_t *m;
 if(!nflstreet_matrix_top(d,k,&top)||!(t=nflstreet_draw_at(d,top,0x40))||!(m=nflstreet_draw_at(d,slot,0x40)))return 0;
 float a[16],b[16],r[16];for(unsigned i=0;i<16;++i){a[i]=nflstreet_as_float(rd32(t+4*i));b[i]=nflstreet_as_float(rd32(m+4*i));}
 for(unsigned row=0;row<4;++row)for(unsigned i=0;i<4;++i){const float *q=a+4*row;
  float acc=b[4+i]*q[1];acc=nflstreet_fmadds(b[i],q[0],acc);acc=nflstreet_fmadds(b[8+i],q[2],acc);acc=nflstreet_fmadds(b[12+i],q[3],acc);r[4*row+i]=acc;}
 for(unsigned i=0;i<16;++i)nflstreet_wr32(t+4*i,nflstreet_float_bits(r[i]));return 1;}
int nflstreet_camera_slots(const nflstreet_view_context *c){
 static nflstreet_render_state unused;
 if(!c||!c->x||!c->x->span)return 0;
 nflstreet_draw dd={c->x,&unused},*d=&dd;const nflstreet_matrix_stacks *k=&c->stacks;
 const uint32_t s1=c->slots+0x40,s2=c->slots+0x80,s3=c->slots+0xc0,s4=c->slots+0x100,s5=c->slots+0x140,s6=c->slots+0x180;
 uint32_t view;float unit[2];
 if(!mk_float(d,UNIT,&unit[0])||!mk_float(d,UNIT+4,&unit[1])||!nflstreet_matrix_select(d,k)||!nflstreet_matrix_top(d,k,&view))return 0;
 /* slot 3: identity pushed, times the view (the original copies the view to a local first; it is the same data) */
 return nflstreet_matrix_push_identity(d,k)&&nflstreet_matrix_concat(d,k,view,unit)&&nflstreet_matrix_store(d,k,s3)&&nflstreet_matrix_pop(d,k)&&
  nflstreet_matrix_load(d,k,s3)&&nflstreet_matrix_push(d,k)&&
  nflstreet_matrix_load(d,k,s1)&&concat4(d,k,s2)&&nflstreet_matrix_store(d,k,s6)&&
  nflstreet_matrix_load(d,k,s1)&&concat4(d,k,s2)&&concat4(d,k,s3)&&nflstreet_matrix_store(d,k,s5)&&
  nflstreet_matrix_load(d,k,s2)&&concat4(d,k,s3)&&nflstreet_matrix_store(d,k,s4)&&nflstreet_matrix_pop(d,k);}
