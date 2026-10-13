#include "scenery_draw.h"
#include "draw_engine.h"
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
