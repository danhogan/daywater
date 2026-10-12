#include <windows.h>
#include "moderngekko/module_abi.h"
#include "dispatcher.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int execute(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_ui_dispatcher*s){
 c->pc=0x801d16a0;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned j=0;c->pc&&j<1000000;++j){
  if(c->pc==0x801d4b3c){if(s->count>=64||c->gpr[8]>16)return 0;unsigned kind=c->gpr[6],p=c->gpr[7]-0x80000000;if(kind!=0&&kind!=1&&kind!=3)return 0;
   nflstreet_ui_service_request*q=&s->requests[s->count++];memset(q,0,sizeof *q);q->kind=9;q->source=(c->gpr[3]&65535)<<16|(c->gpr[4]&65535);q->property=kind;q->target=be(c->ram+p);q->argc=c->gpr[8];if(kind==0)q->link=be(c->ram+p+4);if(kind==1)q->aux=be(c->ram+p+8);
   for(unsigned i=0;i<q->argc;++i)q->args[i]=be(c->ram+c->gpr[9]-0x80000000+i*4);c->pc=c->lr;
  }else m->dispatch(c,c->pc);if(c->exception)return 0;
 }return !c->pc;
}
int main(int argc,char**argv){if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);uint8_t source[4273];f=fopen(argv[3],"rb");if(!f||fread(source,1,sizeof source,f)!=sizeof source)return 4;fclose(f);unsigned relocated;if(!nflstreet_ui_relocate(source,sizeof source,0x81100000,&relocated))return 5;
 unsigned checked=0;uint32_t ids[]={2,2};
 for(unsigned identity=0;identity<2;++identity){uint8_t native[4273];memcpy(native,source,sizeof source);memcpy(ram+0x1100000,source,sizeof source);nflstreet_ui_dispatcher want={0},actual={0};put(ram+0x30b3fc,identity);want.query134=actual.query134=identity;for(unsigned j=0;j<3;++j)want.transition[j]=actual.transition[j]=be(ram+0x3a9a40-0x6f88+j*4);
  memset(ram+0x17f0000,0,0x200);put(ram+0x17f0008,16);put(ram+0x17f000c,0x8012a414);put(ram+0x17f0104,ids[identity]);put(ram+0x17f0110,0x81100000);
  for(unsigned pass=0;pass<2;++pass){nflstreet_ui_script vm={0};vm.sp=pass?5:4;vm.pc=0x81100000+(pass?0x4ca:0x1bc);vm.root=0x81100088;vm.current_screen=ids[identity];vm.flags=be(ram+0x17f0100);
   if(pass){vm.words[1]=0x96;vm.words[2]=UINT32_MAX;vm.words[3]=vm.root;}else{vm.words[1]=0xfffe;vm.words[2]=vm.root;}
   for(unsigned i=0;i<256;++i)put(ram+0x17c0000+i*4,vm.words[i]);put(ram+0x17f200c,0x817c0000+vm.sp*4);put(ram+0x17f2010,vm.pc);
   CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f2000;c.gpr[6]=0x817f0100;c.gpr[7]=vm.root;
   if(!execute(m,&c,&actual))return 6;for(unsigned j=0;j<3;++j)actual.transition[j]=be(ram+0x3a9a40-0x6f88+j*4);if(!nflstreet_ui_script_services(native,sizeof native,0x81100000,0x817c0000,&vm,10000,nflstreet_ui_dispatcher_service,&want,sizeof want)){fprintf(stderr,"Native dispatcher failed pass=%u\n",pass);return 7;}
   if(memcmp(native,ram+0x1100000,sizeof native)||vm.pc!=be(ram+0x17f2010)||vm.sp*4+0x817c0000!=be(ram+0x17f200c)||vm.flags!=be(ram+0x17f0100)||memcmp(&want,&actual,sizeof want)){fprintf(stderr,"Dispatcher mismatch pass=%u requests=%u/%u\n",pass,want.count,actual.count);for(unsigned k=0;k<sizeof native;++k)if(native[k]!=ram[0x1100000+k]){fprintf(stderr,"File diff=%x native=%x original=%x\n",k,native[k],ram[0x1100000+k]);break;}for(unsigned k=0;k<sizeof want;++k)if(((uint8_t*)&want)[k]!=((uint8_t*)&actual)[k]){fprintf(stderr,"Context diff=%x native=%x original=%x\n",k,((uint8_t*)&want)[k],((uint8_t*)&actual)[k]);break;}fprintf(stderr,"PC=%x/%x SP=%u/%x flags=%x/%x\n",vm.pc,be(ram+0x17f2010),vm.sp,be(ram+0x17f200c),vm.flags,be(ram+0x17f0100));for(unsigned i=0;i<want.count;++i)fprintf(stderr," request%u kind=%u target=%x link=%x aux=%x original=%u/%x/%x/%x\n",i,want.requests[i].property,want.requests[i].target,want.requests[i].link,want.requests[i].aux,actual.requests[i].property,actual.requests[i].target,actual.requests[i].link,actual.requests[i].aux);return 8;}
   for(unsigned i=0;i<256;++i)if(vm.words[i]!=be(ram+0x17c0000+i*4)){fprintf(stderr,"Dispatcher stack mismatch pass=%u word=%u\n",pass,i);return 9;}++checked;
  }
  if(want.count!=6||want.requests[5].property!=0||want.requests[5].target!=0x50000)return 10;
 }
 uint8_t owned[4273],before[4273];memcpy(owned,source,sizeof owned);nflstreet_ui_dispatcher state={0},prior;unsigned flags=0;
 if(!nflstreet_ui_dispatcher_activate(owned,sizeof owned,0x81100000,&flags,&state)||!nflstreet_ui_dispatcher_options(owned,sizeof owned,0x81100000,&flags,&state)||state.count!=6||state.requests[5].target!=0x50000)return 11;
 memcpy(owned,source,sizeof owned);memcpy(before,owned,sizeof owned);memset(&state,0,sizeof state);state.count=61;prior=state;flags=0;
 if(nflstreet_ui_dispatcher_activate(owned,sizeof owned,0x81100000,&flags,&state)||memcmp(owned,before,sizeof owned)||memcmp(&state,&prior,sizeof state)||flags)return 12;
 printf("Native event-table activation/Options wrappers pass;late request-capacity rollback preserves file/context/flags\n");
 printf("Original dispatcher actual activation and Options(96,-1) exact:%u full source-file/256-word stack/PC/flags comparisons;12 ordered load/readiness/activation requests;destination group5/entry0;requests captured, recipients not loaded\n",checked);free(ram);FreeLibrary(h);return 0;
}
