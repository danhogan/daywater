#include "text.h"
#include <math.h>
#include <string.h>
static unsigned u16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void put16(uint8_t*p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static int length(const char*s,unsigned*n){if(!s)return 0;for(unsigned i=0;i<=354;++i){if(!s[i]){*n=i;return 1;}if(s[i]=='\n')return 0;}return 0;}
int nflstreet_ui_text_prepare(uint8_t object[40],const nflstreet_font*f,const char*s){
 unsigned n;int32_t width;if(!object||!f||!length(s,&n)||(u16(object)&0x100)||object[15]>1||!nflstreet_font_measure(f,s,u16(object),&width))return 0;
 uint8_t b[40];memcpy(b,object,40);put16(b+20,(unsigned)width);b[15]=1;put16(b+36,0);put16(b+38,n);
 if(!(u16(b)&0x200))put16(b+22,f->line_height);
 put16(b,u16(b)&0x7fff);memcpy(object,b,40);return 1;
}
int nflstreet_ui_text_draw(const uint8_t b[40],const char*s,const float ctx[8],nflstreet_ui_text_plan*out){
 unsigned n;if(!b||!ctx||!out||!length(s,&n)||(u16(b)&0x8300)||!(u16(b)&0x50)||b[15]!=1||u16(b+36)!=0||u16(b+38)!=n||b[4]!=255||b[5]!=255)return 0;
 for(unsigned i=0;i<8;++i)if(!isfinite(ctx[i]))return 0;
 nflstreet_ui_text_plan p={0};p.flags=u16(b)&15;p.archive=(int8_t)b[3];p.entry=(int8_t)b[2];p.visible=b[16]==0;
 for(unsigned i=0;i<3;++i)p.position[i]=(float)(int16_t)u16(b+6+i*2);
 /* Source float DAT80263BA8 converted to double as in original PPC. */
 const double scale=(double)0.003921568859368563f;
 for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<4;++i){float added=(float)b[24+pass*4+i]+ctx[4+i];float scaled=(float)(scale*(double)added);p.color[pass][i]=scaled*ctx[i];}
 if(u16(b)&0x50)for(unsigned i=0;i<3;++i)p.shadow_offset[i]=(float)(int8_t)b[12+i];

 *out=p;return 1;
}

static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static const uint8_t*owned(const uint8_t*b,size_t n,uint32_t base,uint32_t v,size_t z){if(v<base||v-base>n||z>n-(v-base))return NULL;return b+v-base;}
int nflstreet_ui_text_find(const uint8_t*b,size_t n,uint32_t base,unsigned node,unsigned ordinal,uint8_t object[40],const char**text){
 if(!b||!object||!text||n<32||node>=u32(b))return 0;
 const uint8_t*nodes=owned(b,n,base,u32(b+4),(size_t)u32(b)*20);if(!nodes)return 0;const uint8_t*r=nodes+node*20;
 unsigned count=u32(r+4);const uint8_t*list=owned(b,n,base,u32(r+8),(size_t)count*4);if(!list)return 0;
 for(unsigned i=0;i<count;++i){const uint8_t*obj=owned(b,n,base,u32(list+i*4),12);if(!obj)return 0;
  unsigned links=u32(obj+4);const uint8_t*link=owned(b,n,base,u32(obj+8),(size_t)links*8);if(!link)return 0;
  for(unsigned j=0;j<links;++j){const uint8_t*l=link+j*8;if(u16(l)!=8)continue;if(ordinal){--ordinal;continue;}
   const uint8_t*cell=owned(b,n,base,u32(l+4),4);if(!cell)return 0;
   uint32_t off=u32(cell);if(off>n||40>n-off)return 0;const uint8_t*t=b+off;
   uint32_t relative=u32(t+32);const uint8_t*d;if(relative>=base){d=owned(b,n,base,relative,12);if(!d)return 0;}else{if(relative>n-off||12>n-off-relative)return 0;d=t+relative;}
   const uint8_t*s=owned(b,n,base,u32(d+8),1);if(!s)return 0;
   size_t available=n-(size_t)(s-b);if(!memchr(s,0,available))return 0;
   memcpy(object,t,40);*text=(const char*)s;return 1;
  }
 }return 0;
}
