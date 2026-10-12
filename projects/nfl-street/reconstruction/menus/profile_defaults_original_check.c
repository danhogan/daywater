/* Original8014DFF0 initializes22 records. Database read boundary is captured. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "profile_store.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);memset(ram+0x30b2ac,0x5a,22*16+12);CPUState c={0};c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.pc=0x8014dff0;unsigned calls=0;for(unsigned i=0;c.pc&&i<100000;++i){if(c.pc==0x801b1ac8){++calls;c.gpr[3]=0;c.pc=c.lr;continue;}c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 5;}if(c.pc||calls!=1)return 5;uint32_t defaults[22];nflstreet_option_defaults(defaults);for(unsigned i=0;i<22;++i){uint8_t*p=ram+0x30b2ac+i*16;if(be(p)!=defaults[i]||be(p+4)!=0x5354504f||be(p+8)!=nflstreet_option_keys[i]||be(p+12)!=0x5a5a5a5a)return 6;}if(be(ram+0x30b40c)||be(ram+0x30b410)!=0xffffffff||be(ram+0x30b414)!=0xffffffff)return 7;puts("Original8014DFF0:22 Options keys/defaults/record tags and3 terminator words exact;database load captured;native envelope not original card format");free(ram);FreeLibrary(h);return 0;}
