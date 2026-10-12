/* Executes the actual title activation bytecode and fade completion against
   the untouched module. Deferred broadcasts are captured, not executed. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "title.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int execute(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_ui_title*s,unsigned fn){c->pc=fn;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x801d4b3c){if(c->gpr[6]!=9||s->broadcasts==32||c->gpr[8]>16)return 0;nflstreet_ui_service_request*q=s->requests+s->broadcasts++;memset(q,0,sizeof *q);q->kind=2;q->property=be(c->ram+c->gpr[7]-0x80000000);q->argc=c->gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(c->ram+c->gpr[9]-0x80000000+j*4);c->pc=c->lr;}else m->dispatch(c,c->pc);if(c->exception)break;
 }if(c->pc||c->exception)fprintf(stderr,"Original title stopped pc=%x exception=%x\n",c->pc,c->exception);return !c->pc&&!c->exception;
}
static int rates(uint8_t*ram,const nflstreet_ui_title*s){if(be(ram+0x17f1000)!=s->entry_flags||be(ram+0x17f0054)!=s->rates.count)return 0;for(unsigned i=0;i<s->rates.count;++i)for(unsigned j=0;j<13;++j){unsigned native=s->rates.records[i].words[j];if(j==7&&native==3)native=0x817f1000;if(native!=be(ram+0x17e0000+i*52+j*4)){fprintf(stderr,"Title rate mismatch %u/%u native=%x original=%x\n",i,j,native,be(ram+0x17e0000+i*52+j*4));return 0;}}return 1;}
int main(int argc,char**argv){if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);uint8_t source[1279];f=fopen(argv[3],"rb");if(!f||fread(source,1,sizeof source,f)!=sizeof source)return 4;fclose(f);unsigned relocation;if(!nflstreet_ui_relocate(source,sizeof source,0x81100000,&relocation))return 5;
 unsigned activations=0,frames=0,broadcasts=0;
 for(unsigned seed=0;seed<4;++seed){uint8_t native[1279];memcpy(native,source,sizeof native);memcpy(ram+0x1100000,source,sizeof source);memset(ram+0x17f0000,0,0x200);memset(ram+0x17f1000,0,32);memset(ram+0x17e0000,0,64*52);memset(ram+0x17c0000,0,1024);
  put(ram+0x17f0008,16);put(ram+0x17f000c,0x8012a414);put(ram+0x17f0058,0x817e0000);put(ram+0x17f0088,0x817c0000+16);put(ram+0x17f008c,0x81100220);put(ram+0x17f1004,0x00000003);put(ram+0x17f1010,0x81100000);
  put(ram+0x17c0004,0xfffe);put(ram+0x17c0008,0x81100098);
  unsigned r13=0x3a9a40;ram[r13-0x6de8]=(uint8_t)seed;ram[r13-0x7fdb]=(uint8_t)(seed+2);ram[r13-0x6dd0]=(uint8_t)(seed+3);
  nflstreet_ui_title want={0},actual={0};want.rates.clock=16;want.title_flag=(uint8_t)seed;want.flag_470c=(uint8_t)(seed+2);want.flag_22130=(uint8_t)(seed+3);
  CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f007c;c.gpr[6]=0x817f1000;c.gpr[7]=0x81100098;
  if(!nflstreet_ui_title_activate(native,sizeof native,0x81100000,&want)||!execute(m,&c,&actual,0x801d16a0)||memcmp(native,ram+0x1100000,sizeof native)||want.title_flag!=ram[r13-0x6de8]||want.flag_470c!=ram[r13-0x7fdb]||want.flag_22130!=ram[r13-0x6dd0]||!rates(ram,&want)||want.broadcasts!=actual.broadcasts||memcmp(want.requests,actual.requests,want.broadcasts*sizeof *want.requests)){fprintf(stderr,"Activation mismatch seed=%u broadcasts=%u/%u flags=%u,%u,%u/%u,%u,%u rates=%u/%u\n",seed,want.broadcasts,actual.broadcasts,want.title_flag,want.flag_470c,want.flag_22130,ram[r13-0x6de8],ram[r13-0x7fdb],ram[r13-0x6dd0],want.rates.count,be(ram+0x17f0054));for(unsigned k=0;k<sizeof native;++k)if(native[k]!=ram[0x1100000+k]){fprintf(stderr,"File diff %x native=%x original=%x\n",k,native[k],ram[0x1100000+k]);break;}return 6;}++activations;broadcasts+=want.broadcasts;
  for(unsigned j=0;j<16;++j){c.gpr[3]=0x817f0000;c.gpr[4]=16;if(!nflstreet_ui_title_advance(native,sizeof native,0x81100000,&want,16)||!execute(m,&c,&actual,0x801cf610)||memcmp(native,ram+0x1100000,sizeof native)||!rates(ram,&want)||want.broadcasts!=actual.broadcasts||memcmp(want.requests,actual.requests,want.broadcasts*sizeof *want.requests)){fprintf(stderr,"Title fade mismatch seed=%u frame=%u native root70=%x original=%x\n",seed,j,be(native+0x108),be(ram+0x1100108));return 7;}++frames;}
 }
 unsigned guards=0;
 for(unsigned mode=0;mode<2;++mode){uint8_t guarded[1279],before[1279];memcpy(guarded,source,sizeof guarded);/* Source game callback executes before rejection. */unsigned p=0x500;guarded[p++]=0x41;guarded[p++]=0x10;put(guarded+p,0x80000001);p+=4;guarded[p++]=0x10;put(guarded+p,0);p+=4;guarded[p++]=0x0b;guarded[p++]=0xff;memcpy(before,guarded,sizeof before);nflstreet_ui_title state={0},old;state.title_flag=7;state.flag_470c=8;state.flag_22130=9;old=state;nflstreet_ui_script vm={0},oldvm;vm.sp=2;vm.pc=0x81100500;oldvm=vm;
  if(nflstreet_ui_script_services(guarded,sizeof guarded,0x81100000,0x817c0000,&vm,mode?4:100,nflstreet_ui_title_service,&state,sizeof state)||memcmp(before,guarded,sizeof before)||memcmp(&state,&old,sizeof old)||memcmp(&vm,&oldvm,sizeof vm))return 8;++guards;}
 printf("Atomic title service rollback: %u unsupported/budget guards\n",guards);
 printf("Original title activation stage exact: %u actual-script activations, %u fade/completion frames, %u ordered broadcasts; original game callbacks execute, broadcasts captured rather than dispatched; full navigation unfinished\n",activations,frames,broadcasts);free(ram);FreeLibrary(h);return 0;}
