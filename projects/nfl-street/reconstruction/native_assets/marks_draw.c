#include "marks_draw.h"
#include "scenery_shared.h"
#define rd32 nflstreet_rd32
#define GX NFLSTREET_GX
#define K (&c->stacks)
static int word(nflstreet_draw *d,uint32_t token,uint32_t *v){return nflstreet_draw_word(d,token,v);}
/* Handlers per type: begin, end, draw. */
static int handlers(nflstreet_draw *d,const nflstreet_marks_context *c,uint32_t type,uint32_t h[3]){
 for(unsigned i=0;i<3;++i)if(!word(d,c->handlers+0x20u*type+4u*i,h+i))return 0;return 1;}
/* 8015FD28: blend preset 2, early depth, alpha always, depth LEQUAL with
   update, one channel. */
static int begin_flat(nflstreet_draw *d){
 return nflstreet_draw_blend(d,2)&&GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,1)&&GX(d,NFLSTREET_GX_SET_ALPHA_COMPARE,7,0,0,4,0)&&
  GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,1)&&GX(d,NFLSTREET_GX_SET_NUM_CHANS,1);}
/* 80160104 */
static int begin_marks(nflstreet_draw *d,const nflstreet_marks_context *c){
 uint32_t fog;uint8_t *saved=nflstreet_draw_at(d,c->fog_saved,1);if(!saved)return 0;
 nflstreet_draw_reset(d);if(!nflstreet_scenery_fog_switch(d,c->scene,1,&fog))return 0;*saved=(uint8_t)fog;
 const uint32_t white=0xffffffffu;
 return GX(d,NFLSTREET_GX_SET_CULL_MODE,0)&&GX(d,NFLSTREET_GX_SET_BLEND_MODE,1,4,5,5)&&GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,0)&&
  GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,0)&&nflstreet_draw_depth_range(d,c->depth[0],c->depth[0])&&
  nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)&&GX(d,NFLSTREET_GX_SET_VTX_DESC,9,1)&&GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)&&
  nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},&white,1)&&GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)&&
  GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,0,0,1,2,1)&&GX(d,NFLSTREET_GX_SET_VTX_DESC,0xd,1)&&GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0)&&
  GX(d,NFLSTREET_GX_SET_TEV_OP,0,0)&&GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)&&GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x3c,0,0x7d)&&
  GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)&&GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4);}
/* 80160250 */
static int end_marks(nflstreet_draw *d,const nflstreet_marks_context *c){
 uint32_t unused;const uint8_t *saved=nflstreet_draw_at(d,c->fog_saved,1);if(!saved)return 0;
 return nflstreet_draw_depth_range(d,c->depth[1],c->depth[2])&&GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,1)&&
  nflstreet_scenery_fog_switch(d,c->scene,*saved,&unused);}
/* 80160298 */
static int draw_mark(nflstreet_draw *d,const nflstreet_marks_context *c,uint32_t mark){
 const uint8_t *m=nflstreet_draw_at(d,mark,0x74);uint32_t record,top,mtx[12];if(!m)return 0;
 const uint32_t color=0xffffff00u|((uint32_t)nflstreet_fctiwz(nflstreet_as_float(rd32(m+0x34))*c->alpha_scale)&0xffu);
 record=rd32(m+0x60);const uint8_t *r=nflstreet_draw_at(d,record+0x28,8);if(!r)return 0;
 if(!nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},&color,1)||!nflstreet_draw_texture(d,0,rd32(r),rd32(r+4))||
    !nflstreet_matrix_push(d,K)||
    !nflstreet_matrix_translate(d,K,nflstreet_as_float(rd32(m)),nflstreet_as_float(rd32(m+4)),nflstreet_as_float(rd32(m+8)))||
    !nflstreet_matrix_rotate(d,K,2,rd32(m+0x30))||
    !nflstreet_matrix_scale(d,K,nflstreet_as_float(rd32(m+0xc)),nflstreet_as_float(rd32(m+0x10)),nflstreet_as_float(rd32(m+0x14)))||
    !nflstreet_matrix_top(d,K,&top))return 0;
 const uint8_t *t=nflstreet_draw_at(d,top,0x30);if(!t)return 0;for(unsigned i=0;i<12;++i)mtx[i]=rd32(t+4*i);
 if(!nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},mtx,12)||!GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0)||
    !nflstreet_matrix_pop(d,K)||!GX(d,NFLSTREET_GX_BEGIN,0xa0,0,4))return 0;
 /* Corners (x, y, 0, s, t): quad indices into 80260730.. (-0.16, 0.27, 0, 0.16, 1, -0.27). */
 static const uint8_t v[20]={0,1,2,2,2, 3,1,2,4,2, 3,5,2,4,4, 0,5,2,2,4};
 for(unsigned i=0;i<20;++i)if(!GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(c->quad[v[i]])))return 0;
 return 1;}
static int list(nflstreet_draw *d,const nflstreet_marks_context *c,uint32_t index,uint32_t *base,uint32_t *count){
 return word(d,c->lists+4*index,base)&&word(d,c->counts+4*index,count);}
int nflstreet_marks_draw(const nflstreet_marks_context *c,nflstreet_render_state *s){
 if(!c||!c->scene||!c->scene->draw||!c->scene->draw->span||!c->scene->draw->emit||!s)return 0;
 nflstreet_draw dd={c->scene->draw,s},*d=&dd;const uint8_t *on=nflstreet_draw_at(d,c->enabled,1);if(!on)return 0;if(!*on)return 1;
 uint32_t h[3][3];for(unsigned t=0;t<3;++t)if(!handlers(d,c,t,h[t]))return 0;
 if(h[2][0]!=c->type2[0]||h[2][1]!=c->type2[2]||h[2][2]!=c->type2[1])return 0;
 for(unsigned t=0;t<2;++t)if(h[t][0]!=c->type_begin[0]||h[t][1]!=c->type_begin[1])return 0;
 if(!nflstreet_matrix_select(d,K)||!begin_marks(d,c))return 0;
 uint32_t base,count,i;float alpha=c->ramp_step;if(!list(d,c,2,&base,&count)||!word(d,c->newest,&i))return 0;
 for(uint32_t n=0;(int32_t)n<(int32_t)count;){uint8_t *m=nflstreet_draw_at(d,base+0x74u*i,0x74);uint32_t type;if(!m)return 0;
  nflstreet_wr32(m+0x34,nflstreet_float_bits(alpha));
  if(m[0x70]){type=rd32(m+0x68);if(type!=2||!draw_mark(d,c,base+0x74u*i))return 0;}
  if((int32_t)++i>=(int32_t)count)i=0;
  if(!(++n&7)){alpha+=c->ramp_step;if(!(alpha<=c->ramp_cap))alpha=c->ramp_cap;}}
 if(!end_marks(d,c))return 0;
 for(int t=1;t>=0;--t){if(!begin_flat(d)||!list(d,c,(uint32_t)t,&base,&count))return 0;
  for(uint32_t k=0;k<count;++k){const uint8_t *m=nflstreet_draw_at(d,base+0x74u*k,0x74);if(!m)return 0;if(m[0x70])return 0;}}
 return 1;}
