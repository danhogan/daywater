#include "quaternion_kernels.h"
#include "moderngekko/module_abi.h"
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define A 0x80f37f00u
#define B 0x80f37f10u
#define OUT 0x80f37f20u
static uint32_t be(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t w){p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
static uint32_t bits(float f){uint32_t w;memcpy(&w,&f,4);return w;}
static uint32_t rnd(uint32_t*s){*s^=*s<<13;*s^=*s>>17;*s^=*s<<5;return*s;}
static int call(const ModernGekkoModuleDesc*m,uint8_t*ram,float t){const uint32_t ret=0x81234560;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=BYTES;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=OUT;c.gpr[4]=A;c.gpr[5]=B;c.gpr[6]=B;c.fpr[1]=t;c.ps1[1]=t;c.lr=ret;c.pc=0x801a383c;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.downcount=-1000000000;for(unsigned i=0;c.pc!=ret&&i<10000;++i)if(m->dispatch(&c,c.pc)!=1||c.exception)return 0;return c.pc==ret;}
static uint32_t unary(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t pc,float x){const uint32_t ret=0x81234560;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=BYTES;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.fpr[1]=x;c.ps1[1]=x;c.lr=ret;c.pc=pc;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.downcount=-1000000000;for(unsigned i=0;c.pc!=ret&&i<10000;++i)if(m->dispatch(&c,c.pc)!=1||c.exception)return 0xffffffffu;return bits((float)c.fpr[1]);}
int main(int argc,char**argv){if(argc!=3)return 2;FILE*f=fopen(argv[1],"rb");uint8_t*ram=malloc(BYTES);if(!f||!ram||fread(ram,1,BYTES,f)!=BYTES)return 2;fclose(f);void*l=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);ModernGekkoGetModuleFn get=l?(ModernGekkoGetModuleFn)dlsym(l,MODERNGEKKO_GET_MODULE_SYMBOL):0;const ModernGekkoModuleDesc*m=get?get():0;if(!m)return 2;uint32_t s=0x801a383c;for(unsigned n=0;n<10000;++n){float a[4],b[4],out[4],t=(float)(rnd(&s)&0xffff)/65535.0f;for(unsigned i=0;i<4;++i){a[i]=(float)(int16_t)rnd(&s)*0x1p-14f;b[i]=(float)(int16_t)rnd(&s)*0x1p-14f;put(ram+A-BASE+4*i,bits(a[i]));put(ram+B-BASE+4*i,bits(b[i]));}nflstreet_801a383c_slerp(t,a,b,out);if(!call(m,ram,t))return 3;for(unsigned i=0;i<4;++i)if(be(ram+OUT-BASE+4*i)!=bits(out[i])){const float dot=fmaf(a[3],b[3],fmaf(a[2],b[2],fmaf(a[0],b[0],a[1]*b[1])));const float al=fmaf(a[3],a[3],fmaf(a[2],a[2],fmaf(a[0],a[0],a[1]*a[1])));const float bl=fmaf(b[3],b[3],fmaf(b[2],b[2],fmaf(b[0],b[0],b[1]*b[1])));const float ia=1.0f/sqrtf(al),ib=1.0f/sqrtf(bl),co=dot*(ia*ib);fprintf(stderr,"case=%u t=%08x a=%08x,%08x,%08x,%08x b=%08x,%08x,%08x,%08x lane=%u original=%08x native=%08x cosine=%08x invlen a original/native=%08x/%08x b=%08x/%08x acos original/native=%08x/%08x sin original/native=%08x/%08x\\n",n,bits(t),bits(a[0]),bits(a[1]),bits(a[2]),bits(a[3]),bits(b[0]),bits(b[1]),bits(b[2]),bits(b[3]),i,be(ram+OUT-BASE+4*i),bits(out[i]),bits(co),unary(m,ram,0x80188eacu,al),bits(ia),unary(m,ram,0x80188eacu,bl),bits(ib),unary(m,ram,0x80188d08u,co),bits(acosf(co)),unary(m,ram,0x80188accu,acosf(co)),bits(sinf(acosf(co))));return 1;}}puts("801A383C host candidate exact.");return 0;}
