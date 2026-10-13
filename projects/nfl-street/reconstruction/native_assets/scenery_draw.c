#include "scenery_draw.h"
#include "draw_engine.h"
#include <math.h>
#define rd32 nflstreet_rd32
#define GX NFLSTREET_GX
static float fword(const uint8_t *p){return nflstreet_as_float(rd32(p));}
static uint8_t *at(nflstreet_draw *d,uint32_t token,uint32_t size){return nflstreet_draw_at(d,token,size);}
/* 8018937C: copy sixteen words. */
static int copy16(nflstreet_draw *d,uint32_t to,uint32_t from){
 uint8_t *t=at(d,to,0x40),*f=at(d,from,0x40);if(!t||!f)return 0;memmove(t,f,0x40);return 1;}
/* 801CAE78: the eight bounding-box corners through a 3x4 matrix, each
   x*m0 + ((y*m1 + z*m2) + m3) in single precision. */
static void corners(const float b[8],const float m[12],float out[24]){
 for(unsigned r=0;r<3;++r){const float *q=m+4*r;
  float a=q[1]*b[1],c=q[2]*b[2],t=q[3],xa=q[0]*b[0],xb=q[0]*b[4],yb=q[1]*b[5],zb=q[2]*b[6];
  float s0=(a+c)+t,s1=(yb+c)+t,s2=(a+zb)+t,s3=(yb+zb)+t;
  out[r]=xa+s0;out[3+r]=xb+s0;out[6+r]=xa+s1;out[9+r]=xb+s1;out[12+r]=xa+s2;out[15+r]=xb+s2;out[18+r]=xa+s3;out[21+r]=xb+s3;}}
/* 801CB05C: clip codes of points against a 4x4 projection (only P0, P2,
   P5, P6, P10, P11 and P14 are used); returns (or << 8) | and. */
static uint32_t clip_codes(const float *p,unsigned n,const float P[16]){
 uint32_t all=0x3f,any=0;
 for(unsigned i=0;i<n;++i,p+=3){float z=p[2],w=P[14]*z;
  float x=nflstreet_fmadds(P[0],p[0],P[2]*z),y=nflstreet_fmadds(P[5],p[1],P[6]*z),c=nflstreet_fmadds(P[10],z,P[11]);uint32_t code=0;
  if(!(x>=-w))code=2;else if(!(x<=w))code=1;
  if(!(y>=-w))code|=8;else if(!(y<=w))code|=4;
  if(!(c>=-w))code|=0x20;else if(!(c<=w))code|=0x10;
  any|=code;all&=code;}
 return any<<8|all;}
/* 801CB3EC(0), 801CB430(matrix), 801CB708(box, 0, 0). */
static int culled(nflstreet_draw *d,const nflstreet_scenery_context *c,uint32_t instance,int *out){
 if(!copy16(d,c->cull_projection,c->projection)||!copy16(d,c->cull_modelview,instance+8))return 0;
 const uint8_t *P=at(d,c->cull_projection,0x40),*M=at(d,c->cull_modelview,0x30),*B=at(d,instance+0x98,0x20);if(!P||!M||!B)return 0;
 float proj[16],m[12],box[8],pts[24];
 for(unsigned i=0;i<16;++i)proj[i]=fword(P+4*i);for(unsigned i=0;i<12;++i)m[i]=fword(M+4*i);for(unsigned i=0;i<8;++i)box[i]=fword(B+4*i);
 corners(box,m,pts);*out=(clip_codes(pts,8,proj)&0x3f)!=0;return 1;}
/* 801EDD74: fog from the camera or off. */
static int fog_apply(nflstreet_draw *d,const nflstreet_scenery_context *c,uint8_t enabled){
 uint32_t v[5]={c->fog_color,0,0,0,0};
 if(!enabled||!c->fog_camera){for(unsigned i=1;i<5;++i)v[i]=nflstreet_float_bits(c->fog_off);
  return nflstreet_draw_gx(d,NFLSTREET_GX_SET_FOG,1,(uint32_t[]){0},v,5);}
 const uint8_t *cam=at(d,c->fog_camera+0xe8,8);if(!cam)return 0;
 v[1]=nflstreet_float_bits(c->fog_start);v[2]=nflstreet_float_bits(c->fog_end);v[3]=rd32(cam);v[4]=rd32(cam+4);
 return nflstreet_draw_gx(d,NFLSTREET_GX_SET_FOG,1,&c->fog_type,v,5);}
/* 801EDDF8: set the fog byte, returning the previous state. */
static int fog_switch(nflstreet_draw *d,const nflstreet_scenery_context *c,uint32_t value,uint32_t *previous){
 uint8_t *f=at(d,c->fog,1);if(!f)return 0;*previous=value;
 if(*f!=(uint8_t)value){*f=(uint8_t)value;if(!fog_apply(d,c,*f))return 0;*previous=*f==0;}
 return 1;}
static int chan(nflstreet_draw *d,uint32_t ch,uint32_t enable,uint32_t amb,uint32_t mat,uint32_t lights,uint32_t diffuse,uint32_t attn){
 return GX(d,NFLSTREET_GX_SET_CHAN_CTRL,ch,enable,amb,mat,lights,diffuse,attn);}
static int scenery(nflstreet_draw *d,const nflstreet_scenery_context *c,uint32_t instance,uint32_t lights){
 if(!c->enable)return 1;
 const uint8_t *in=at(d,instance,0xbc);if(!in)return 0;if(!rd32(in+0x54))return 1;
 uint32_t flags=rd32(in),fog=0;
 if(flags&0x4000){int skip;if(!culled(d,c,instance,&skip))return 0;if(skip)return 1;}
 if(rd32(in+0xb8)&&!nflstreet_draw_shader(d,rd32(in+0xb8)))return 0;
 uint32_t m[12],n[9];for(unsigned i=0;i<12;++i)m[i]=rd32(in+8+4*i);
 for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)n[3*r+k]=m[4*r+k];
 if(!nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},(uint32_t[]){0x808080ffu},1)||
    !nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)||!nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_NRM_MTX,1,(uint32_t[]){0},n,9)||
    !GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0))return 0;
 if(!nflstreet_draw_blend(d,flags&0x100000?3:flags&0x200000?6:flags&2?2:1))return 0;
 if(flags&0x200){if(!GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,0)||!GX(d,NFLSTREET_GX_SET_ALPHA_COMPARE,4,0x20,0,4,0))return 0;}
 else if(!GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,1)||!GX(d,NFLSTREET_GX_SET_ALPHA_COMPARE,7,0,0,4,0))return 0;
 uint32_t update=(flags&8)?0:1;
 if(flags&0x40000){if(!GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,update)||!nflstreet_draw_depth_range(d,0.0f,1.0f-c->depth_trim))return 0;}
 else if(flags&0x80000){float z=fword(in+0x48);if(!GX(d,NFLSTREET_GX_SET_Z_MODE,1,1,update)||!nflstreet_draw_depth_range(d,z,z))return 0;}
 else if(!GX(d,NFLSTREET_GX_SET_Z_MODE,1,flags&0x400000?7:3,update))return 0;
 if(!(flags&1)&&!fog_switch(d,c,0,&fog))return 0;
 if(!GX(d,NFLSTREET_GX_SET_NUM_CHANS,1))return 0;
 int ok;
 if(flags&0x20000)ok=nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_AMB_COLOR,1,(uint32_t[]){4},(uint32_t[]){rd32(in+0x4c)},1)&&
  chan(d,0,1,0,1,0,0,2)&&chan(d,2,1,0,1,0,0,2);
 else if(flags&0x8000)ok=chan(d,2,0,0,0,0,0,2)&&chan(d,0,1,0,0,lights,2,1);
 else ok=chan(d,2,0,0,0,0,0,2)&&chan(d,0,0,0,flags&0x10000?1:0,0,0,2);
 if(!ok||!nflstreet_draw_select_model(d,instance+0x64))return 0;
 d->s->materials=instance+0x5c;
 if(!rd32(in+0x78)||rd32(in+4)==0xffffffffu){if(!nflstreet_draw_all(d))return 0;}
 else if(!nflstreet_draw_group(d,0,(int32_t)rd32(in+4)))return 0;
 if((flags&0x4c0000)&&(!GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,1)||!nflstreet_draw_depth_range(d,0.0f,1.0f)))return 0;
 uint32_t unused;return (flags&1)||fog_switch(d,c,fog,&unused);}
int nflstreet_scenery_draw(const nflstreet_scenery_context *c,uint32_t instance,uint32_t lights,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!s)return 0;
 nflstreet_draw d={c->draw,s};return scenery(&d,c,instance,lights);}
int nflstreet_field_draw(const nflstreet_scenery_context *c,uint32_t object,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!s||!object)return 0;
 nflstreet_draw d={c->draw,s};static const uint16_t first[5]={0x258,0xe0,0x19c,0x24,0x314};
 for(unsigned i=0;i<5;++i)if(!scenery(&d,c,object+first[i],1))return 0;
 for(unsigned i=0;i<10;++i)if(!scenery(&d,c,object+0x3d0u+0xbcu*i,1))return 0;
 return 1;}
/* 801ECD98: ambient bytes = truncated value * 255 (single), flags | 0x20000. */
static int ambient(nflstreet_draw *d,uint32_t instance,const float v[4]){
 uint8_t *in=at(d,instance,0x50);if(!in)return 0;nflstreet_wr32(in,rd32(in)|0x20000u);
 for(unsigned k=0;k<4;++k)in[0x4c+k]=(uint8_t)nflstreet_fctiwz(v[k]*255.0f);return 1;}
/* 8015B564. */
static int pass(nflstreet_draw *d,const nflstreet_scenery_context *c,uint32_t object,unsigned layer,float alpha){
 uint32_t instance=object+0x2cu+0xbcu*layer,model;const uint8_t *rec=at(d,c->pass_table+0x1cu*layer,0x1c);
 if(!nflstreet_draw_word(d,object+0x3d8u+4u*layer,&model)||!rec)return 0;
 if(!model||!rec[0x19])return 1;
 const float v[4]={c->pass_color,c->pass_color,c->pass_color,alpha*fword(rec+0x14)};
 if(!ambient(d,instance,v))return 0;
 float depth=fword(rec+0xc);
 if(!(depth<=c->pass_depth)){uint8_t *in=at(d,instance,0x4c);if(!in)return 0;
  nflstreet_wr32(in,rd32(in)|0x80000u);nflstreet_wr32(in+0x48,nflstreet_float_bits(depth));}
 return scenery(d,c,instance,1);}
int nflstreet_field_passes_draw(const nflstreet_scenery_context *c,uint32_t object,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!s||!object)return 0;
 nflstreet_draw d={c->draw,s};const uint8_t *offset=at(&d,c->pass_offset,0x10);if(!offset)return 0;
 return pass(&d,c,object,0,c->pass_alpha)&&pass(&d,c,object,1,c->pass_alpha)&&pass(&d,c,object,2,fword(offset+0xc))&&
  pass(&d,c,object,3,c->pass3_alpha)&&pass(&d,c,object,4,c->pass_alpha);}

/* 8018941C push then 80189ED8: the stack slot above the top becomes a copy
   of it (RAM effect), and m receives it. 8018949C pops later. */
static int stack_top(nflstreet_draw *d,const nflstreet_scenery_context *c,int push,uint32_t m[16]){
 uint32_t depth,slots;if(!nflstreet_draw_word(d,c->matrix_stack+4,&depth)||!nflstreet_draw_word(d,c->matrix_stack+8,&slots))return 0;
 uint32_t top=slots+0x40u*depth;if(push&&!copy16(d,top+0x40,top))return 0;
 const uint8_t *t=at(d,push?top+0x40:top,0x40);if(!t)return 0;for(unsigned i=0;i<16;++i)m[i]=rd32(t+4*i);return 1;}
static int write16(nflstreet_draw *d,uint32_t v){return GX(d,NFLSTREET_GX_WRITE,2,v&0xffff);}
int nflstreet_sky_draw(const nflstreet_scenery_context *c,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!s)return 0;
 nflstreet_draw d={c->draw,s};uint32_t m[16],saved[3];nflstreet_draw_reset(&d);
 if(!stack_top(&d,c,1,m))return 0;
 saved[0]=m[3];saved[1]=m[7];saved[2]=m[11];m[3]=m[7]=m[11]=0;
 if(!nflstreet_draw_gx(&d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)||!GX(&d,NFLSTREET_GX_SET_CURRENT_MTX,0))return 0;
 const uint8_t *counts=at(&d,c->sky_tables+0xf02,6);if(!counts)return 0;
 unsigned colors=nflstreet_rd16(counts),quads=nflstreet_rd16(counts+2),tris=nflstreet_rd16(counts+4);
 for(unsigned i=0;i<colors;++i){const uint8_t *f=at(&d,c->sky_tables+16u*i,12);uint8_t *o=at(&d,c->sky_colors+3u*i,3);if(!f||!o)return 0;
  for(unsigned k=0;k<3;++k)o[k]=(uint8_t)nflstreet_fctiwz(fword(f+4*k)*255.0f);}
 if(!GX(&d,NFLSTREET_GX_SET_CHAN_CTRL,0,0,0,1,1,2,1)||!nflstreet_draw_gx(&d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||
    !GX(&d,NFLSTREET_GX_SET_VTX_DESC,9,3)||!GX(&d,NFLSTREET_GX_SET_VTX_DESC,0xb,3)||!GX(&d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||
    !GX(&d,NFLSTREET_GX_SET_TEV_ORDER,0,0xff,0xff,4)||!GX(&d,NFLSTREET_GX_SET_TEV_OP,0,4)||!GX(&d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)||
    !GX(&d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,0,4,0x3c,0,0x7d)||!GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)||
    !GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,0,1,0)||!GX(&d,NFLSTREET_GX_SET_ARRAY,9,c->sky,0x10)||
    !GX(&d,NFLSTREET_GX_SET_ARRAY,0xb,c->sky_colors,3)||!GX(&d,NFLSTREET_GX_BEGIN,0x80,0,(quads&0x3fffu)<<2))return 0;
 static const uint8_t quad_order[8]={6,0xe,4,0xc,2,0xa,0,8},tri_order[6]={0,6,2,8,4,0xa};
 for(unsigned i=0;i<quads;++i){const uint8_t *q=at(&d,c->sky_tables+0x100u+16u*i,16);if(!q)return 0;
  for(unsigned k=0;k<8;++k)if(!write16(&d,nflstreet_rd16(q+quad_order[k])))return 0;}
 if(!GX(&d,NFLSTREET_GX_BEGIN,0x90,0,tris*3u))return 0;
 for(unsigned i=0;i<tris;++i){const uint8_t *q=at(&d,c->sky_tables+0x900u+12u*i,12);if(!q)return 0;
  for(unsigned k=0;k<6;++k)if(!write16(&d,nflstreet_rd16(q+tri_order[k])))return 0;}
 m[3]=saved[0];m[7]=saved[1];m[11]=saved[2];
 return nflstreet_draw_gx(&d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)&&GX(&d,NFLSTREET_GX_SET_CURRENT_MTX,0);}
static int color_byte(nflstreet_draw *d,const uint8_t *f){return GX(d,NFLSTREET_GX_WRITE,1,(uint32_t)nflstreet_fctiwz(fword(f)*255.0f)&0xffu);}
int nflstreet_shadows_draw(const nflstreet_scenery_context *c,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!s)return 0;
 nflstreet_draw d={c->draw,s};uint32_t fog,m[16];nflstreet_draw_reset(&d);
 if(!fog_switch(&d,c,1,&fog)||!GX(&d,NFLSTREET_GX_SET_CULL_MODE,0)||!GX(&d,NFLSTREET_GX_SET_BLEND_MODE,1,4,5,5)||
    !GX(&d,NFLSTREET_GX_SET_Z_COMP_LOC,0)||!GX(&d,NFLSTREET_GX_SET_Z_MODE,1,3,0)||!nflstreet_draw_depth_range(&d,0.01f,0.01f)||
    !nflstreet_draw_gx(&d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||!GX(&d,NFLSTREET_GX_SET_VTX_DESC,9,1)||
    !GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)||!GX(&d,NFLSTREET_GX_SET_VTX_DESC,0xb,1)||!GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,1,5,0)||
    !GX(&d,NFLSTREET_GX_SET_NUM_CHANS,1)||!GX(&d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,1,1,0,0,2)||!GX(&d,NFLSTREET_GX_SET_VTX_DESC,0xd,1)||
    !GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0)||!GX(&d,NFLSTREET_GX_SET_TEV_OP,0,0)||!GX(&d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)||
    !GX(&d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x3c,0,0x7d)||!GX(&d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||
    !GX(&d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)||!nflstreet_draw_texture(&d,0,c->shadow_texture,c->shadow_tlut)||
    !stack_top(&d,c,0,m)||!nflstreet_draw_gx(&d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)||!GX(&d,NFLSTREET_GX_SET_CURRENT_MTX,0))return 0;
 const uint8_t *count=at(&d,c->actor_count,2);if(!count)return 0;
 for(unsigned i=0;i<nflstreet_rd16(count);++i){uint32_t actor;const uint8_t *a;
  if(!c->actors||!nflstreet_draw_word(&d,c->actors+4u*i,&actor)||!(a=at(&d,actor,0x1340)))return 0;
  if(!a[0x133d])continue;
  uint32_t alpha=(uint32_t)nflstreet_fctiwz(fword(a+0x10a8)*255.0f)&0xffu;
  if(!GX(&d,NFLSTREET_GX_BEGIN,0xa0,0,4))return 0;
  for(unsigned k=0;k<4;++k)if(!GX(&d,NFLSTREET_GX_WRITE,4,rd32(a+0x10ac+12*k))||!GX(&d,NFLSTREET_GX_WRITE,4,rd32(a+0x10b0+12*k))||
   !GX(&d,NFLSTREET_GX_WRITE,4,rd32(a+0x10b4+12*k))||!color_byte(&d,a+0x109c)||!color_byte(&d,a+0x10a0)||!color_byte(&d,a+0x10a4)||
   !GX(&d,NFLSTREET_GX_WRITE,1,alpha)||!GX(&d,NFLSTREET_GX_WRITE,4,rd32(a+0x10dc+8*k))||!GX(&d,NFLSTREET_GX_WRITE,4,rd32(a+0x10e0+8*k)))return 0;}
 uint32_t unused;
 return nflstreet_draw_depth_range(&d,0.0f,1.0f)&&GX(&d,NFLSTREET_GX_SET_Z_MODE,1,3,1)&&fog_switch(&d,c,fog,&unused);}

int nflstreet_ring_draw(const nflstreet_scenery_context *c,uint32_t object,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!s||!object)return 0;
 nflstreet_draw d={c->draw,s};const uint8_t *o=at(&d,object,0xfc);if(!o)return 0;
 if(!rd32(o+0xf8)||!(rd32(o+0x20)&2))return 1;
 uint32_t type=rd32(o+0x28);int drawn=1;
 if(type==6)return 0;
 if(type==5||type==7){const uint8_t *flag=at(&d,c->ring_flags+(type==5?0u:1u),1);if(!flag)return 0;
  if(*flag!=1)drawn=0;
  else{const uint8_t *k=at(&d,c->ring_colors+16u*rd32(o+0x30),16);if(!k)return 0;
   const float v[4]={fword(k)*c->ring_scale,fword(k+4)*c->ring_scale,fword(k+8)*c->ring_scale,fword(k+12)};
   if(!ambient(&d,object+0x3c,v))return 0;}}
 if(!drawn)return 1;
 return GX(&d,NFLSTREET_GX_SET_Z_MODE,1,3,0)&&nflstreet_draw_texture(&d,0,0,0)&&scenery(&d,c,object+0x3c,1);}

/* 8018941C + 80189BB0: the slot above the top becomes the top translated by v
   (t_r += (x*m_r0 + y*m_r1 + z*m_r2) with fused multiply-adds). Returns it. */
static uint8_t *push_translate(nflstreet_draw *d,const nflstreet_scenery_context *c,uint32_t v_token,uint32_t *slot){
 uint32_t depth,slots;if(!nflstreet_draw_word(d,c->matrix_stack+4,&depth)||!nflstreet_draw_word(d,c->matrix_stack+8,&slots))return NULL;
 uint32_t top=slots+0x40u*depth;const uint8_t *v=at(d,v_token,12);uint8_t *m;
 if(!v||!copy16(d,top+0x40,top)||!(m=at(d,top+0x40,0x30)))return NULL;
 float x=fword(v),y=fword(v+4),z=fword(v+8);
 for(unsigned r=0;r<3;++r){const uint8_t *q=m+16*r;
  float t=nflstreet_fmadds(z,fword(q+8),nflstreet_fmadds(x,fword(q),y*fword(q+4)));
  nflstreet_wr32(m+16*r+12,nflstreet_float_bits(fword(q+12)+t));}
 *slot=top+0x40;return m;}
/* 800D9FA8: projected size of the reference box at the line (801EC24C into
   801CB1C4 bounds, 801DE744 length), mapped to [1, 10.5]. */
static int distance_scale(nflstreet_draw *d,const nflstreet_scenery_context *c,uint32_t object,float *out){
 uint32_t slot;if(!push_translate(d,c,object+4,&slot))return 0;
 if(!copy16(d,c->cull_projection,c->projection)||!copy16(d,c->cull_modelview,slot))return 0;
 const uint8_t *P=at(d,c->cull_projection,0x40),*M=at(d,c->cull_modelview,0x30),*B=at(d,c->line_box,0x20);if(!P||!M||!B)return 0;
 float proj[16],m[12],box[8],pts[24],lo[3]={3.4028234663852886e38f,3.4028234663852886e38f,3.4028234663852886e38f},hi[3];
 for(unsigned k=0;k<3;++k)hi[k]=-lo[k];
 for(unsigned i=0;i<16;++i)proj[i]=fword(P+4*i);for(unsigned i=0;i<12;++i)m[i]=fword(M+4*i);for(unsigned i=0;i<8;++i)box[i]=fword(B+4*i);
 corners(box,m,pts);
 for(unsigned i=0;i<8;++i){const float *p=pts+3*i;float z=p[2];
  float x=nflstreet_fmadds(proj[0],p[0],proj[2]*z),y=nflstreet_fmadds(proj[5],p[1],proj[6]*z),zz=nflstreet_fmadds(proj[10],z,proj[11]),inv=1.0f/(proj[14]*z);
  const float q[3]={x*inv,y*inv,zz*inv};
  for(unsigned k=0;k<3;++k){if(!(q[k]>=lo[k]))lo[k]=q[k];if(!(q[k]<=hi[k]))hi[k]=q[k];}}
 float dx=hi[0]-lo[0],dy=hi[1]-lo[1],len=sqrtf(nflstreet_fmadds(dx,dx,dy*dy)),scale=0.0f;
 if(!(len>=0.08f)){float f=0.08f/len;if(!(f<1.0f)){if(!(f<=20.0f))f=20.0f;scale=(f-1.0f)*0.5f;}}
 *out=scale+1.0f;return 1;}
static int fwrite4(nflstreet_draw *d,float v){return GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(v));}
static int gates(nflstreet_draw *d,const nflstreet_scenery_context *c,int *open){
 uint32_t v,w;const uint8_t *b;*open=0;
 if(!nflstreet_draw_word(d,c->game_state+0x30,&v))return 0;if(!v)return 1;
 if(c->gate_7b88){if(!(b=at(d,c->gate_7b88+0x182,1)))return 0;if(*b)return 1;}
 if(c->gate_7f40){if(!nflstreet_draw_word(d,c->gate_7f40+0xb64,&v)||!nflstreet_draw_word(d,c->gate_7f40+0xb60,&w))return 0;
  if((v&0xc00880u)||w==4)return 1;}
 if(!(b=at(d,c->gate_7258,1)))return 0;if(*b!=1)return 1;
 if(!nflstreet_draw_word(d,c->gate_count,&v))return 0;if((int32_t)v>0)return 1;
 uint8_t *cache=at(d,c->mode_cache,4);if(!nflstreet_draw_word(d,c->mode_source,&v)||!cache)return 0;nflstreet_wr32(cache,v);
 if(v==7)return 1;
 *open=1;return 1;}
int nflstreet_line_draw(const nflstreet_scenery_context *c,uint32_t object,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!s||!object)return 0;
 nflstreet_draw d={c->draw,s};int open;if(!gates(&d,c,&open))return 0;if(!open)return 1;
 uint8_t *o=at(&d,object,0x124);if(!o)return 0;
 if(fword(o+0xe0)>0.0f){
  float scale,width,ratio;uint32_t fog,slot,m[16];if(!distance_scale(&d,c,object,&scale))return 0;
  nflstreet_draw_reset(&d);
  uint32_t color=0;for(unsigned k=0;k<4;++k)color|=((uint32_t)nflstreet_fctiwz(fword(o+0xd4+4*k)*255.0f)&0xffu)<<(24-8*k);
  if(!fog_switch(&d,c,0,&fog)||!GX(&d,NFLSTREET_GX_SET_CULL_MODE,0)||!GX(&d,NFLSTREET_GX_SET_BLEND_MODE,1,4,5,5)||
     !GX(&d,NFLSTREET_GX_SET_Z_COMP_LOC,0)||!GX(&d,NFLSTREET_GX_SET_Z_MODE,0,3,0)||!nflstreet_draw_depth_range(&d,0.0f,1.0f)||
     !nflstreet_draw_gx(&d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||!GX(&d,NFLSTREET_GX_SET_VTX_DESC,9,1)||
     !GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)||!nflstreet_draw_gx(&d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},&color,1)||
     !GX(&d,NFLSTREET_GX_SET_NUM_CHANS,1)||!GX(&d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,0,0,1,2,1)||!GX(&d,NFLSTREET_GX_SET_VTX_DESC,0xd,1)||
     !GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0)||!GX(&d,NFLSTREET_GX_SET_TEV_OP,0,0)||!GX(&d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)||
     !GX(&d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x3c,0,0x7d)||!GX(&d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||
     !GX(&d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)||!nflstreet_draw_texture(&d,0,rd32(o+0x11c),rd32(o+0x120))||
     !push_translate(&d,c,object+4,&slot))return 0;
  const uint8_t *t=at(&d,slot,0x40),*w=at(&d,c->game_state+0x90,4),*r=at(&d,c->line_ratio,4);if(!t||!w||!r)return 0;
  for(unsigned i=0;i<16;++i)m[i]=rd32(t+4*i);width=fword(w);ratio=fword(r);
  if(!nflstreet_draw_gx(&d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)||!GX(&d,NFLSTREET_GX_SET_CURRENT_MTX,0)||
     !GX(&d,NFLSTREET_GX_BEGIN,0x98,0,4))return 0;
  const float far_v=1.0f-ratio*0.5f,near_v=ratio*0.5f,y0=scale*((1.0f-ratio)*0.55555999f),y1=scale*((1.0f-ratio)*-0.55555999f);
  const float v[4][5]={{width,y0,0.0f,0.9f,far_v},{width,y1,0.0f,0.9f,near_v},{-width,y0,0.0f,0.1f,far_v},{-width,y1,0.0f,0.1f,near_v}};
  for(unsigned i=0;i<4;++i)for(unsigned k=0;k<5;++k)if(!fwrite4(&d,v[i][k]))return 0;
  uint32_t unused;if(!fog_switch(&d,c,fog,&unused))return 0;}
 /* Alpha pulse: reverse at the bounds. */
 float e=fword(o+0xe0)+fword(o+0xe4),step=fword(o+0xe4);nflstreet_wr32(o+0xe0,nflstreet_float_bits(e));
 if((!(step<=0.0f)&&e>fword(o+0xec))||(!(step>=0.0f)&&!(e>=fword(o+0xe8))))nflstreet_wr32(o+0xe4,nflstreet_float_bits(-step));
 return 1;}
