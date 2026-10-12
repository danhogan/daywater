#include "model_view.h"
#include <string.h>
static uint32_t word(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned half(const uint8_t*p){return(unsigned)p[0]<<8|p[1];}
static void write(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static int span(size_t total,size_t off,size_t n){return off<=total&&n<=total-off;}
static int valid(const nflstreet_model_view_source*s,unsigned*count,size_t*directory){
 if(!s||!s->bytes||s->size<16||!s->token||memcmp(s->bytes,"TMdl",4)||s->bytes[15]>1)return 0;
 size_t declared=word(s->bytes+4);if(declared<16||declared>s->size)return 0;
 *count=half(s->bytes+8);*directory=half(s->bytes+10);
 if(*count&&!*directory)return 0;if(!span(declared,*directory,(size_t)*count*16))return 0;
 for(unsigned i=0;i<*count;++i){const uint8_t*r=s->bytes+*directory+i*16;size_t n=word(r+8),encoded=word(r+4);
  if(r[15]!=s->bytes[15])return 0;
  if(n&&encoded){if(s->bytes[15]){if(encoded<s->token)return 0;encoded-=s->token;}
   if(!span(declared,encoded,n))return 0;}}
 return 1;
}
int nflstreet_model_view_find(const nflstreet_model_view_source*s,uint32_t tag,nflstreet_model_view_chunk*out){
 unsigned count=0;size_t directory=0;if(!out||!valid(s,&count,&directory))return -1;
 for(unsigned i=0;i<count;++i){const uint8_t*r=s->bytes+directory+i*16;uint32_t n=word(r+8);
  if(word(r)==tag&&n){uint32_t encoded=word(r+4);if(!encoded)return 0;
   size_t offset=s->bytes[15]?encoded-s->token:encoded;
   nflstreet_model_view_chunk chunk={tag,encoded,s->token,s->bytes+offset,n,i};*out=chunk;return 1;}}
 return 0;
}
int nflstreet_model_view_apply(uint8_t*v,size_t size,const nflstreet_model_view_source*s,uint32_t tag,unsigned*found){
 if(!v||size<NFLSTREET_MODEL_VIEW_BYTES||!found)return 0;nflstreet_model_view_chunk chunk;
 int result=nflstreet_model_view_find(s,tag,&chunk);if(result<0)return 0;*found=(unsigned)result;if(!result)return 1;
 unsigned slot=13;switch(tag){case 0x496e666fu:slot=0;break;case 0x4578746eu:slot=1;break;case 0x47656f6du:slot=2;break;case 0x436c7374u:slot=3;break;
 case 0x536b656cu:slot=4;break;case 0x53776170u:slot=5;break;case 0x4d61746cu:slot=6;break;case 0x5478666du:slot=8;break;
 case 0x496e7374u:slot=9;break;case 0x52696e74u:slot=10;break;case 0x44496e66u:slot=11;break;case 0x46616465u:slot=12;break;default:break;}
 if(slot<13)write(v+4*slot,chunk.encoded_word);return 1;
}
int nflstreet_model_view_attach(uint8_t*v,size_t size,const nflstreet_model_view_source*s){
 unsigned count=0;size_t directory=0;if(!v||size<NFLSTREET_MODEL_VIEW_BYTES||!valid(s,&count,&directory))return 0;
 static const uint32_t tags[]={0x496e666fu,0x4578746eu,0x47656f6du,0x436c7374u,0x53776170u,0x536b656cu,0x4d61746cu,0x5478666du,0x496e7374u,0x52696e74u,0x46616465u,0x44496e66u};
 write(v+0x2c,0);write(v+0x30,0);for(unsigned i=0;i<12;++i){unsigned found;if(!nflstreet_model_view_apply(v,size,s,tags[i],&found))return 0;}return 1;
}
