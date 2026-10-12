#include <windows.h>
#include "moderngekko/module_abi.h"
#include "link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int execute(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_ui_activation*s){
 c->lr=0;c->exception=0;c->downcount=c->cycle_budget=10000000;
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
 for(unsigned initial=0;initial<4;++initial)for(unsigned co=0;co<2;++co)for(unsigned busy=0;busy<2;++busy)for(unsigned notify=0;notify<2;++notify)for(unsigned blocked=0;blocked<2;++blocked)for(unsigned target=0;target<5;++target)for(unsigned repeat=1;repeat<=2;++repeat){
  nflstreet_ui_links native={0},original={0};nflstreet_ui_activation *ns=&native.manager,*os=&original.manager;
  ns->count=3;ns->active=initial==3?UINT32_MAX:initial;ns->flags=blocked;ns->coroutines=co;ns->busy=busy;ns->notify=notify;
  memset(ram+0x17f0000,0,0x200);put(ram+0x17f0004,ns->flags);put(ram+0x17f0030,ns->active);put(ram+0x17f0038,3);put(ram+0x17f003c,0x817e0000);put(ram+0x17f0060,co);ram[0x17f00bf]=busy;put(ram+0x17f0024,notify?0x817d1000:0);put(ram+0x17f00b0,0x817a0100);put(ram+0x17f00b4,0x817a0100);
  for(unsigned i=0;i<3;++i){unsigned file=0x817d0000+i*0x100,record=0x17e0000+i*20;ns->loaded[i]=(nflstreet_ui_loaded){i?0x10000+i:i,0,(checks+i)&1};if(target==4&&i==2)ns->loaded[i].identity=0x80000002;memset(ram+record,0,20);put(ram+record+4,ns->loaded[i].identity);put(ram+record+16,file);put(ram+file-0x80000000+4,file+0x20);put(ram+file-0x80000000+0x20,file+0x40);put(ram+file-0x80000000+0x44,ns->loaded[i].root_selected);}
  unsigned id=target<3?ns->loaded[target].identity:target==3?0x20007:0x80000002;
  original=native;uint8_t source[256]={0},owned[256];for(unsigned i=0;i<repeat;++i)source[i]=0x48;source[repeat]=0x44;memcpy(owned,source,256);memcpy(ram+0x1100000,source,256);
  nflstreet_ui_script vm={0};vm.pc=0x81100000;vm.sp=1;vm.linked_screen=id;put(ram+0x17e0008,id);memset(ram+0x17b0000,0,32);put(ram+0x17b000c,0x817c0004);put(ram+0x17b0010,vm.pc);memset(ram+0x17c0000,0,1024);
  CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817b0000;c.gpr[6]=0x817e0000;c.gpr[7]=0;c.pc=0x801d16a0;
  if(!execute(m,&c,os)||!nflstreet_ui_script_services(owned,256,0x81100000,0x817c0000,&vm,1000,nflstreet_ui_link_service,&native,sizeof native))return 5;
  if(memcmp(owned,ram+0x1100000,256)||vm.pc!=be(ram+0x17b0010)||vm.sp*4+0x817c0000!=be(ram+0x17b000c)||vm.flags!=be(ram+0x17e0000)||native.count!=repeat)return 6;
  for(unsigned i=0;i<256;++i)if(vm.words[i]!=be(ram+0x17c0000+i*4))return 6;
  for(unsigned i=0;i<repeat;++i){unsigned top=0x17a0100-i*36;if(be(ram+top)!=3||be(ram+top-4)||be(ram+top-8)||be(ram+top-28)!=id||be(ram+top-32))return 7;original.identities[original.count++]=id;}
  if(be(ram+0x17f00b4)!=0x817a0100-repeat*36)return 7;
  c.gpr[3]=0x817f0000;c.gpr[4]=0;c.pc=0x801d502c;
  if(!execute(m,&c,os)||!nflstreet_ui_links_pump(&native))return 8;
  os->flags=be(ram+0x17f0004);os->active=be(ram+0x17f0030);for(unsigned i=0;i<3;++i)os->loaded[i].root_selected=be(ram+0x17d0044+i*0x100);if(!blocked)original.count=0;
  if(be(ram+0x17f00b4)!=(blocked?0x817a0100-repeat*36:0x817a0100)||memcmp(&native,&original,sizeof native)){fprintf(stderr,"Linked activation mismatch case=%u initial=%u co=%u busy=%u notify=%u blocked=%u target=%u repeat=%u effects=%u/%u\n",checks,initial,co,busy,notify,blocked,target,repeat,ns->effect_count,os->effect_count);return 9;}++checks;
 }
 nflstreet_ui_links guard={0};guard.count=1;guard.identities[0]=0x10002;guard.manager.count=1;guard.manager.notify=1;guard.manager.effect_count=63;guard.manager.loaded[0].identity=0x10002;nflstreet_ui_links before=guard;if(nflstreet_ui_links_pump(&guard)||memcmp(&guard,&before,sizeof guard))return 10;
 uint8_t bytes[2]={0x48,0x44},copy[2];memcpy(copy,bytes,2);nflstreet_ui_script vm={0},old;vm.sp=1;vm.pc=0x81100000;guard.count=16;old=vm;before=guard;if(nflstreet_ui_script_services(bytes,2,0x81100000,0x817c0000,&vm,1000,nflstreet_ui_link_service,&guard,sizeof guard)||memcmp(&vm,&old,sizeof vm)||memcmp(&guard,&before,sizeof guard)||memcmp(bytes,copy,2))return 11;
 printf("Original linked-screen request48 and queue activation exact:%u controlled cases;owned file/256-word stack/PC/flags, meaningful queue fields/cursor, ordered script/observer boundaries and manager flags/index/root selection;queue and late-effect rollback passed;loaded records/scripts supplied, padding excluded\n",checks);free(ram);FreeLibrary(h);return 0;
}
