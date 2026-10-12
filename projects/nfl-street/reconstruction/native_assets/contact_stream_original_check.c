/* Original-module check of the selected common-bank stream; no gameplay
   meaning is assigned to clip 575. The module stays outside the viewer. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../actor_target_input/command_timing.h"
#include "../actor_target_input/event_signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){m->dispatch(c,c->pc);if(c->exception)break;}return !c->pc&&!c->exception;}
int main(int argc,char**argv){if(argc!=4&&argc!=5)return 2;unsigned clip=argc==5?(unsigned)strtoul(argv[4],NULL,10):575;if(clip!=18&&clip!=575&&clip!=0x249&&clip!=0x24a)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;
 unsigned char*ram=malloc(24*1024*1024),*bank=malloc(16*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!bank||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 5;fclose(f);f=fopen(argv[3],"rb");if(!f)return 5;size_t n=fread(bank,1,16*1024*1024,f);fclose(f);if(n<16)return 5;
 unsigned table=be(bank+12);if(table>n-4||be(bank+table)<=clip||table+4+(clip+1)*4>n)return 5;unsigned at=be(bank+table+4+clip*4);if(at>n-38)return 5;unsigned next=be(bank+table+8+clip*4);if(next<=at||next>n)return 5;size_t size=next-at;memcpy(ram+0x17f0000,bank+at,size);memset(ram+0x17f1000,0,64);
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.gpr[3]=0x817f1000;c.gpr[4]=0x817f0000;c.gpr[5]=0;
 /* Controlled accepted actor callback route; all original code executes. */
 /* Direct accepted callback, with cleared/set bits and unrelated flags.
    Compare all actor bytes, not just the flag word. */
 const unsigned patterns[]={0,0xffffffff,0x10004600,0xabcdef01};
 const unsigned direct_commands[]={0xc004,0xc006,0xc00b,0xc00c};
 for(unsigned cmd=0;cmd<4;++cmd)for(unsigned j=0;j<4;++j){unsigned char expected[0x14d4];memcpy(expected,ram+0x3c19c0,sizeof expected);unsigned result=patterns[j];
  for(unsigned k=0;k<4;++k)ram[0x3c19cc+k]=(unsigned char)(result>>(24-k*8));
  c.gpr[3]=0x803c19c0;c.gpr[4]=direct_commands[cmd];c.gpr[5]=0xffff;
  if(!call(m,&c,0x8001532c)||c.gpr[3]!=1||!nflstreet_event_flag_command(direct_commands[cmd],&result))return 10;
  for(unsigned k=0;k<4;++k)expected[12+k]=(unsigned char)(result>>(24-k*8));
  if(memcmp(expected,ram+0x3c19c0,sizeof expected))return 10;
 }
 /* Clear the bit so an actual stream callback must demonstrably set it. */
 unsigned seeded_flags=be(ram+0x3c19cc)&~0x40000u;
 for(unsigned k=0;k<4;++k)ram[0x3c19cc+k]=(unsigned char)(seeded_flags>>(24-k*8));
 c.gpr[3]=0x817f1000;c.gpr[4]=0x817f0000;c.gpr[5]=0;
 unsigned char actor[0x14d4];memcpy(actor,ram+0x3c19c0,sizeof actor);
 unsigned initial_flags=be(actor+12);
 ram[0x3a42a4]=0; /* original callback stack index */
 const unsigned contexts[]={0x803c19c0,0xffff};const size_t offsets[]={0x32bf10,0x32bf1c};
 for(unsigned j=0;j<2;++j)for(unsigned k=0;k<4;++k)ram[offsets[j]+k]=(unsigned char)(contexts[j]>>(24-k*8));
 if(be(ram+0x3a330c)!=0x8001532c)return 6;
 unsigned char state[64]={0};unsigned flags=initial_flags,complete;unsigned short emitted[64];size_t count;
 unsigned callbacks=0,terminations=0,steps=0;
 if(!call(m,&c,0x8017724c)||!nflstreet_stream_setup(state,0x817f0000,bank+at,size,&flags,emitted,&count,&complete)||memcmp(state,ram+0x17f1000,64))return 6;
 callbacks+=(unsigned)count;
 for(unsigned k=0;k<4;++k)actor[12+k]=(unsigned char)(flags>>(24-k*8));
 if(memcmp(actor,ram+0x3c19c0,sizeof actor))return 8;
 const double deltas[]={0,1,1,0.5,2.5,17,0.25,31,2,64};
 for(unsigned i=0;i<200;++i){double delta=deltas[i%10];c.gpr[3]=0x817f1000;c.fpr[1]=delta;
  if(!call(m,&c,0x80177374)||!nflstreet_command_timing(state,bank+at,size,delta,&flags,emitted,&count,&complete)||memcmp(state,ram+0x17f1000,64)||complete!=c.gpr[3]||flags!=be(ram+0x3c19cc)){printf("Stream mismatch step=%u delta=%.9g\n",i,delta);return 7;}
  ++steps;callbacks+=(unsigned)count;for(unsigned k=0;k<4;++k)actor[12+k]=(unsigned char)(flags>>(24-k*8));
  if(memcmp(actor,ram+0x3c19c0,sizeof actor))return 8;
  for(size_t j=0;j<count;++j)if(emitted[j]!=0xc00c&&emitted[j]!=0xc00b&&emitted[j]!=0xc006&&emitted[j]!=0xc004)return 9;
  if(complete){++terminations;break;}
 }
 if(clip==18&&!callbacks)return 9;if((clip==0x249||clip==0x24a)&&(!terminations||!callbacks))return 9;
 printf("Clip %u: %u accepted supported callbacks, full actor blocks exact\n",clip,callbacks);
 printf("Original setup and %u complete 64-byte timing states exact; %u original terminations; direct callback full-actor controls exact\n",steps,terminations);free(bank);free(ram);FreeLibrary(h);return 0;}
