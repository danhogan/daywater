#include "gx_texobj.h"
#include "draw_engine.h"
#include <string.h>
#define rd32 nflstreet_rd32
#define wr32 nflstreet_wr32
void nflstreet_gx_init_tex_obj(uint8_t o[0x20],uint32_t image,uint16_t width,uint16_t height,uint32_t format,uint32_t wrap_s,uint32_t wrap_t){
 memset(o,0,0x20);
 uint32_t w0=(wrap_s&0xffffffffu);w0=(w0&~0xcu)|(wrap_t<<2);w0=(w0&~0x10u)|0x10u;w0=(w0&~0xe0u)|0x80u;wr32(o,w0);
 wr32(o+0x14,format);
 uint32_t w2=(rd32(o+8)&0xfffffc00u)|((uint32_t)width-1u);w2=(w2&~0x000ffc00u)|(((uint32_t)height-1u)<<10);
 w2=(w2&~0x00f00000u)|((format<<20)&0x00f00000u);wr32(o+8,w2);
 wr32(o+12,(rd32(o+12)&0xffe00000u)|((image>>5)&0x01ffffffu));
 /* Tile class and tile shifts by format (jump table 80298A7C). */
 static const uint8_t cls[15]={1,2,2,2,2,2,3,2,1,2,2,2,2,2,0},xs[15]={3,3,3,2,2,2,2,2,3,3,2,2,2,2,3},ys[15]={3,2,2,2,2,2,2,2,3,2,2,2,2,2,3};
 unsigned f=format&15u,c=f<15?cls[f]:2,x=f<15?xs[f]:2,y=f<15?ys[f]:2;
 o[0x1e]=(uint8_t)c;
 int32_t tiles=(((int32_t)width+(1<<x)-1)>>x)*(((int32_t)height+(1<<y)-1)>>y);
 o[0x1c]=(uint8_t)((uint32_t)tiles>>8&0x7fu);o[0x1d]=(uint8_t)tiles;
 o[0x1f]|=2u;}
void nflstreet_gx_init_tex_obj_ci(uint8_t o[0x20],uint32_t image,uint16_t width,uint16_t height,uint32_t format,uint32_t wrap_s,uint32_t wrap_t,
 uint32_t tlut_name){
 nflstreet_gx_init_tex_obj(o,image,width,height,format,wrap_s,wrap_t);o[0x1f]&=(uint8_t)~2u;wr32(o+0x18,tlut_name);}
int nflstreet_gx_init_tex_obj_lod(uint8_t o[0x20],const nflstreet_gx_texobj_constants *k,uint32_t min_filter,uint32_t mag_filter,
 float min_lod,float max_lod,float lod_bias,uint8_t bias_clamp,uint8_t edge_lod,uint32_t anisotropy){
 if(min_filter>5)return 0;
 if(!(lod_bias>=k->bias_min))lod_bias=k->bias_min;else if(lod_bias>=k->bias_cap)lod_bias=k->bias_max;
 uint32_t w0=rd32(o);
 w0=(w0&~0x0001fe00u)|(((uint32_t)nflstreet_fctiwz(k->bias_scale*lod_bias)&0xffu)<<9);
 w0=(w0&~0x10u)|((mag_filter==1?1u:0u)<<4);
 w0=(w0&~0xe0u)|((uint32_t)k->filters[min_filter]<<5);
 w0=(w0&~0x100u)|((bias_clamp==0?1u:0u)<<8);
 w0&=~0x20000u;w0&=~0x40000u;
 w0=(w0&~0x00180000u)|((anisotropy<<19)&0xfff80000u);
 w0=(w0&~0x00200000u)|(((uint32_t)edge_lod<<21)&0x1fe00000u);
 wr32(o,w0);
 if(!(min_lod>=k->lod_min))min_lod=k->lod_min;else if(min_lod>k->lod_max)min_lod=k->lod_max;
 uint32_t lo=(uint32_t)nflstreet_fctiwz(k->lod_scale*min_lod);
 if(!(max_lod>=k->lod_min))max_lod=k->lod_min;else if(max_lod>k->lod_max)max_lod=k->lod_max;
 uint32_t w1=(rd32(o+4)&0xffffff00u)|(lo&0xffu);
 w1=(w1&~0xff00u)|(((uint32_t)nflstreet_fctiwz(k->lod_scale*max_lod)&0xffu)<<8);wr32(o+4,w1);
 return 1;}
void nflstreet_gx_init_tlut_obj(uint8_t o[12],uint32_t lut,uint32_t format,uint16_t entries){
 wr32(o,(0u&~0xc00u)|((format<<10)&0xfffffc00u));
 wr32(o+4,(rd32(o+4)&0xffe00000u)|((lut>>5)&0x01ffffffu));wr32(o+4,(rd32(o+4)&0x00ffffffu)|0x64000000u);
 o[8]=(uint8_t)(entries>>8);o[9]=(uint8_t)entries;}
