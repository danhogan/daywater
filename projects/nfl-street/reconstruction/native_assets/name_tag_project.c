#include "name_tag_project.h"
#include "marker_ram.h"
#define K_UNIT 0x80252b6cu   /* 1.0 */
#define K_ASPECT 0x80252b70u /* 0.75 */
#define K_WIDTH 0x80252b74u  /* 320.0 */
#define K_HEIGHT 0x80252b78u /* 224.0 */
#define K_LIFT 0x80252b7cu   /* 25.0 */
#define rd32 nflstreet_rd32
/* 801DED68: v transformed by the stack top as 4x4 rows (row r: m[4r+1]*y,
   then fused m[4r]*x, m[4r+2]*z, m[4r+3]*w). */
static int transform(nflstreet_draw *d,const nflstreet_matrix_stacks *k,const float v[4],float out[4]){
 uint32_t top;const uint8_t *t;if(!nflstreet_matrix_top(d,k,&top)||!(t=nflstreet_draw_at(d,top,0x40)))return 0;
 for(unsigned r=0;r<4;++r){const uint8_t *q=t+16*r;float m[4];for(unsigned i=0;i<4;++i)m[i]=nflstreet_as_float(rd32(q+4*i));
  float acc=m[1]*v[1];acc=nflstreet_fmadds(m[0],v[0],acc);acc=nflstreet_fmadds(m[2],v[2],acc);out[r]=nflstreet_fmadds(m[3],v[3],acc);}
 return 1;}
int nflstreet_name_tag_project(const nflstreet_view_context *c,uint32_t tag,uint32_t position){
 static nflstreet_render_state unused;
 if(!c||!c->x||!c->x->span||!tag)return 0;
 nflstreet_draw dd={c->x,&unused},*d=&dd;const nflstreet_matrix_stacks *k=&c->stacks;
 float p[3],one,o[4],q[4],z,lift,width,height,aspect,depth_scale,node_aspect;uint32_t node;
 for(unsigned i=0;i<3;++i)if(!mk_float(d,position+4*i,&p[i]))return 0;
 if(!nflstreet_matrix_select(d,k)||!nflstreet_matrix_push(d,k)||!mk_float(d,K_UNIT,&one)||!nflstreet_matrix_load(d,k,c->slots+0x100))return 0;
 const float v[4]={p[0],p[1],p[2],one};
 if(!transform(d,k,v,o))return 0;
 const float w=o[3],nw=-w;
 if(o[0]<=nw||o[0]>=w||o[1]<=nw||o[1]>=w||o[2]<=nw||o[2]>=w)return mk_putb(d,tag+0x1c,0)&&nflstreet_matrix_pop(d,k);
 const float inv=one/w;
 q[0]=o[0]*inv;q[1]=o[1]*inv;q[2]=o[2]*inv;q[3]=w*inv;
 if(!nflstreet_matrix_load(d,k,c->slots+0x40)||!transform(d,k,q,o))return 0;
 z=o[2];
 if(!mk_word(d,c->r13-0x4eecu,&node)||!mk_float(d,node+0xd0,&node_aspect)||!mk_float(d,K_HEIGHT,&height)||!mk_float(d,K_ASPECT,&aspect)||
    !mk_float(d,K_WIDTH,&width)||!mk_float(d,c->r13-0x7244u,&depth_scale)||!mk_float(d,K_LIFT,&lift))return 0;
 const float x=nflstreet_fmadds(node_aspect*aspect*width,q[0],width);
 const float y=(height-q[1]*height)+lift;
 return mk_putb(d,tag+0x1c,1)&&mk_putf(d,tag+4,x)&&mk_putf(d,tag+8,y)&&mk_putf(d,tag+0x20,z*depth_scale)&&nflstreet_matrix_pop(d,k);}
int nflstreet_actor_name_tag(const nflstreet_view_context *c,uint32_t actor){
 static nflstreet_render_state unused;
 if(!c||!c->x||!c->x->span)return 0;
 nflstreet_draw dd={c->x,&unused};uint32_t tag;uint8_t shown;
 if(!mk_word(&dd,actor+0x3fc,&tag))return 0;if(!tag)return 1;
 if(!mk_byte(&dd,tag+0x1c,&shown))return 0;
 return shown?nflstreet_name_tag_project(c,tag,actor+4):1;}
