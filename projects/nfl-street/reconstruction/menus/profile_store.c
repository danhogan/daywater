#include "profile_store.h"
#include <string.h>
const uint32_t nflstreet_option_keys[22]={0x454c534f,0x414c504f,0x544c504f,0x4152544f,0x5658464f,0x5647494f,0x5645464f,0x5349444f,0x4f52504f,0x5550434f,0x4249564f,0x4853434f,0x4554534f,0x4153414f,0x41434d4f,0x444d474f,0x5455544f,0x5249414f,0x4d4e534f,0x4d5a434f,0x4542474f,0x5041504f};
void nflstreet_option_defaults(uint32_t*v){static const uint32_t defaults[22]={0,36,200000,10,10,10,10,0,0,1,1,1,1,1,0,0,1,1,1,0,1,0};if(v)memcpy(v,defaults,sizeof defaults);}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t crc(const uint8_t*b,size_t n){uint32_t c=0xffffffff;for(size_t i=0;i<n;++i){c^=b[i];for(unsigned j=0;j<8;++j)c=(c>>1)^(0xedb88320u& (0u-(c&1)));}return ~c;}
int nflstreet_option_encode(const uint32_t*v,uint8_t*b){if(!v||!b)return 0;memcpy(b,"DWNSOPT1",8);put(b+8,1);put(b+12,22);for(unsigned i=0;i<22;++i){put(b+16+i*8,nflstreet_option_keys[i]);put(b+20+i*8,v[i]);}put(b+192,crc(b,192));return 1;}
int nflstreet_option_decode(const uint8_t*b,size_t n,uint32_t*v){if(!b||!v||n!=196||memcmp(b,"DWNSOPT1",8)||be(b+8)!=1||be(b+12)!=22||be(b+192)!=crc(b,192))return 0;uint32_t values[22];for(unsigned i=0;i<22;++i){if(be(b+16+i*8)!=nflstreet_option_keys[i])return 0;values[i]=be(b+20+i*8);}memcpy(v,values,sizeof values);return 1;}
