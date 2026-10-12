#include <windows.h>
#include "moderngekko/module_abi.h"
#include "activation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int execute(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_ui_activation*s){
 c->pc=0x801d1090;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=10000000;
 for(unsigned j=0;c->pc&&j<10000;++j){
  if(c->pc==0x801cf34c||c->pc==0x817d1000){if(s->effect_count>=64)return 0;unsigned active=be(c->ram+0x17f0030),kind=c->pc==0x817d1000;
   if(active>=s->count)return 0;unsigned identity=s->loaded[active].identity;
   if(kind){if(c->gpr[3]!=(identity>>16)||c->gpr[4]!=(identity&65535))return 0;}
   else if(c->gpr[4]!=0x817e0000+active*20||c->gpr[5]!=0x817f0068||c->gpr[6]||c->gpr[7]||c->gpr[9]||c->gpr[10])return 0;
   s->effects[s->effect_count++]=(nflstreet_ui_activation_effect){kind,kind?0:c->gpr[8],active,be(c->ram+0x17f0004),active,identity};c->pc=c->lr;
  }else m->dispatch(c,c->pc);
  if(c->exception)return 0;
 }return !c->pc;
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);unsigned checks=0;
 for(unsigned initial=0;initial<4;++initial)for(unsigned co=0;co<2;++co)for(unsigned busy=0;busy<2;++busy)for(unsigned notify=0;notify<2;++notify)for(unsigned activate=0;activate<2;++activate)for(unsigned target=0;target<4;++target){
  nflstreet_ui_activation native={0},original={0};native.count=3;native.active=initial==3?UINT32_MAX:initial;native.flags=(checks%4)*2;native.coroutines=co;native.busy=busy;native.notify=notify;
  memset(ram+0x17f0000,0,0x200);put(ram+0x17f0004,native.flags);put(ram+0x17f0030,native.active);put(ram+0x17f0038,3);put(ram+0x17f003c,0x817e0000);put(ram+0x17f0060,co);ram[0x17f00bf]=busy;put(ram+0x17f0024,notify?0x817d1000:0);
  for(unsigned i=0;i<3;++i){unsigned file=0x817d0000+i*0x100,record=0x17e0000+i*20;native.loaded[i]=(nflstreet_ui_loaded){i?0x10000+i:i,0,(checks+i)&1};memset(ram+record,0,20);put(ram+record+4,native.loaded[i].identity);put(ram+record+16,file);put(ram+file-0x80000000+4,file+0x20);put(ram+file-0x80000000+0x20,file+0x40);put(ram+file-0x80000000+0x44,native.loaded[i].root_selected);}
  original=native;unsigned id=target<3?native.loaded[target].identity:0x20007;
  if(!nflstreet_ui_activate(&native,activate,id))return 5;CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f0000;c.gpr[4]=activate;c.gpr[5]=id>>16;c.gpr[6]=id&65535;
  if(!execute(m,&c,&original))return 6;
  original.flags=be(ram+0x17f0004);original.active=be(ram+0x17f0030);for(unsigned i=0;i<3;++i)original.loaded[i].root_selected=be(ram+0x17d0044+i*0x100);
  if(memcmp(&native,&original,sizeof native)){fprintf(stderr,"Activation mismatch case=%u initial=%u co=%u busy=%u notify=%u activate=%u target=%u effects=%u/%u\n",checks,initial,co,busy,notify,activate,target,native.effect_count,original.effect_count);return 7;}++checks;
 }
 nflstreet_ui_activation guard={0};guard.count=1;guard.notify=1;guard.effect_count=63;guard.loaded[0].identity=0x10002;nflstreet_ui_activation before=guard;if(nflstreet_ui_activate(&guard,1,0x10002)||memcmp(&guard,&before,sizeof guard))return 8;
 printf("Original loaded-screen activation exact:%u controlled cases;ordered deactivation/observer/activation boundaries captured,root selected and manager flags/index compared;late-capacity rollback passed;loaded records and scripts supplied\n",checks);free(ram);FreeLibrary(h);return 0;
}
