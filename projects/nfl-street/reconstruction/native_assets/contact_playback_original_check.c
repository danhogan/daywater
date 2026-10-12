/* Controlled full selected-stream actor comparison; not complete gameplay. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "live_contact.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint8_t*read(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size<=0){fclose(f);return NULL;}uint8_t*b=malloc(size);if(!b||fread(b,1,size,f)!=(size_t)size){free(b);b=NULL;}fclose(f);*n=(size_t)size;return b;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i)m->dispatch(c,c->pc);return !c->pc&&!c->exception;}
int main(int argc,char**argv){if(argc!=6)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t rn,bn,sn;uint8_t*ram=read(argv[2],&rn),*bank=read(argv[5],&bn),*selection=read(argv[3],&sn);if(!ram||rn!=24*1024*1024||!bank||!selection||sn<8)return 4;
 nflstreet_live_contact native;if(!nflstreet_live_contact_open(&native,argv[3],argv[4],bank,bn))return 5;
 unsigned actors[2]={be(selection),be(selection+4)};for(unsigned i=0;i<2;++i){if(actors[i]<0x80000000||actors[i]-0x80000000+0x14d4>rn)return 6;memcpy(ram+actors[i]-0x80000000,native.actor[i],0x14d4);
  unsigned program=be(native.stream[i]);if(program<0x80000000||program-0x80000000+native.program_size[i]>rn||memcmp(ram+program-0x80000000,native.program[i],native.program_size[i]))return 7;}
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=rn;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 unsigned comparisons=0,updates=0;while(native.active&&updates<200){unsigned prior[2]={native.done[0],native.done[1]};int16_t frames[2][384];unsigned count;
  if(!nflstreet_live_contact_step(&native,1,frames,&count))return 8;
  for(unsigned i=0;i<2;++i){ram[0x3a42a4]=0;put(ram+0x32bf10,actors[i]);put(ram+0x32bf1c,0xffff);
   if(!prior[i]){c.gpr[3]=actors[i]+0x4e4+native.slots[i]*124+0x4c;c.fpr[1]=1;if(!call(m,&c,0x80177374)||c.gpr[3]!=native.done[i])return 9;}
   if(memcmp(ram+actors[i]-0x80000000,native.actor[i],0x14d4)){fprintf(stderr,"Actor mismatch step=%u actor=%u\n",updates,i);return 10;}++comparisons;
  }++updates;
 }
 if(native.active||native.callbacks!=5||native.completions!=1)return 11;
 printf("%u joined updates, %u complete original actor comparisons exact; both finite ends and all five accepted callbacks match (controlled callback eligibility)\n",updates,comparisons);
 free(ram);free(bank);free(selection);FreeLibrary(h);return 0;}
