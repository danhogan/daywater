#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_actor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define OBJECT 0x817d1000u
#define ARGUMENT 0x817d2000u
#define CALLBACK_SLOT 0x803a2dd8u
#define CALLBACK_TRAMP 0x817d7000u
#define STACK 0x817df000u
#define SELECTED 0x12345678u
#define SENTINEL 0xabcddcbau

static const uint32_t object_words[5]={SELECTED,0xc0200000u,0x3f800001u,0x41200000u,0x3eaaaaabu};
typedef struct {unsigned calls;uint32_t gpr[10];double fpr[4];} callback_capture;

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=v;}
static uint8_t*load_file(const char*path,size_t*size){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(unsigned long)z>RAM_SIZE||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}
static float word_float(uint32_t word){float f;memcpy(&f,&word,4);return f;}
static void seed(uint8_t*ram,uint32_t property,int registered){for(unsigned i=0;i<5;++i)put(ram+OBJECT-BASE+i*4,object_words[i]);put(ram+ARGUMENT-BASE,SELECTED);put(ram+CALLBACK_SLOT-BASE,registered?CALLBACK_TRAMP:0);put(ram+0x17d3000,SENTINEL);(void)property;}

static int original_call(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t property,
                         int registered,callback_capture*capture,uint32_t*return_reg){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 c.gpr[3]=OBJECT;c.gpr[4]=property;c.gpr[5]=0;c.gpr[6]=ARGUMENT;c.pc=0x8012c570;c.lr=0;c.cycle_budget=10000000;
 for(unsigned step=0;c.pc&&step<10000;++step){uint32_t pc=c.pc;
  if(pc==CALLBACK_TRAMP){if(!registered||capture->calls)return 0;++capture->calls;
   for(unsigned i=0;i<10;++i)capture->gpr[i]=c.gpr[i+3];for(unsigned i=0;i<4;++i)capture->fpr[i]=c.fpr[i+1];c.pc=c.lr;
  }else{c.downcount=-1000000000;m->dispatch(&c,pc);}
  if(c.exception){fprintf(stderr,"Original class3 exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}
 }
 if(c.pc||c.gpr[1]!=STACK){fprintf(stderr,"Original class3 stopped pc=%08x SP=%08x\n",c.pc,c.gpr[1]);return 0;}*return_reg=c.gpr[3];return 1;
}

static int compare_one(const ModernGekkoModuleDesc*m,const uint8_t*baseline,uint32_t property,int registered){
 uint8_t*actual=malloc(RAM_SIZE),*native_ram=malloc(RAM_SIZE);if(!actual||!native_ram)return 0;memcpy(actual,baseline,RAM_SIZE);memcpy(native_ram,baseline,RAM_SIZE);
 seed(actual,property,registered);seed(native_ram,property,registered);uint8_t before[20];memcpy(before,actual+OBJECT-BASE,20);
 callback_capture captured={0};uint32_t ret=0;if(!original_call(m,actual,property,registered,&captured,&ret)){free(actual);free(native_ram);return 0;}
 nflstreet_team_actor state={0};state.callback_registered=(uint32_t)registered;nflstreet_ui_service_request q={0};q.kind=0;q.class_id=3;q.target=OBJECT;q.property=property;if(property==0){q.argc=1;q.args[0]=SELECTED;}
 uint32_t result=SENTINEL;if(!nflstreet_team_actor_service(&state,native_ram,RAM_SIZE,BASE,&q,&result)||result!=SENTINEL){fprintf(stderr,"Native class3 rejected prop=%08x registered%d\n",property,registered);free(actual);free(native_ram);return 0;}
 if(property==0)put(before,SELECTED);
 if(memcmp(actual+OBJECT-BASE,native_ram+OBJECT-BASE,20)||memcmp(native_ram+OBJECT-BASE,before,20)||(!(property==0xfffffffeu&&registered)&&ret!=OBJECT)){fprintf(stderr,"Class3 object/return differs prop=%08x registered%d ret=%08x\n",property,registered,ret);free(actual);free(native_ram);return 0;}
 unsigned expected_calls=(property==0xfffffffeu&&registered)?1u:0u;
 if(captured.calls!=expected_calls||state.request_count!=expected_calls){fprintf(stderr,"Class3 callback count differs prop=%08x registered%d original%u native%u\n",property,registered,captured.calls,state.request_count);free(actual);free(native_ram);return 0;}
 if(expected_calls){const uint32_t order[4]={object_words[2],object_words[1],object_words[3],object_words[4]};
  for(unsigned i=0;i<4;++i){double expected=(double)word_float(order[i]);if(captured.fpr[i]!=expected||state.requests[0].float_words[i]!=order[i]){fprintf(stderr,"Class3 float arg%u differs original%.17g expected%.17g word%08x\n",i,captured.fpr[i],expected,order[i]);free(actual);free(native_ram);return 0;}}
  uint32_t selected=state.requests[0].selected_index;int found=0;for(unsigned i=0;i<10;++i)if(captured.gpr[i]==selected)found=1;
  if(!found){fprintf(stderr,"Class3 selected index %08x absent from callback GPRs r3-r12:",selected);for(unsigned i=0;i<10;++i)fprintf(stderr," %08x",captured.gpr[i]);fputc('\n',stderr);free(actual);free(native_ram);return 0;}
 }
 if(be(actual+CALLBACK_SLOT-BASE)!=(registered?CALLBACK_TRAMP:0)||be(native_ram+CALLBACK_SLOT-BASE)!=(registered?CALLBACK_TRAMP:0)){free(actual);free(native_ram);return 0;}
 free(actual);free(native_ram);return 1;
}

static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}
int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"Usage: team-actor-check.exe <original-DLL> <snapshot.mem1>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t z=0;uint8_t*baseline=load_file(argv[2],&z);if(!baseline||z!=RAM_SIZE)return 4;uint64_t hash=hash_bytes(baseline,z);const uint32_t props[]={0,UINT32_MAX,0xfffffffcu,0xfffffffeu};unsigned cases=0;
 for(unsigned i=0;i<4;++i)for(unsigned registered=0;registered<2;++registered){if(!compare_one(m,baseline,props[i],(int)registered)){fprintf(stderr,"Class3 compare failed property=%08x registration=%u\n",props[i],registered);return 5;}++cases;}
 size_t after_size=0;uint8_t*after=load_file(argv[2],&after_size);if(!after||after_size!=z||hash_bytes(after,after_size)!=hash){fprintf(stderr,"Snapshot baseline changed during check\n");return 6;}
 printf("Team class3 original callback routing exact: %u direct 8012C570 cases; property0 selected-index store, FFFFFFFF/FFFFFFFC no-op, FFFFFFFE null registration and supplied callback capture with four converted float args plus selected index. Explicit callback trampoline only; 801289A4 renderer and visible actor not verified. Baseline FNV64 %016llx unchanged.\n",cases,(unsigned long long)hash);
 free(after);free(baseline);FreeLibrary(h);return 0;
}
