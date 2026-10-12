#include "image_construct.h"
#include <string.h>
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void put(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
int nflstreet_ui_image_construct(uint8_t*b,size_t n,uint32_t base,uint32_t target,unsigned*requested,uint32_t*identity){
 if(!b||!requested||!identity||!base||n>UINT32_MAX-base||target<base||target-base>n||36>n-(target-base))return 0;
 uint8_t object[36];memcpy(object,b+target-base,36);unsigned flags=sh(object),entry=sh(object+4),archive=sh(object+6);if(flags&0x8000)return 0;
 unsigned acquire=!!(flags&2)&&entry!=65535&&archive!=65535;
 put(object+12,entry);put(object+14,archive);
 if((flags&3)==2){flags|=1;for(unsigned i=0;i<4;++i)memset(object+20+i*4,255,3);}
 put(object,flags|0x4000);memcpy(b+target-base,object,36);*requested=acquire;*identity=acquire?(archive<<16|entry):0;return 1;
}
