#include "font.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
static unsigned u16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int span(size_t n,size_t p,size_t z){return p<=n&&z<=n-p;}
int nflstreet_font_open(const uint8_t*b,size_t n,nflstreet_font*out){
 if(!b||!out||n<32||memcmp(b,"FNTG",4))return 0;
 nflstreet_font f={b,n,u32(b+20),u32(b+24),u32(b+28),u16(b+10),0,0,b[19]};
 if(!f.count||!span(n,f.glyphs,(size_t)f.count*12)||!span(n,f.atlas,16))return 0;
 f.width=u16(b+f.atlas+4);f.height=u16(b+f.atlas+6);
 if(!f.width||!f.height||!f.line_height)return 0;
 for(unsigned i=0;i<f.count;++i){const uint8_t*g=b+f.glyphs+i*12;if(i&&u16(g)<=u16(g-12))return 0;if(u16(g+4)+g[2]>f.width||u16(g+6)+g[3]>f.height)return 0;}
 if(f.kerning&&(!span(n,f.kerning,4)||!span(n,f.kerning+4,(size_t)u32(b+f.kerning)*8)))return 0;
 *out=f;return 1;
}
const uint8_t*nflstreet_font_glyph(const nflstreet_font*f,uint32_t code){
 if(!f||!f->bytes)return NULL;
 if(code>=32&&code-32<f->count){const uint8_t*g=f->bytes+f->glyphs+(code-32)*12;if(u16(g)==code)return g;}
 unsigned n=f->count;const uint8_t*g=f->bytes+f->glyphs;
 while(n){unsigned half=n>>1;const uint8_t*at=g+half*12;unsigned value=u16(at);if(code<value)n=half;else if(code==value)return at;else{g=at+12;n-=half+1;}}
 return NULL;
}
int nflstreet_font_kerning(const nflstreet_font*f,uint32_t a,uint32_t b){
 if(!f||!f->kerning)return 0;
 const uint8_t*p=f->bytes+f->kerning;unsigned n=u32(p);
 for(unsigned i=0;i<n;++i){const uint8_t*r=p+4+i*8;if(u16(r)==a&&u16(r+2)==b)return (int)(int8_t)r[4];}return 0;
}
static int text_size(const char*s,size_t*n){if(!s)return 0;for(size_t i=0;i<=354;++i)if(!s[i]){*n=i;return 1;}return 0;}
int nflstreet_font_measure(const nflstreet_font*f,const char*s,unsigned flags,int32_t*out){
 size_t n;if(!f||!out||!text_size(s,&n))return 0;int32_t width=0,maximum=0;uint32_t previous=UINT32_MAX;
 for(size_t i=0;i<n;++i){uint32_t c=(uint8_t)s[i];if(c==10){if(width>maximum)maximum=width;width=0;previous=UINT32_MAX;continue;}
  const uint8_t*g=nflstreet_font_glyph(f,c);if(g){if(previous!=UINT32_MAX&&!(flags&4))width+=nflstreet_font_kerning(f,previous,c);width+=(int8_t)g[8];}previous=c;
 }if(width>maximum)maximum=width;*out=maximum;return 1;
}
int nflstreet_font_layout(const nflstreet_font*f,const char*s,unsigned flags,double limit,nflstreet_glyph_box*out,size_t cap,unsigned*count){
 size_t n;if(!f||!count||!out||!isfinite(limit)||!text_size(s,&n)||cap<n)return 0;
 nflstreet_glyph_box boxes[354];unsigned used=0;size_t i=0;int32_t y=0;
 while(i<n){unsigned start=used;int32_t x=0;uint32_t previous=UINT32_MAX;
  for(;i<n;++i){uint32_t c=(uint8_t)s[i];if(c==10){++i;break;}if(limit>0&&limit<(double)(float)x)break;
   const uint8_t*g=nflstreet_font_glyph(f,c);if(g){if(previous!=UINT32_MAX&&!(flags&4))x+=nflstreet_font_kerning(f,previous,c);
    boxes[used++]=(nflstreet_glyph_box){x+(int8_t)g[9],y+(int8_t)g[10],g[2],g[3],(uint16_t)u16(g+4),(uint16_t)u16(g+6)};x+=(int8_t)g[8];}previous=c;
  }
  int32_t shift=flags&1?x/2:flags&2?x:0;for(unsigned j=start;j<used;++j)boxes[j].x-=shift;y+=(int32_t)f->line_height;
 }memcpy(out,boxes,used*sizeof *boxes);*count=used;return 1;
}
int nflstreet_font_quad(const nflstreet_font*f,const nflstreet_glyph_box*g,float out[16]){
 if(!f||!g||!out||!f->width||!f->height)return 0;
 float x0=(float)g->x,x1=(float)(g->x+(int32_t)g->width),y0=(float)g->y,y1=(float)(g->y+(int32_t)g->height);
 float u0=(float)g->u/(float)f->width,u1=(float)(g->u+g->width)/(float)f->width,v0=(float)g->v/(float)f->height,v1=(float)(g->v+g->height)/(float)f->height;
 float q[16]={x0,y0,u0,v0,x1,y0,u1,v0,x1,y1,u1,v1,x0,y1,u0,v1};memcpy(out,q,sizeof q);return 1;
}

void nflstreet_font_palettes(uint16_t palettes[2][16]){
 if(!palettes)return;
 for(unsigned i=0;i<16;++i){
  palettes[0][i]=i<8?(uint16_t)(((i*255/7)<<8)|255):65535;
  palettes[1][i]=i<8?0:(uint16_t)((((i-8)*255/7)<<8)|255);
 }
}
static void put16(uint8_t*p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
int nflstreet_font_atlas(const nflstreet_font*f,unsigned pass,nflstreet_texture*out){
 if(!f||!f->bytes||!out||pass>1||!f->width||!f->height)return 0;
 size_t tilesx=(f->width+7)/8,tilesy=(f->height+7)/8,bytes=tilesx*tilesy*32;
 if(f->width>4096||f->height>4096||!span(f->size,f->atlas,16)||!span(f->size,f->atlas+16,bytes))return 0;
 /* Adapt the already bounded GX decoder; this is an in-memory descriptor,
    not a recovered MMAP object and not a source screen. */
 int paletted=(u32(f->bytes+12)&4)!=0;size_t offset=100,total=offset+bytes;
 uint8_t*b=calloc(total,1);if(!b)return 0;
 memcpy(b,"MMAP",4);put16(b+4,2);put16(b+14,1);put16(b+16,1);put32(b+24,40);put32(b+28,56);
 put16(b+40,f->width);put16(b+42,f->height);put16(b+44,paletted?8:0);put32(b+48,(uint32_t)bytes);put32(b+52,(uint32_t)offset);
 put32(b+60,32);put32(b+64,68);uint16_t palettes[2][16];nflstreet_font_palettes(palettes);
 for(unsigned i=0;i<16;++i)put16(b+68+i*2,palettes[pass][i]);
 memcpy(b+offset,f->bytes+f->atlas+16,bytes);
 int ok=nflstreet_texture_open(b,total,out);free(b);return ok;
}
