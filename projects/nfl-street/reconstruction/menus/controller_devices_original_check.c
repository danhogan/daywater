#include <windows.h>
#include "moderngekko/module_abi.h"
#include "controller_callback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int emit(nflstreet_controller_devices*d,CPUState*c,unsigned argc){if(d->count>=NFLSTREET_CONTROLLER_DEVICE_EFFECTS)return 0;nflstreet_controller_effect*q=&d->effects[d->count++];q->function=c->pc;q->argc=argc;for(unsigned i=0;i<argc;++i)q->args[i]=c->gpr[3+i];return 1;}
static int run(const ModernGekkoModuleDesc*m,uint8_t*ram,const nflstreet_controller_callback*seed,uint32_t fn,uint32_t arg,nflstreet_controller_devices*actual,uint32_t*out){
 *actual=seed->devices;for(unsigned i=0;i<4;++i){put(ram+0x282590+i*4,seed->selection.slots[i]);put(ram+0x2f506c+i*4,seed->selection.sides[i]);ram[0x3a9a40-0x6e60+i]=(uint8_t)seed->selection.mapping[i];}
 memcpy(ram+0x3a9a40-0x5a40,seed->devices.connected,8);put(ram+0x3a9a40-0x6e5c,seed->committed.choice);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=arg;c.gpr[4]=0x817e0100;c.gpr[6]=0x817e0200;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.pc=fn;c.cycle_budget=10000000;unsigned device=0;
 for(unsigned step=0;c.pc&&step<10000;++step){c.downcount=-1000000000;
  if(c.pc==0x80199e58){if(!emit(actual,&c,0))return 0;c.pc=c.lr;}
  else if(c.pc==0x8018013c){if(c.gpr[3]>=8||c.gpr[4]||!emit(actual,&c,2))return 0;device=c.gpr[3];c.pc=c.lr;}
  else if(c.pc==0x80199bcc){c.gpr[3]=actual->status[device];c.pc=c.lr;}
  else if(c.pc==0x80129aa4){if(c.gpr[3]>=10||!emit(actual,&c,2))return 0;actual->bindings[c.gpr[3]]=c.gpr[4];c.pc=c.lr;}
  else if(c.pc==0x80129acc){if(c.gpr[3]>=4)return 0;c.gpr[3]=actual->bindings[c.gpr[3]];c.pc=c.lr;}
  else if(c.pc==0x80129ab8){c.gpr[3]=actual->active_port;c.pc=c.lr;}
  else if(c.pc==0x80127798){if(c.gpr[4]!=4)return 0;memcpy(ram+c.gpr[3]-0x80000000,seed->next_mapping,4);c.pc=c.lr;}
  else if(c.pc==0x8010c390){c.gpr[3]=actual->query_mode;c.pc=c.lr;}
  else if(c.pc==0x8010b60c){c.gpr[3]=actual->profile_mode;c.pc=c.lr;}
  else if(c.pc==0x80127910){if(!emit(actual,&c,1))return 0;c.pc=c.lr;}
  else if(c.pc==0x801278b4){if(!emit(actual,&c,2))return 0;c.pc=c.lr;}
  else if(c.pc==0x80009e5c||c.pc==0x80009e6c){c.gpr[3]=0;c.pc=c.lr;}
  else m->dispatch(&c,c.pc);if(c.exception)return 0;
 }*out=c.gpr[3];memcpy(actual->connected,ram+0x3a9a40-0x5a40,8);return !c.pc;
}
static int state(const uint8_t*ram,const nflstreet_controller_callback*s,const nflstreet_controller_devices*actual){
 if(memcmp(&s->devices,actual,sizeof *actual))return 0;for(unsigned i=0;i<4;++i)if(s->selection.slots[i]!=be(ram+0x282590+i*4)||s->selection.sides[i]!=be(ram+0x2f506c+i*4)||s->selection.mapping[i]!=(int8_t)ram[0x3a9a40-0x6e60+i])return 0;return 1;
}
static void seed(nflstreet_controller_callback*s,unsigned bits,unsigned variant){
 memset(s,0,sizeof *s);s->selection.profile_count=4;s->devices.active_port=variant%5==4?UINT32_MAX:variant%5;s->devices.query_mode=variant%3;s->devices.profile_mode=variant%2?6:0;s->committed.choice=0x12345678+variant;
 for(unsigned i=0;i<8;++i){s->devices.status[i]=(bits&(1u<<i))?2:variant%2?1:UINT32_MAX;s->devices.connected[i]=((variant&1)?bits:bits^255)>>i&1;}
 for(unsigned i=0;i<10;++i)s->devices.bindings[i]=0x11220000+i;
 for(unsigned i=0;i<4;++i){s->selection.slots[i]=(variant+i)%7;s->selection.sides[i]=(variant+i)%3==2?UINT32_MAX:(variant+i)%3;s->selection.mapping[i]=(int8_t)i;s->next_mapping[i]=variant%4==3&&i>=2?-1:(int8_t)((i+variant)%4);}
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);unsigned refreshes=0,initializations=0,callbacks=0;
 for(unsigned bits=0;bits<256;++bits)for(unsigned v=0;v<14;++v){nflstreet_controller_callback prior;seed(&prior,bits,v);
  for(unsigned force=0;force<2;++force){nflstreet_controller_callback native=prior;nflstreet_controller_devices actual;uint32_t changed,result;if(!nflstreet_controller_refresh(&native.selection,&native.devices,force,&changed)||!run(m,ram,&prior,0x8011b430,force,&actual,&result)||changed!=result||!state(ram,&native,&actual)){fprintf(stderr,"Refresh differs bits%x variant%u force%u\n",bits,v,force);return 5;}++refreshes;}
  nflstreet_controller_callback native=prior;nflstreet_controller_devices actual;uint32_t result,choice=native.committed.choice;if(!nflstreet_controller_initialize(&native.selection,&native.devices,native.next_mapping,&choice)||!run(m,ram,&prior,0x8011b698,0x817e0040,&actual,&result)||choice!=be(ram+0x17e0040)||choice!=be(ram+0x3a9a40-0x6e5c)||!state(ram,&native,&actual)){fprintf(stderr,"Initialize differs bits%x variant%u\n",bits,v);return 6;}++initializations;
  for(unsigned k=0;k<4;++k){uint8_t bytes[1024];memset(bytes,0x5a,sizeof bytes);put(bytes+0x40,1);put(bytes+0x44,4);uint32_t stack[256]={0},out=0x1234;nflstreet_ui_service_request q={0};q.kind=1;q.property=k==3?8:k==2?0x8000000b:k?0x80000006:0x80000001;q.argc=k==3?3:k==1?2:1;q.args[0]=k==3?v%4==3?UINT32_MAX:98+v%3:k==2?v%3:0x817e0040;q.args[1]=(v&1)?0x817e0048:0x817e0080;
   if(k==3){q.args[1]=0x817e0040;q.args[2]=(v&1)?q.args[1]:0x817e0080;}
   for(unsigned i=0;i<q.argc;++i)put(bytes+0x100+i*4,q.args[i]);put(bytes+0x200,out);memcpy(ram+0x17e0000,bytes,sizeof bytes);native=prior;
   if(!nflstreet_controller_callback_memory(&native,bytes,sizeof bytes,0x817e0000,&q,&out,0x817c0000,stack)||!run(m,ram,&prior,0x8011bc68,q.property,&actual,&result)||result!=1||!state(ram,&native,&actual)||memcmp(bytes,ram+0x17e0000,sizeof bytes)){fprintf(stderr,"Callback differs bits%x variant%u property%x\n",bits,v,q.property);return 7;}if(!k&&native.committed.choice!=be(ram+0x3a9a40-0x6e5c))return 7;++callbacks;
  }
 }
 nflstreet_controller_callback guard;seed(&guard,255,1);guard.devices.count=NFLSTREET_CONTROLLER_DEVICE_EFFECTS-1;nflstreet_controller_callback before=guard;uint32_t changed=0x1234;if(nflstreet_controller_refresh(&guard.selection,&guard.devices,1,&changed)||memcmp(&guard,&before,sizeof guard)||changed!=0x1234)return 8;
 seed(&guard,255,1);guard.devices.connected[7]=2;before=guard;if(nflstreet_controller_refresh(&guard.selection,&guard.devices,1,&changed)||memcmp(&guard,&before,sizeof guard))return 8;
 printf("Original controller devices exact:%u refreshes,%u local initializations,%u actual screen callbacks; complete connected/slot/side/mapping/binding/effect states and buffers match. Supplied device responses and local profile queries; no Windows hardware discovery or source UI activation claimed. Capacity and malformed-status rollback pass.\n",refreshes,initializations,callbacks);free(ram);FreeLibrary(h);return 0;
}
