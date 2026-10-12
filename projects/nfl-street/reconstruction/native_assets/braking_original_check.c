/* Original-module oracle on naturally zero-strength/zero-acceleration saved
   actors. Full original calls, no stubs or DLL/source patching. Not linked
   into the independent native viewer. Outputs a private NBS1 fixture. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../kinematics/kinematics.h"
#include "../command_to_movement/actor_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(unsigned char *p,unsigned w){p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
int main(int argc,char **argv){if(argc!=5&&argc!=6)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc *m=get();if(m->cpu_state_size!=sizeof(CPUState)||m->abi_version!=3||m->cpu_abi_version!=4)return 4;
 char *end;unsigned actor=(unsigned)strtoul(argv[3],&end,16);if(*end||actor<0x80000000||actor>0x817ff000)return 2;unsigned a=actor&0x1ffffff;
 unsigned char *ram=malloc(24*1024*1024);if(!ram)return 5;FILE *f=fopen(argv[2],"rb");if(!f)return 5;
 if(fread(ram,1,24*1024*1024,f)!=24*1024*1024||fgetc(f)!=EOF)return 5;fclose(f);
 if(memcmp(ram+a+0xb80,"Thomas",7))return 6;
 if(argc==6){unsigned char seed[152];FILE *sf=fopen(argv[5],"rb");if(!sf||fread(seed,1,152,sf)!=152||memcmp(seed,"NMS1",4))return 6;fclose(sf);
 if(be(seed+8)!=be(ram+a))return 6;
 memcpy(ram+a+0x200,seed+20,48);memcpy(ram+a+0x1a8,seed+68,84);
 /* Controlled selection of the original zero-status dispatch, leaving
    positive active strength and all other original context intact. */
 ram[a+0x21e]=0;
 printf("Controlled release from captured moving state: original dispatch status zero; active strength retained\n");
 }else if(be(ram+a+0x200)||be(ram+a+0x210)||be(ram+a+0x1dc))return 6;
 if(argc==5)printf("Identified Thomas: requested/active strengths and acceleration are zero in unmodified snapshot\n");
 uint32_t state[21],actor_input[12];for(unsigned i=0;i<21;++i)state[i]=be(ram+a+0x1a8+i*4);for(unsigned i=0;i<12;++i)actor_input[i]=be(ram+a+0x200+i*4);
 nflstreet_actor_runtime runtime;nflstreet_actor_runtime_init(&runtime,be(ram+a),(int16_t)((unsigned)ram[a+0xbb4]<<8|ram[a+0xbb5]),(int16_t)((unsigned)ram[a+0xbbc]<<8|ram[a+0xbbd]),be(ram+a+0x408),actor_input,state);
 f=fopen(argv[4],"wb");if(!f)return 5;unsigned char header[152];memcpy(header,"NBS1",4);put(header+4,48);memcpy(header+8,ram+a,4);memcpy(header+12,ram+a+0xbb4,2);memcpy(header+14,ram+a+0xbbc,2);memcpy(header+16,ram+a+0x408,4);memcpy(header+20,ram+a+0x200,48);memcpy(header+68,ram+a+0x1a8,84);if(fwrite(header,1,152,f)!=152)return 5;
 unsigned first_zero=0;for(unsigned tick=0;tick<48;++tick){CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.gpr[3]=actor;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.pc=0x800f86bc;c.lr=0;c.downcount=c.cycle_budget=100000000;
 unsigned dispatches=0;for(;c.pc&&dispatches<1000000;++dispatches){m->dispatch(&c,c.pc);if(c.exception)break;}if(c.pc||c.exception){printf("Original failure pc=%08x exception=%08x\n",c.pc,c.exception);return 7;}
 if(!nflstreet_actor_runtime_brake(&runtime))return 8;memcpy(state,runtime.state,sizeof state);
 for(unsigned i=0;i<12;++i)if(runtime.actor[i]!=be(ram+a+0x200+i*4)){printf("Actor mismatch update=%u offset=%x native=%08x original=%08x\n",tick,i*4,runtime.actor[i],be(ram+a+0x200+i*4));return 9;}
 for(unsigned i=0;i<21;++i)if(state[i]!=be(ram+a+0x1a8+i*4)){printf("Mismatch update=%u offset=%x native=%08x original=%08x\n",tick,i*4,state[i],be(ram+a+0x1a8+i*4));return 9;}
 if(!first_zero&&state[7]==0)first_zero=tick+1;
 if(fwrite(ram+a+0x200,1,48,f)!=48||fwrite(ram+a+0x1a8,1,84,f)!=84)return 5;
 }
 fclose(f);if(!first_zero||state[10]||state[11]||state[7])return 10;
 printf("Verified 48 complete original caller actor and braking states, first zero-speed update=%u, held zero through update 48\n",first_zero);
 free(ram);FreeLibrary(h);return 0;}
