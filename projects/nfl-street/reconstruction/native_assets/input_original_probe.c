/* Research-only: run an original input translation call on private full MEM1.
   Never linked into the native viewer. No DLL/source patching or stubbed calls.
   Scratch stack/vector addresses belong to this harness, not game fields. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void dump(const unsigned char *p,unsigned n){for(unsigned i=0;i<n;++i)printf("%02x",p[i]);}
int main(int argc,char **argv){if(argc!=4)return 2;char *end;unsigned actor=(unsigned)strtoul(argv[3],&end,16);if(*end||actor<0x80000000||actor>0x817ff000)return 2;
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc *m=get();if(m->cpu_state_size!=sizeof(CPUState)||m->abi_version!=3)return 4;
 unsigned char *saved=malloc(24*1024*1024),*ram=malloc(24*1024*1024);if(!saved||!ram)return 5;FILE *f=fopen(argv[2],"rb");if(!f)return 5;if(fread(saved,1,24*1024*1024,f)!=24*1024*1024||fgetc(f)!=EOF)return 5;fclose(f);
 unsigned off=actor&0x1ffffff;printf("Original actor header=%08x name=",be(saved+off));dump(saved+off+0xb80,16);putchar('\n');
 const char *names[]={"neutral","right","forward","diagonal"};
 for(unsigned test=0;test<4;++test){memcpy(ram,saved,24*1024*1024);memset(ram+0x17f0000,0,32);
 if(test==1||test==3){ram[0x17f0004]=0x3f;ram[0x17f0005]=0x80;}if(test==2||test==3){ram[0x17f0008]=0x3f;ram[0x17f0009]=0x80;}
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.gpr[3]=actor;c.gpr[4]=0x817f0000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.pc=0x8003ddec;c.lr=0;c.downcount=c.cycle_budget=100000000;
 printf("%s input-before=",names[test]);dump(ram+off+0x200,48);putchar('\n');
 unsigned watched_return=0,watched_pc=0,steps=0;for(;c.pc&&steps<1000000;++steps){if(watched_return&&c.pc==watched_return){printf("branch-return pc=%08x r3=%08x\n",watched_pc,c.gpr[3]);watched_return=0;}
 if(c.pc==0x8006510c||c.pc==0x80065290||c.pc==0x801784b8){watched_pc=c.pc;watched_return=c.lr;printf("branch-entry pc=%08x r3=%08x\n",c.pc,c.gpr[3]);}
 m->dispatch(&c,c.pc);if(c.exception)break;}
 printf("%s pc=%08x exception=%08x steps=%u input-after=",names[test],c.pc,c.exception,steps);dump(ram+off+0x200,48);printf(" name-preserved=%d\n",!memcmp(saved+off+0xb80,ram+off+0xb80,16));
 if(c.pc||c.exception)return 6;
 }
 free(saved);free(ram);FreeLibrary(h);return 0;}
