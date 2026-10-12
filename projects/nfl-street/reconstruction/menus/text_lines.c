#include "text_lines.h"
#include <string.h>
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void ps(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
static void pw(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int advance(const nflstreet_font*f,unsigned c){const uint8_t*g=nflstreet_font_glyph(f,c);return g?(int8_t)g[8]:0;}
static int append(nflstreet_ui_text_lines*p,unsigned first,unsigned end){if(p->count==255||first>end||end>354)return 0;p->lines[p->count++]=(nflstreet_ui_text_line){first,end};return 1;}
int nflstreet_ui_text_prepare_lines(uint8_t object[40],const nflstreet_font*f,const char*text,uint32_t line_base,nflstreet_ui_text_lines*out){
 if(!object||!f||!f->bytes||!text||!out||object[15]>1)return 0;unsigned flags=sh(object);if(!(flags&0x100)||!(flags&0x3000))return 0;
 unsigned length=0;while(length<=354&&text[length])++length;if(length>354)return 0;
 nflstreet_ui_text_lines p={0};unsigned first=0;int limit=(int16_t)sh(object+20);
 for(unsigned end=0;end<=length;++end){if(text[end]&&text[end]!='\n')continue;unsigned last_space=first;int width=0;
  for(unsigned i=first;i<end;++i){int step=advance(f,(uint8_t)text[i]);if(text[i]==' '||text[last_space]==' ')last_space=i;
   if(limit<width+step){unsigned cut;
    if((flags&0x1000)&&last_space!=first){cut=last_space;i=cut;}
    else{if(i==first)++i;cut=i;}
    if(!append(&p,first,cut))return 0;first=cut;width=0;last_space=first;
   }width+=step;
  }
  if(!append(&p,first,end))return 0;first=end+1;
 }
 if(p.count>1&&(!line_base||line_base>UINT32_MAX-p.count*4))return 0;
 uint8_t b[40];memcpy(b,object,40);b[15]=(uint8_t)p.count;
 if(p.count>1)pw(b+36,line_base);else{ps(b+36,p.lines[0].begin);ps(b+38,p.lines[0].end);}
 if(!(flags&0x200))ps(b+22,p.count*f->line_height);ps(b,flags&0x7fff);
 memcpy(object,b,40);*out=p;return 1;
}
