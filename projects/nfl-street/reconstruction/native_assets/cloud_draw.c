#include "cloud_draw.h"
#define rd32 nflstreet_rd32
#define GX NFLSTREET_GX
static float fword(const uint8_t *p){return nflstreet_as_float(rd32(p));}
static uint8_t *at(nflstreet_draw *d,uint32_t token,uint32_t size){return nflstreet_draw_at(d,token,size);}
static int load(nflstreet_draw *d,uint32_t token,float *v){const uint8_t *p=at(d,token,4);if(!p)return 0;*v=fword(p);return 1;}
static int store(nflstreet_draw *d,uint32_t token,float v){uint8_t *p=at(d,token,4);if(!p)return 0;nflstreet_wr32(p,nflstreet_float_bits(v));return 1;}
/* 8015F0E8 (801DE858 on +00 and +0C, then +18 and +1C): out = c + (b - c) * t
   per float, each read before its own store (out may be b or c). */
static int lerp(nflstreet_draw *d,uint32_t out,uint32_t b,uint32_t c,float t){
 for(unsigned k=0;k<8;++k){float x,y;if(!load(d,c+4*k,&y)||!load(d,b+4*k,&x)||!store(d,out+4*k,nflstreet_fmadds(x-y,t,y)))return 0;}
 return 1;}
static int copy(nflstreet_draw *d,uint32_t to,uint32_t from,uint32_t n){uint8_t *t=at(d,to,n);const uint8_t *f=at(d,from,n);if(!t||!f)return 0;memmove(t,f,n);return 1;}
/* 8015F164(8030FEB4): the period's color record, blended toward the next
   (period 2) or from the previous (period 0) by the period blend, and
   toward the overlay records by the overlay amount (not in period 3). */
static int sky_record(nflstreet_draw *d,const nflstreet_cloud_context *c){
 uint32_t period,out=c->sky_record,tab=c->palette;float blend,over,t;
 if(!nflstreet_draw_word(d,c->period,&period)||!load(d,c->period+4,&blend)||!load(d,c->overlay,&over))return 0;
 const uint32_t next=(period+1u)&3u,prev=period?period-1u:3u;int ok;
 if(period==1)ok=copy(d,out,tab+0x20,0x20);
 else if(period==2){t=blend+blend;if(!(t<=c->blend_cap))t=c->blend_cap;ok=lerp(d,out,tab+0x40,tab+(prev<<5),t);}
 else if(period==0){t=blend-c->blend_half;t=t+t;if(!(t>=c->blend_floor))t=c->blend_floor;ok=lerp(d,out,tab+(next<<5),tab,t);}
 else ok=copy(d,out,tab+0x60,0x20);
 if(!ok)return 0;
 if(!(over==c->blend_floor)&&period!=3){
  if(over<=c->blend_half){if(!lerp(d,out,tab+0x80,out,over+over))return 0;}
  else{t=over-c->blend_half;if(!lerp(d,out,tab+0xa0,tab+0x80,t+t))return 0;}}
 if(period==2){if(!(blend<=c->blend_half)){t=blend-c->blend_half;return lerp(d,out,tab+(next<<5),out,t+t);}}
 else if(period==0){if(!(blend>c->blend_half))return lerp(d,out,out,tab+(prev<<5),blend+blend);}
 return 1;}
/* 8015F3F0: color A (+20) = {+00, +04, +08, +1C}, color B (+30) = {+0C, +10, +14, 802606E4}. */
static int sky_colors(nflstreet_draw *d,const nflstreet_cloud_context *c){
 const uint8_t *r=at(d,c->sky_record,0x20);if(!r)return 0;uint32_t w[8];
 const unsigned from[7]={0,1,2,7,3,4,5};for(unsigned i=0;i<7;++i)w[i]=rd32(r+4*from[i]);w[7]=nflstreet_float_bits(c->color_alpha);
 uint8_t *o=at(d,c->sky_record+0x20,0x20);if(!o)return 0;for(unsigned i=0;i<8;++i)nflstreet_wr32(o+4*i,w[i]);return 1;}
/* 8015F450's color byte: the float times 255 as an unsigned word (fctiwz,
   offset by 2^31 at and above it), 255 above 254. */
static uint32_t color_byte(float v){
 if(v>=2147483648.0f)return 0xff;uint32_t u=(uint32_t)nflstreet_fctiwz(v);return u>0xfe?0xff:u;}
static int write(nflstreet_draw *d,unsigned bytes,uint32_t v){return GX(d,NFLSTREET_GX_WRITE,bytes,v);}
static int fwrite4(nflstreet_draw *d,float v){return write(d,4,nflstreet_float_bits(v));}
/* 8015F450(record). */
static int cloud(nflstreet_draw *d,const nflstreet_cloud_context *c,uint32_t record){
 const uint8_t *r=at(d,record,0x24);if(!r)return 0;const unsigned layers=r[0];float step,share;
 if(layers>1){step=fword(r+0xc)/(float)(int32_t)(layers-1u);share=c->one/(float)layers;}else{step=c->zero;share=c->one;}
 const float h=fword(r+8)*c->half,x=fword(r+0x18),z=fword(r+0x1c),x0=x-h,x1=x+h,z0=z-h,z1=z+h;float y=fword(r+0x20),fade;
 const uint8_t *a=at(d,c->sky_record+0x20,12),*b=at(d,c->sky_record+0x30,12);
 if(!a||!b||!load(d,c->sky_record+0x18,&fade)||!GX(d,NFLSTREET_GX_BEGIN,0x80,0,(layers+1u)<<2))return 0;
 for(unsigned i=0;i<=layers;++i){
  y=y-step;float f=c->one-(float)i*share*fword(r+0x10)*fade;if(!(f>=c->zero))f=c->zero;
  const float alpha=share*fword(r+0x14);const uint32_t A=alpha>=c->byte_scale?0xff:(uint32_t)nflstreet_fctiwz(alpha)&0xffu;uint32_t rgb[3];
  for(unsigned k=0;k<3;++k)rgb[k]=color_byte(nflstreet_fmadds(f,fword(a+4*k),fword(b+4*k))*c->byte_scale);
  const float corner[4][2]={{x0,z0},{x0,z1},{x1,z1},{x1,z0}};
  for(unsigned v=0;v<4;++v)if(!fwrite4(d,corner[v][0])||!fwrite4(d,y)||!fwrite4(d,corner[v][1])||!write(d,1,rgb[0])||!write(d,1,rgb[1])||
   !write(d,1,rgb[2])||!write(d,1,A)||!write(d,1,v))return 0;}
 return 1;}
int nflstreet_clouds_draw(const nflstreet_cloud_context *c,nflstreet_render_state *s){
 if(!c||!c->scene||!c->scene->draw||!c->scene->draw->span||!c->scene->draw->emit||!c->stacks||!s)return 0;
 nflstreet_draw d={c->scene->draw,s};uint32_t texture;const nflstreet_matrix_stacks *k=c->stacks;
 if(!nflstreet_draw_word(&d,c->texture,&texture))return 0;if(!texture)return 1;
 if(!GX(&d,NFLSTREET_GX_LOAD_TEX_OBJ,texture,0))return 0;
 nflstreet_draw_reset(&d);
 if(!sky_record(&d,c)||!sky_colors(&d,c)||!nflstreet_matrix_push(&d,k)||!nflstreet_matrix_rotate(&d,k,0,0x400000u)||
    !GX(&d,NFLSTREET_GX_SET_CURRENT_MTX,0)||!nflstreet_matrix_load_top(&d,k)||!nflstreet_matrix_pop(&d,k)||
    !GX(&d,NFLSTREET_GX_SET_Z_MODE,0,0,0)||!nflstreet_draw_gx(&d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||
    !GX(&d,NFLSTREET_GX_SET_VTX_DESC,9,1)||!GX(&d,NFLSTREET_GX_SET_VTX_DESC,0xb,1)||!GX(&d,NFLSTREET_GX_SET_VTX_DESC,0xd,2)||
    !GX(&d,NFLSTREET_GX_SET_NUM_CHANS,1)||!GX(&d,NFLSTREET_GX_SET_CHAN_CTRL,0,0,0,1,1,2,1)||!GX(&d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||
    !GX(&d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)||!GX(&d,NFLSTREET_GX_SET_TEV_OP,0,0)||!GX(&d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)||
    !GX(&d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,0,4,0x3c,0,0x7d)||!GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)||
    !GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,1,5,0)||!GX(&d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0)||
    !GX(&d,NFLSTREET_GX_SET_ARRAY,0xd,c->texcoords,8)||!GX(&d,NFLSTREET_GX_SET_BLEND_MODE,1,4,5,5)||!GX(&d,NFLSTREET_GX_SET_Z_COMP_LOC,0)||
    !GX(&d,NFLSTREET_GX_SET_ALPHA_COMPARE,4,0,0,4,0))return 0;
 const uint8_t *n=at(&d,c->count,2);if(!n)return 0;
 for(uint32_t i=0;i<nflstreet_rd16(n);++i){uint32_t record;if(!nflstreet_draw_word(&d,c->records+4u*i,&record)||!cloud(&d,c,record))return 0;}
 return GX(&d,NFLSTREET_GX_SET_BLEND_MODE,0,0,0,5)&&nflstreet_matrix_load_top(&d,k)&&nflstreet_draw_shader(&d,0);}
int nflstreet_sky_node_draw(const nflstreet_cloud_context *c,nflstreet_render_state *s){
 if(!c||!c->scene||!c->scene->draw||!c->scene->draw->span||!c->scene->draw->emit||!s)return 0;
 nflstreet_draw d={c->scene->draw,s};const uint8_t *e=at(&d,c->enable,1),*k=at(&d,c->sky_enable,1),*g;
 if(!e||!k)return 0;if(!e[0]||!k[0])return 1;
 if(!GX(&d,NFLSTREET_GX_SET_Z_MODE,0,3,0)||!(g=at(&d,c->gx_state+0x420,0x1c)))return 0;
 uint32_t v[7],saved[3];v[0]=nflstreet_float_bits((float)rd32(g));for(unsigned i=1;i<7;++i)v[i]=rd32(g+4*i);
 saved[0]=v[2];saved[1]=v[4];saved[2]=v[5];v[2]=v[4]=v[5]=nflstreet_float_bits(c->projection_value);
 if(!nflstreet_draw_gx(&d,NFLSTREET_GX_SET_PROJECTION_V,0,NULL,v,7)||!nflstreet_sky_draw(c->scene,s)||!nflstreet_clouds_draw(c,s))return 0;
 v[2]=saved[0];v[4]=saved[1];v[5]=saved[2];
 return nflstreet_draw_gx(&d,NFLSTREET_GX_SET_PROJECTION_V,0,NULL,v,7)&&GX(&d,NFLSTREET_GX_SET_Z_MODE,1,3,1);}
