#include <windows.h>
#include "moderngekko/module_abi.h"
#include "construct.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
typedef struct {unsigned count;nflstreet_ui_service_request requests[1024];} effects;
static int service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){(void)b;(void)n;(void)base;(void)result;effects*s=ctx;if(s->count>=1024)return 0;s->requests[s->count++]=*q;return 1;}
static int execute(const ModernGekkoModuleDesc*m,CPUState*c,effects*s){
 c->pc=0x801ce994;c->lr=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x817d1000||c->pc==0x817d1008){if(s->count>=1024||c->gpr[5]||c->gpr[6]||c->gpr[7])return 0;nflstreet_ui_service_request q={0};q.kind=0;q.class_id=c->pc&15;q.target=c->gpr[3];q.property=c->gpr[4];s->requests[s->count++]=q;c->pc=c->lr;}else m->dispatch(c,c->pc);if(c->exception)return 0;}return !c->pc;
}
int main(int argc,char**argv){if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);f=fopen(argv[3],"rb");if(!f)return 4;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<32||n>1000000)return 4;uint8_t*b=malloc(n),*source=malloc(n),*before=malloc(n);if(!b||!source||!before||fread(b,1,n,f)!=(size_t)n)return 4;fclose(f);unsigned relocation;if(!nflstreet_ui_relocate(b,n,0x81100000,&relocation))return 5;memcpy(source,b,n);memcpy(ram+0x1100000,b,n);
 memset(ram+0x17f0000,0,0x200);put(ram+0x17f0048,9);put(ram+0x17f004c,0x817b0000);memset(ram+0x17b0000,0,36);put(ram+0x17b0000,0x817d1000);put(ram+0x17b0020,0x817d1008);put(ram+0x17f0110,0x81100000);
 effects native={0},original={0};unsigned count0=0,count8=0;
 for(unsigned pass=0;pass<2;++pass){unsigned property=pass?0:UINT32_MAX;
  if(!nflstreet_ui_construct(b,n,0x81100000,0,property,0x101,service,&native,sizeof native))return 6;
  CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f0000;c.gpr[4]=0x817f0100;c.gpr[5]=0;c.gpr[6]=property;
  if(!execute(m,&c,&original)||memcmp(b,ram+0x1100000,n)||memcmp(&native,&original,sizeof native)){fprintf(stderr,"Construction mismatch pass=%u effects=%u/%u\n",pass,native.count,original.count);return 7;}
  if(!pass)for(unsigned i=0;i<native.count;++i){count0+=native.requests[i].class_id==0;count8+=native.requests[i].class_id==8;}
 }
 memcpy(b,source,n);memcpy(before,b,n);effects prior={0};memset(&native,0,sizeof native);native.count=1023;prior=native;
 if(nflstreet_ui_construct(b,n,0x81100000,0,UINT32_MAX,0x101,service,&native,sizeof native)||memcmp(b,before,n)||memcmp(&native,&prior,sizeof native))return 8;
 printf("Original Options class construction traversal exact:2 full owned-file comparisons,%u ordered registered callbacks (first pass class0=%u class8=%u);global and link initialization flags compared;late callback-capacity rollback;callbacks captured, font/images not initialized\n",original.count,count0,count8);free(b);free(source);free(before);free(ram);FreeLibrary(h);return 0;
}
