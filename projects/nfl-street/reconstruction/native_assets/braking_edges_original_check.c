/* Original-module boundary comparisons, never linked into native runtime. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../kinematics/kinematics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(unsigned char *p,unsigned w){p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
int main(int argc,char **argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc *m=get();if(m->abi_version!=3||m->cpu_abi_version!=4||m->cpu_state_size!=sizeof(CPUState))return 4;
 unsigned char *base=malloc(24*1024*1024),*ram=malloc(24*1024*1024);if(!base||!ram)return 5;FILE *f=fopen(argv[2],"rb");if(!f||fread(base,1,24*1024*1024,f)!=24*1024*1024||fgetc(f)!=EOF)return 5;fclose(f);
 const unsigned block=0x3c1b68;const char *names[]={"decrement-clamped-to-speed","already-stopped","zero-maximum-positive-force","quarter-turn-heading","speed-above-maximum","below-small-motion-threshold"};
 for(unsigned test=0;test<6;++test){memcpy(ram,base,24*1024*1024);uint32_t state[21];for(unsigned j=0;j<21;++j)state[j]=be(ram+block+j*4);
 if(test==0)state[7]=0x3b000000;
 if(test==1)state[7]=0;
 if(test==2){state[9]=0;state[13]=0x3c000000;}
 if(test==3)state[8]=0x400000;
 if(test==4)state[7]=0x3e800000;
 if(test==5)state[7]=0x30000000;
 for(unsigned j=0;j<21;++j)put(ram+block+j*4,state[j]);
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.gpr[3]=0x80000000+block;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.pc=0x80036b28;c.downcount=c.cycle_budget=100000000;
 for(unsigned step=0;c.pc&&step<1000000;++step){m->dispatch(&c,c.pc);if(c.exception)break;}if(c.pc||c.exception)return 6;
 if(!nflstreet_step_kinematics(state))return 7;
 for(unsigned j=0;j<21;++j)if(state[j]!=be(ram+block+j*4)){printf("%s offset=%x native=%08x original=%08x\n",names[test],j*4,state[j],be(ram+block+j*4));return 8;}
 printf("Original boundary match: %s, all 21 words exact\n",names[test]);
 }
 free(base);free(ram);FreeLibrary(h);return 0;}
