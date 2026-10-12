#include <windows.h>
#include "moderngekko/module_abi.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i)m->dispatch(c,c->pc);return !c->pc&&!c->exception;}
static void cpu(CPUState*c,uint8_t*ram){memset(c,0,sizeof *c);c->gpr[1]=0x81700000;c->gpr[2]=0x803b9a40;c->gpr[13]=0x803a9a40;c->ram=ram;c->ram_size=24*1024*1024;c->msr=0x2000;}
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);unsigned screens=0,relocations=0,events=0;
 for(unsigned group=0;group<12;++group)for(unsigned entry=0;entry<1024;++entry){char path[4096];snprintf(path,sizeof path,"%s/%05u/%05u.bin",argv[3],group,entry);f=fopen(path,"rb");if(!f)break;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<32||n>1024*1024)return 4;uint8_t*source=malloc((size_t)n),*native=malloc((size_t)n);if(!source||!native||fread(source,1,(size_t)n,f)!=(size_t)n)return 4;fclose(f);++screens;
  for(unsigned location=0;location<2;++location){unsigned base=location?0x81200000:0x81100000,at=base-0x80000000;memcpy(native,source,(size_t)n);memcpy(ram+at,source,(size_t)n);uint32_t result;
   if(!nflstreet_ui_relocate(native,(size_t)n,base,&result)){fprintf(stderr,"Native relocation rejected %u/%u\n",group,entry);return 5;}
   CPUState c;cpu(&c,ram);c.gpr[3]=base;if(!call(m,&c,0x801cf8c8)||c.gpr[3]!=result)return 6;
   if(memcmp(native,ram+at,(size_t)n)){for(unsigned i=0;i<(unsigned)n;++i)if(native[i]!=ram[at+i]){fprintf(stderr,"Relocation %u/%u at=%x byte=%x native=%x original=%x\n",group,entry,base,i,native[i],ram[at+i]);break;}return 7;}++relocations;
   if(!nflstreet_ui_relocate(native,(size_t)n,base,&result)||result!=0xffffffff)return 8;cpu(&c,ram);c.gpr[3]=base;if(!call(m,&c,0x801cf8c8)||c.gpr[3]!=result||memcmp(native,ram+at,(size_t)n))return 8;
   unsigned nodes=be(native),rows=be(native+4)-base;
   for(unsigned node=0;node<nodes;++node){const uint8_t*r=native+rows+node*20;unsigned count=be(r+12),table=be(r+16)-base;
    for(unsigned ev=0;ev<count+1;++ev){uint16_t event=ev<count?(uint16_t)sh(native+table+ev*8+2):0x7ffe,id=ev<count?(uint16_t)(sh(native+table+ev*8)&0x2fff):0;if(event==0xffff)continue;
     for(unsigned kind=0;kind<3;++kind){uint32_t off;unsigned found;if(!nflstreet_ui_event(native,(size_t)n,base,node,kind,event,id,&off,&found))return 9;
      cpu(&c,ram);c.gpr[3]=base+rows+node*20;c.gpr[4]=kind==2?id:event;c.gpr[5]=event;unsigned pc=kind==0?0x801d3890:kind==1?0x801d3948:0x801d38e4;if(!call(m,&c,pc)||c.gpr[3]!=(found?base+off:0)){fprintf(stderr,"Event %u/%u node=%u kind=%u event=%x\n",group,entry,node,kind,event);return 10;}++events;
     }
    }
   }
  }free(native);free(source);
 }
 uint8_t invalid[32]={0},saved[32];invalid[7]=0xff;memcpy(saved,invalid,32);uint32_t result=0x12345678;
 if(nflstreet_ui_relocate(invalid,32,0x81100000,&result)||memcmp(saved,invalid,32)||result!=0x12345678)return 11;
 printf("Original screen checks exact: screens=%u complete-relocations=%u event-selections=%u; two supplied logical bases, idempotence and atomic malformed guard pass; VM/navigation unfinished\n",screens,relocations,events);free(ram);FreeLibrary(h);return 0;
}
