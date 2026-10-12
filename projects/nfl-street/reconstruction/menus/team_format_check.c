#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_format.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x81100000u
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int one(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const char*format,unsigned cap,unsigned argc,const uint32_t*args){
 uint8_t*ram=malloc(RAM_SIZE),b[4096];if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);memset(b,0xa5,sizeof b);
 put(b,0);put(b+4,cap);put(b+8,BASE+0x500);put(b+0x20,0);put(b+0x24,128);put(b+0x28,BASE+0x100);strcpy((char*)b+0x100,format);
 put(b+0x40,0);put(b+0x44,20);put(b+0x48,BASE+0x200);strcpy((char*)b+0x200,"RAVENS");
 for(unsigned i=0;i<argc;++i)put(b+0x300+i*4,args[i]);memcpy(ram+0x1100000,b,sizeof b);
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=0;c.gpr[4]=BASE;c.gpr[5]=BASE+0x20;c.gpr[6]=argc;c.gpr[7]=BASE+0x300;c.pc=0x801d4434;c.lr=0;c.cycle_budget=10000000;
 for(unsigned i=0;c.pc&&i<100000;++i){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)break;}
 nflstreet_ui_service_request q={0};q.kind=10;q.target=BASE;q.property=BASE+0x20;q.argc=argc;memcpy(q.args,args,argc*4);
 int ok=!c.pc&&!c.exception&&nflstreet_team_format(b,sizeof b,BASE,&q)&&!memcmp(b,ram+0x1100000,sizeof b)&&c.gpr[1]==0x81700000;
 if(!ok)fprintf(stderr,"Format differs '%s' cap%u pc%x ex%x native'%s' original'%s'\n",format,cap,c.pc,c.exception,b+0x500,ram+0x1100500);free(ram);return ok;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 FILE*f=fopen(argv[2],"rb");uint8_t*ram=malloc(RAM_SIZE);if(!f||!ram||fread(ram,1,RAM_SIZE,f)!=RAM_SIZE)return 4;fclose(f);
 const char*formats[]={"Team label","%d","%i","%u","%s","%d / %s %%"};unsigned caps[]={0,1,4,32};uint32_t args[3]={0};unsigned cases=0;
 for(unsigned i=0;i<6;++i)for(unsigned j=0;j<4;++j)for(unsigned k=0;k<3;++k){unsigned count=i==0?0:i==5?2:1;args[0]=i==4?BASE+0x40:k==0?0:k==1?42:UINT32_MAX;args[1]=BASE+0x40;if(!one(m,ram,formats[i],caps[j],count,args))return 5;++cases;}
 printf("Team formatter original exact: %u controlled literal/d/i/u/s/percent cases, caps0/1/4/32; original801D4434/801D4170 and string/numeric writers execute. Full4096-byte owner and SP match; width/precision formats unsupported.\n",cases);free(ram);FreeLibrary(h);return 0;
}
