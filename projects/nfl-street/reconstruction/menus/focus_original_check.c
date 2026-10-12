/* Complete original focus selection, script execution boundary captured. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "focus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned target,unsigned selected,unsigned arg,nflstreet_ui_focus_effects*out){
 c->gpr[3]=0x817f0000;c->gpr[4]=selected;c->gpr[5]=arg;c->gpr[6]=target;c->gpr[7]=0;c->gpr[8]=0;c->gpr[9]=1;c->pc=0x801d0e18;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;
 for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x801d399c){if(out->count>=4||c->gpr[8]!=2||c->gpr[10]!=0)return 0;unsigned p=c->gpr[9]-0x80000000;if(p>24*1024*1024-8)return 0;
   out->requests[out->count++]=(nflstreet_ui_focus_request){c->gpr[5]-0x81100000,c->gpr[7]-0x81100000,{be(c->ram+p),be(c->ram+p+4)}};c->gpr[3]=0;c->pc=c->lr;
  }else m->dispatch(c,c->pc);if(c->exception)break;
 }return !c->pc&&!c->exception;
}
int main(int argc,char**argv){if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);f=fopen(argv[3],"rb");if(!f)return 4;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<32||n>1000000)return 4;uint8_t*source=malloc((size_t)n),*native=malloc((size_t)n);if(!source||!native||fread(source,1,(size_t)n,f)!=(size_t)n)return 4;fclose(f);unsigned result;if(!nflstreet_ui_relocate(source,(size_t)n,0x81100000,&result))return 5;
 unsigned nodes=be(source),table=be(source+4)-0x81100000,checked=0,requests=0;
 for(unsigned previous=0;previous<=nodes;++previous)for(unsigned target=0;target<nodes;++target)for(unsigned selected=0;selected<2;++selected){
  memcpy(native,source,(size_t)n);for(unsigned i=0;i<nodes;++i)put(native+be(native+table+i*20)-0x81100000+4,i==previous);
  memcpy(ram+0x1100000,native,(size_t)n);memset(ram+0x17f0000,0,0x200);put(ram+0x17f0038,1);put(ram+0x17f003c,0x817f0100);put(ram+0x17f0104,0x00010000);put(ram+0x17f0110,0x81100000);
  CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;
  nflstreet_ui_focus_effects want={0},actual={0};unsigned arg=target*17+previous;
  if(!nflstreet_ui_focus(native,(size_t)n,0x81100000,target,selected,arg,&want)||!call(m,&c,0x81100000+be(source+table+target*20)-0x81100000,selected,arg,&actual)||memcmp(native,ram+0x1100000,(size_t)n)||want.count!=actual.count||memcmp(want.requests,actual.requests,want.count*sizeof *want.requests)){for(long d=0;d<n;++d)if(native[d]!=ram[0x1100000+d]){fprintf(stderr,"First diff %lx native=%02x original=%02x\n",d,native[d],ram[0x1100000+d]);break;}fprintf(stderr,"Focus mismatch previous=%u target=%u selected=%u native requests=%u original=%u\n",previous,target,selected,want.count,actual.count);return 6;}++checked;requests+=want.count;
 }
 printf("Original focus exact: %u complete-file states and %u ordered script requests; previous selection/all targets/select-deselect; original script calls captured, not executed; optional index lists unsupported\n",checked,requests);free(native);free(source);free(ram);FreeLibrary(h);return 0;}
