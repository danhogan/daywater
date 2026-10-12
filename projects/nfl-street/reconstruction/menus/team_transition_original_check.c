#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_transition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define RAM_BASE 0x80000000u
#define R13 0x803a9a40u
#define STACK 0x817df000u
#define OBJECT 0x817d0400u
#define HANDLE_POINTER 0x803a2f00u
#define AUDIO_ENABLED 0x803a2f04u
#define BROADCAST_CONTEXT 0x803a2db8u
#define HANDLE_RECORD 0x817d0500u

static uint32_t expected_audio[3],expected_broadcast[4],clock_value;
static unsigned audio_calls,broadcast_calls,clock_calls;
static uint32_t rng=0x91b4c327u;
static uint32_t draw(void){rng=rng*1664525u+1013904223u;return rng;}
static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint32_t get32(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}

static int run_original(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t delay,
 uint32_t handle,uint32_t active,uint32_t context,uint32_t clock){
 put32(ram+HANDLE_POINTER-RAM_BASE,OBJECT);
 put32(ram+OBJECT-RAM_BASE,handle);
 ram[AUDIO_ENABLED-RAM_BASE]=(uint8_t)active;
 put32(ram+BROADCAST_CONTEXT-RAM_BASE,context);
 expected_audio[0]=handle;expected_audio[1]=delay;expected_audio[2]=0;audio_calls=0;
 expected_broadcast[0]=context;expected_broadcast[1]=0x80000003u;expected_broadcast[2]=0;expected_broadcast[3]=0;broadcast_calls=0;
 clock_value=clock;clock_calls=0;
 CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=delay;c.pc=0x80137b84u;
 c.downcount=c.cycle_budget=10000000;
 for(unsigned step=0;c.pc&&step<2000;++step){uint32_t pc=c.pc;c.downcount=-1000000000;
  if(pc==0x801aba64u){if(c.gpr[3]!=expected_audio[0]||c.gpr[4]!=expected_audio[1]||c.gpr[5]!=0)return 0;++audio_calls;c.pc=c.lr;}
  else if(pc==0x801af464u){++clock_calls;c.gpr[3]=clock_value;c.pc=c.lr;}
  else if(pc==0x801d49f0u){for(unsigned i=0;i<4;++i)if(c.gpr[3+i]!=expected_broadcast[i])return 0;++broadcast_calls;c.pc=c.lr;}
  else m->dispatch(&c,pc);
  if(c.exception){fprintf(stderr,"Original transition exception pc%08x ex%08x\n",c.pc,c.exception);return 0;}
 }
 return !c.pc&&c.gpr[1]==STACK;
}

static int one_case(const ModernGekkoModuleDesc*m,uint8_t*ram,unsigned test,
 uint32_t delay,uint32_t handle,uint32_t active,uint32_t context,uint32_t clock){
 uint8_t before[0x20],native_object[0x20],original_object[0x20];
 for(unsigned i=0;i<sizeof before;++i)before[i]=(uint8_t)draw();put32(before,handle);
 memcpy(native_object,before,sizeof before);memcpy(original_object,before,sizeof before);
 nflstreet_team_transition_effects effects;
 if(!nflstreet_team_transition_apply(native_object,sizeof native_object,delay,clock,active,context,&effects))return 0;
 memcpy(ram+OBJECT-RAM_BASE,original_object,sizeof original_object);
 if(!run_original(m,ram,delay,handle,active,context,clock)){fprintf(stderr,"Original transition call failed case%u\n",test);return 0;}
 memcpy(original_object,ram+OBJECT-RAM_BASE,sizeof original_object);
 if(memcmp(native_object,original_object,sizeof native_object)){fprintf(stderr,"Transition object mismatch case%u\n",test);return 0;}
 if(effects.audio_requested!=(active!=0)||effects.broadcast_context!=context||effects.broadcast_property!=0x80000003u||effects.broadcast_arg0||effects.broadcast_arg1||
    (active&&(effects.audio_handle!=handle||effects.audio_delay!=delay))||
    audio_calls!=(active!=0)||broadcast_calls!=1||clock_calls!=1){fprintf(stderr,"Transition effect mismatch case%u\n",test);return 0;}
 if(get32(native_object+0x0c)!=clock+delay||native_object[0x13]!=3){fprintf(stderr,"Transition field mismatch case%u\n",test);return 0;}
 for(unsigned i=0;i<sizeof before;++i)if(i<0x0c||i>=0x10){if(i!=0x13&&native_object[i]!=before[i]){fprintf(stderr,"Transition changed unrelated byte case%u offset%02x\n",test,i);return 0;}}
 return 1;
}

static int completion_case(const ModernGekkoModuleDesc*m,uint8_t*ram,unsigned test){
 uint8_t original[32],native[32];for(unsigned i=0;i<sizeof original;++i)original[i]=(uint8_t)draw();
 uint32_t deadline=test%4==0?0:test%4==1?UINT32_MAX:draw();
 uint32_t clock=test%4==2?deadline:test%4==3?deadline-1:draw();
 original[0x10]=(test%5==0);original[0x13]=3;put32(original+0x0c,deadline);
 uint32_t enabled=test%3?0x80:0,handle=get32(original);
 memcpy(native,original,sizeof native);memcpy(ram+OBJECT-RAM_BASE,original,sizeof original);
 put32(ram+HANDLE_POINTER-RAM_BASE,OBJECT);ram[AUDIO_ENABLED-RAM_BASE]=(uint8_t)enabled;
 nflstreet_team_transition_completion effect;
 if(!nflstreet_team_transition_advance(native,sizeof native,clock,enabled,&effect))return 0;
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;
 c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.pc=0x80137854u;c.downcount=c.cycle_budget=10000000;
 unsigned clocks=0,stops=0,updates=0;
 for(unsigned step=0;c.pc&&step<2000;++step){uint32_t pc=c.pc;c.downcount=-1000000000;
  if(pc==0x801af464u){++clocks;c.gpr[3]=clock;c.pc=c.lr;}
  else if(pc==0x801abae4u){if(c.gpr[3]!=handle||c.gpr[4]!=0)return 0;++stops;c.pc=c.lr;}
  else if(pc==0x801abf1cu){++updates;c.pc=c.lr;}
  else m->dispatch(&c,pc);
  if(c.exception)return 0;
 }
 unsigned completed=!original[0x10]&&deadline<=clock;
 if(c.pc||c.gpr[1]!=STACK||memcmp(native,ram+OBJECT-RAM_BASE,sizeof native)||
    clocks!=!original[0x10]||updates!=1||stops!=(completed&&enabled)||
    effect.completed!=completed||effect.audio_stop_requested!=(completed&&enabled)||
    (effect.audio_stop_requested&&effect.audio_handle!=handle)){
  fprintf(stderr,"Original timed transition completion differs case%u\n",test);return 0;
 }
 return 1;
}

int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"Usage: team-transition-original-check.exe <original-DLL> <snapshot.mem1>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 FILE*f=fopen(argv[2],"rb");uint8_t*ram=malloc(RAM_SIZE);if(!ram||!f||fread(ram,1,RAM_SIZE,f)!=RAM_SIZE)return 4;fclose(f);
 unsigned cases=0;for(unsigned i=0;i<1000;++i){uint32_t delay=i==0?0:i==1?UINT32_MAX:draw();uint32_t handle=draw();uint32_t active=i%3==0?0:i%3==1?1:0x80;uint32_t context=draw(),clock=i==2?UINT32_MAX:draw();
  if(!one_case(m,ram,i,delay,handle,active,context,clock))return 5;++cases;}
 printf("80137B84 exact for %u caller-owned object cases; audio, clock, and broadcast service calls were captured as supplied effects and not executed.\n",cases);
 for(unsigned i=0;i<256;++i)if(!completion_case(m,ram,i))return 6;
 puts("80137854 phase3 deadline / 80137C80 completion exact for256 caller-owned cases; clock/audio/global-update effects supplied, other phases and match creation excluded.");
 free(ram);FreeLibrary(h);return 0;
}
