/* Controlled ball association oracle, not possession gameplay. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../ball/ball_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i)m->dispatch(c,c->pc);return !c->pc&&!c->exception;}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 unsigned addresses[2]={0x803c04ec,0x803c19c0},headers[2]={be(ram+0x3c04ec),be(ram+0x3c19c0)},root=be(ram+0x3a2670);if(root<0x80000000||root>0x817fff00)return 4;unsigned at=root-0x80000000;size_t bytes=8+(size_t)ram[at]*244;if(bytes>24*1024*1024-at)return 4;
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 unsigned checks=0;for(unsigned test=0;test<8;++test){if(test){root=0x817f0000;at=0x17f0000;bytes=8+3*244;memset(ram+at,0,bytes);ram[at]=(uint8_t)(test==7?0:3);put(ram+0x3a2670,root);
  unsigned links[3]={test==1?headers[0]:0,test==2?headers[1]:0,test==3?headers[0]:0};
  if(test==4){links[0]=headers[1];links[1]=headers[0];links[2]=headers[1];}
  if(test==5){links[0]=headers[1]|0xa5000000;links[1]=headers[0]|0x80000000;}
  if(test==6){links[0]=(headers[0]&~255u)|2;links[1]=(headers[1]&~255u)|2;}
  for(unsigned i=0;i<3;++i)put(ram+at+8+i*244+0xb4,links[i]);}
  for(unsigned i=0;i<2;++i){unsigned found=99,index=99;if(!nflstreet_ball_find_actor(ram+at,bytes,headers[i],&found,&index))return 5;
   c.gpr[3]=addresses[i];if(!call(m,&c,0x800be4f8)||c.gpr[3]!=(found?root+8+index*244:0)){fprintf(stderr,"Lookup mismatch %u/%u\n",test,i);return 6;}
   if(found){c.gpr[3]=root+8+index*244;if(!call(m,&c,0x800be380)||c.gpr[3]!=addresses[i])return 7;}++checks;
  }
 }
 unsigned found=91,index=92;if(nflstreet_ball_find_actor(ram+at,7,headers[0],&found,&index)||found!=91||index!=92)return 8;
 if(nflstreet_ball_find_actor(ram+at,bytes,0,&found,&index)||found!=91||index!=92)return 8;
 printf("%u original actor-associated ball lookups exact: actual links, detached/empty, first duplicate, reversed order, ignored high bits and wrong tags; malformed rollback pass\n",checks);
 free(ram);FreeLibrary(h);return 0;}
