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
#define OUT 0x80f37f00u
static uint32_t be32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t bits(float f){uint32_t w;memcpy(&w,&f,4);return w;}
static float real(uint32_t w){float f;memcpy(&f,&w,4);return f;}
static uint32_t rnd(uint32_t *s){*s^=*s<<13;*s^=*s>>17;*s^=*s<<5;return *s;}
static int invoke(const ModernGekkoModuleDesc*m,uint8_t*ram,int32_t x,int32_t y,int32_t z){const uint32_t ret=0x81234560u;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=RAM_BYTES;c.gpr[1]=STACK;c.gpr[3]=OUT;c.gpr[4]=x;c.gpr[5]=y;c.gpr[6]=z;c.lr=ret;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.pc=0x801a36ecu;c.downcount=-1000000000;for(unsigned step=0;c.pc!=ret&&step<1000;++step){if(m->dispatch(&c,c.pc)!=1||c.exception){fprintf(stderr,"exception=%08x pc=%08x lr=%08x\\n",c.exception,c.pc,c.lr);return 0;}}if(c.pc!=ret){fprintf(stderr,"did not return pc=%08x lr=%08x\\n",c.pc,c.lr);return 0;}return 1;}
int main(int argc,char**argv){if(argc!=3)return 2;FILE*f=fopen(argv[1],"rb");uint8_t*ram=malloc(RAM_BYTES);if(!f||!ram||fread(ram,1,RAM_BYTES,f)!=RAM_BYTES)return 2;fclose(f);void*l=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);ModernGekkoGetModuleFn get=l?(ModernGekkoGetModuleFn)dlsym(l,MODERNGEKKO_GET_MODULE_SYMBOL):0;const ModernGekkoModuleDesc*m=get?get():0;if(!m)return 2;float p[8];for(unsigned i=0;i<8;++i)p[i]=real(be32(ram+COEFFICIENTS-RAM_BASE+4*i));uint32_t seed=0x801a36ecu;for(unsigned i=0;i<100000;++i){int32_t a[3];for(unsigned n=0;n<3;++n)a[n]=(int32_t)(rnd(&seed)&0xffffffu);float native[4];nflstreet_801a36ec_quaternion(a[0],a[1],a[2],p,native);for(unsigned n=0;n<4;++n)memset(ram+OUT-RAM_BASE+4*n,0xde,4);if(!invoke(m,ram,a[0],a[1],a[2]))return 3;for(unsigned n=0;n<4;++n)if(be32(ram+OUT-RAM_BASE+4*n)!=bits(native[n])){fprintf(stderr,"case=%u angles=%06x,%06x,%06x lane=%u original=%08x native=%08x\n",i,(uint32_t)a[0],(uint32_t)a[1],(uint32_t)a[2],n,be32(ram+OUT-RAM_BASE+4*n),bits(native[n]));return 1;}}printf("801A36EC: 100000 exact Euler-to-quaternion results.\n");return 0;}
