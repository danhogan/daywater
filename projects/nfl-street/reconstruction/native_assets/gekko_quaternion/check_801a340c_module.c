#include "quaternion_kernels.h"
#include "moderngekko/module_abi.h"
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u * 1024u * 1024u)
#define STACK 0x80f47000u
#define COEFFICIENTS 0x8039e100u
#define QUATERNION 0x80f37f00u
#define OUT 0x80f37f20u
static uint32_t be32(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t x){p[0]=(uint8_t)(x>>24);p[1]=(uint8_t)(x>>16);p[2]=(uint8_t)(x>>8);p[3]=(uint8_t)x;}
static float real(uint32_t x){float f;memcpy(&f,&x,4);return f;}
static uint32_t bits(float f){uint32_t x;memcpy(&x,&f,4);return x;}
static uint32_t rnd(uint32_t*s){*s^=*s<<13;*s^=*s>>17;*s^=*s<<5;return*s;}
static int invoke(const ModernGekkoModuleDesc*m,uint8_t*ram){const uint32_t ret=0x81234560u;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=RAM_BYTES;c.gpr[1]=STACK;c.gpr[3]=OUT;c.gpr[4]=QUATERNION;c.lr=ret;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.pc=0x801a340cu;c.downcount=-1000000000;for(unsigned i=0;c.pc!=ret&&i<10000;++i)if(m->dispatch(&c,c.pc)!=1||c.exception)return 0;return c.pc==ret;}
int main(int argc,char**argv){if(argc!=3)return 2;FILE*f=fopen(argv[1],"rb");uint8_t*ram=malloc(RAM_BYTES);if(!f||!ram||fread(ram,1,RAM_BYTES,f)!=RAM_BYTES)return 2;fclose(f);void*l=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);ModernGekkoGetModuleFn get=l?(ModernGekkoGetModuleFn)dlsym(l,MODERNGEKKO_GET_MODULE_SYMBOL):0;const ModernGekkoModuleDesc*m=get?get():0;if(!m)return 2;float p[8];for(unsigned n=0;n<8;++n)p[n]=real(be32(ram+COEFFICIENTS-RAM_BASE+4*n));uint32_t seed=0x801a340cu;for(unsigned i=0;i<500000;++i){int32_t a[3];for(unsigned n=0;n<3;++n)a[n]=(int32_t)(rnd(&seed)&0xffffffu);float q[4];nflstreet_801a36ec_quaternion(a[0],a[1],a[2],p,q);for(unsigned n=0;n<4;++n)put(ram+QUATERNION-RAM_BASE+4*n,bits(q[n]));int32_t native[3];nflstreet_801a340c_euler(q,native);memset(ram+OUT-RAM_BASE,0xde,12);if(!invoke(m,ram)){fprintf(stderr,"invoke %u\n",i);return 3;}for(unsigned n=0;n<3;++n)if(be32(ram+OUT-RAM_BASE+4*n)!=(uint32_t)native[n]){fprintf(stderr,"case=%u q=%08x,%08x,%08x,%08x lane=%u original=%08x native=%08x\n",i,bits(q[0]),bits(q[1]),bits(q[2]),bits(q[3]),n,be32(ram+OUT-RAM_BASE+4*n),(uint32_t)native[n]);return 1;}}puts("801A340C: 500000 exact quaternion-to-Euler results.");return 0;}
