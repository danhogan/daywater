#include "text_construct.h"
#include "text.h"
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int nflstreet_ui_text_construct(uint8_t*b,size_t n,uint32_t base,uint32_t target,const nflstreet_font*font){
 if(!b||!font||!base||n>UINT32_MAX-base||target<base||target-base>n||40>n-(target-base))return 0;
 unsigned offset=target-base,relative=be(b+offset+32);if(relative>n-offset||12>n-offset-relative)return 0;unsigned descriptor=offset+relative,text=be(b+descriptor+8);
 if(text<base||text-base>=n||!memchr(b+text-base,0,n-(text-base)))return 0;
 uint8_t object[40];memcpy(object,b+offset,40);put(object+32,base+descriptor);
 if(!nflstreet_ui_text_prepare(object,font,(const char*)b+text-base))return 0;memcpy(b+offset,object,40);return 1;
}
