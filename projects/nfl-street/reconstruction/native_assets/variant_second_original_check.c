/* Controlled request/second-key/hierarchy oracle, not full gameplay. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../actor_target_input/variant_selection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned u32(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static uint8_t*read(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size<0){fclose(f);return NULL;}uint8_t*b=malloc(size);if(!b||fread(b,1,size,f)!=(size_t)size){free(b);b=NULL;}fclose(f);*n=size;return b;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){m->dispatch(c,c->pc);if(c->exception)break;}if(c->pc||c->exception){fprintf(stderr,"Original call %08x failed pc=%08x ex=%08x\n",pc,c->pc,c->exception);return 0;}return 1;}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static float real(const uint8_t*p){unsigned w=u32(p);float f;memcpy(&f,&w,4);return f;}
static void scalar(uint8_t*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
int main(int argc,char**argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;size_t rn;uint8_t*ram=read(argv[2],&rn);if(!ram||rn!=24*1024*1024)return 5;
 uint8_t*seed=malloc(rn);if(!seed)return 5;memcpy(seed,ram,rn);unsigned reg=u32(ram+0x3a4ad4),registered=u32(ram+0x3a4ad0);
 nflstreet_asset_store store={0x80000000,ram,ram+(reg-0x80000000),rn,registered*28};
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=rn;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 uint8_t*actor=ram+0x3c19c0;unsigned requests=0,seconds=0,joins=0;
 const float speeds[]={0,-0.0f,1.0e-7f,nextafterf(1.0e-7f,-INFINITY),nextafterf(1.0e-7f,INFINITY),0.1f};
 for(unsigned selector=0;selector<11;++selector)for(unsigned state=0;state<5;++state)for(unsigned arg=0;arg<2;++arg)for(unsigned v=0;v<6;++v){
  memcpy(actor,seed+0x3c19c0,0x14d4);put(actor+0x408,selector);put(actor+0x418,state);put(actor+0x40c,0);scalar(actor+0x1c4,speeds[v]);
  nflstreet_request_facts facts={0};facts.selector_word=selector;facts.request_state=state;facts.service_argument=arg;facts.speed=speeds[v];facts.target_available=1;
  unsigned ours=255;c.gpr[3]=0x803c19c0;c.gpr[4]=arg;if(!call(m,&c,0x80062834)||!nflstreet_variant_request(&facts,&ours)||ours!=c.gpr[3]){fprintf(stderr,"Request mismatch selector=%u state=%u arg=%u speed=%g native=%u original=%u\n",selector,state,arg,speeds[v],ours,c.gpr[3]);return 6;}++requests;
 }

 unsigned phase_cases=0;const uint32_t headings[]={0x7fffff,0x800000,0xffffff,0xffffffff};
 for(unsigned negative=0;negative<2;++negative)for(unsigned moving=0;moving<2;++moving)for(unsigned angle=0;angle<4;++angle)for(unsigned same=0;same<2;++same)for(unsigned arg=0;arg<2;++arg){
  memcpy(ram,seed,rn);uint32_t root=u32(ram+0x3a29c8)-0x80000000;put(ram+root+0x34,u32(ram+root+0x34)&~1u);
  nflstreet_phase_facts facts={0};facts.comparison=negative?-0.1f:0;facts.speed=moving?0.1f:0;facts.heading=headings[angle];facts.same_actor=same;facts.service_argument=arg;facts.callback_tag=1;facts.global_flags_available=1;
  unsigned ours=91;if(!nflstreet_variant_phase_simple(&facts,&ours)){if(ours!=91)return 17;continue;}
  scalar(actor+0x1dc,facts.comparison);scalar(actor+0x1c4,facts.speed);put(actor+0x1c0,facts.heading);
  c.gpr[3]=0x803c19c0;c.gpr[4]=same?0x803c19c0:0;c.gpr[5]=actor[2];c.gpr[6]=arg;
  if(!call(m,&c,0x80062950)||ours!=c.gpr[3])return 18;++phase_cases;
 }
 for(unsigned case_id=0;case_id<1544;++case_id){
  unsigned status=case_id<1536?case_id/6:case_id==1536?22:case_id==1537?23:case_id==1543?31:0;
  unsigned same=case_id<1536?(case_id/3)%2:case_id==1536||case_id==1537||case_id==1543;
  unsigned flag=case_id<1536?case_id%3:0;
  memcpy(ram,seed,rn);actor[0x21f]=status;put(actor+0x418,flag?1:0);put(actor+0x408,flag?4:0);

  if(case_id==1536||case_id==1537){put(actor+0x4c8,case_id==1536?3:4);actor[0x4cc]=3;actor[0x4cd]=4;actor[0x4d3]=0;}
  if(case_id==1538)actor[0x21e]=9;
  if(case_id>=1539&&case_id<=1541){ram[u32(actor+0x310)-0x80000000]=42;put(actor+0x1c0,0xc00000);
   unsigned focus=u32(ram+0x3a29c8)-0x80000000;scalar(actor+0x1a8,real(ram+focus+12)+(case_id==1540?9:0));scalar(actor+0x1ac,real(ram+focus+16)+(case_id==1541?6:0));}
  if(case_id==1542)actor[3]=0;
  if(case_id>=1542)put(ram+(u32(ram+0x3a1d48)-0x80000000),1);
  uint8_t saved_actor[0x14d4];memcpy(saved_actor,actor,sizeof saved_actor);unsigned callback_pointer=u32(actor+0x310)-0x80000000;uint8_t saved_callback=ram[callback_pointer];
  nflstreet_variant_facts f={0};f.kind_byte=actor[3];f.side_byte=actor[2];f.status_byte=status;f.variant_byte=actor[0xb65];f.callback_tag=ram[u32(actor+0x310)-0x80000000];f.selector_word=u32(actor+0x408);f.global_selector=actor[2];f.available=15;f.special_word=u32(actor+0x4c8);f.special_enabled=actor[0x4d3]!=0;
  nflstreet_variant_second_facts s={0};s.same_actor=same;s.first_flag=flag;s.request_state=u32(actor+0x418);s.available=127;s.aux_byte=actor[0xb5e];s.control_byte=actor[0x21e];s.side_flag=actor[0xb5d];s.heading=u32(actor+0x1c0);s.speed=real(actor+0x1c4);s.special_table=seed+0x26e51c;s.table_size=256;
  s.x=real(actor+0x1a8);s.y=real(actor+0x1ac);unsigned focus=u32(ram+0x3a29c8)-0x80000000;s.focus_x=real(ram+focus+12);s.focus_y=real(ram+focus+16);
  unsigned other=same?0x803c19c0:0;
  c.gpr[3]=0x803c19c0;c.gpr[4]=0;if(!call(m,&c,0x80062834))return 7;unsigned expected_request=c.gpr[3];nflstreet_request_facts request={0};request.selector_word=f.selector_word;request.request_state=s.request_state;request.speed=s.speed;
  if(!nflstreet_variant_request(&request,&s.request_predicate)||s.request_predicate!=expected_request)return 13;f.predicate=s.request_predicate;
  c.gpr[3]=0x803c19c0;c.gpr[4]=other;c.gpr[5]=f.global_selector;c.gpr[6]=0;if(!call(m,&c,0x80062950))return 7;s.phase_predicate=c.gpr[3];
  if(case_id<1536){nflstreet_phase_facts phase={0};phase.comparison=real(actor+0x1dc);phase.speed=s.speed;phase.heading=s.heading;phase.callback_tag=f.callback_tag;phase.same_actor=same;phase.global_flags_available=1;phase.global_flags=u32(ram+focus+0x34);
   unsigned computed;if(!nflstreet_variant_phase_simple(&phase,&computed)||computed!=s.phase_predicate)return 14;s.phase_predicate=computed;}
  c.gpr[3]=0x803c19c0;if(!call(m,&c,0x8006510c))return 7;f.special_predicate=c.gpr[3];
  c.gpr[3]=0x803c19c0;if(!call(m,&c,0x80065290))return 7;s.additional_predicate=c.gpr[3];if(f.callback_tag==1){unsigned computed;if(!nflstreet_variant_additional_simple(f.callback_tag,&computed)||computed!=s.additional_predicate)return 15;s.additional_predicate=computed;}
  if(!call(m,&c,0x80104238))return 7;s.ending_predicate=c.gpr[3];
  c.gpr[3]=0x803c19c0;if(!call(m,&c,f.special_word==4?0x8004d89c:0x8004d840))return 7;s.special_index=c.gpr[3];
  uint8_t computed_index;if(!nflstreet_variant_index(f.callback_tag,f.special_enabled,saved_actor[f.special_word==4?0x4cd:0x4cc],&computed_index)||computed_index!=(uint8_t)s.special_index)return 16;s.special_index=computed_index;
  memcpy(ram,seed,rn);memcpy(actor,saved_actor,sizeof saved_actor);ram[callback_pointer]=saved_callback;if(case_id>=1542)put(ram+(u32(ram+0x3a1d48)-0x80000000),1);
  c.gpr[3]=0x803c19c0;c.gpr[4]=other;c.gpr[5]=f.global_selector;c.gpr[6]=0;c.gpr[7]=flag;
  uint8_t key=255;if(!call(m,&c,0x80062e2c)||!nflstreet_variant_second(&f,&s,&key)||key!=(uint8_t)c.gpr[3]){fprintf(stderr,"Second-key mismatch status=%u same=%u flag=%u key=%u/%u\n",status,same,flag,key,c.gpr[3]);return 8;}++seconds;if(case_id>=1536)printf("Secondary controlled case%u: key=%u\n",case_id-1536,key);
  if(case_id<1536&&status<8&&same&&!flag){uint8_t data[64]={0},second;nflstreet_variant_choice choice;
   if(!nflstreet_variant_resolve_pair(&store,1,&f,&s,2,data,&choice,&second))return 9;
   c.gpr[3]=0x803c19c0;c.gpr[4]=f.global_selector;c.gpr[5]=0;c.gpr[6]=0x817f8000;ram[0x17f8000]=2;if(!call(m,&c,0x80062c30))return 10;
   unsigned first=c.gpr[3];c.gpr[3]=0x803c19c0;c.gpr[4]=other;c.gpr[5]=f.global_selector;c.gpr[6]=0;c.gpr[7]=ram[0x17f8000];if(!call(m,&c,0x80062e2c))return 10;
   if(first!=choice.key||second!=(uint8_t)c.gpr[3])return 10;
   unsigned group=0x4a;for(unsigned stage=0;stage<3;++stage){c.gpr[3]=1;c.gpr[4]=group;c.gpr[5]=0x817f1000;if(!call(m,&c,0x80175af8))return 10;
    if(stage<2){memset(ram+0x17f0000,255,4);ram[0x17f0000]=stage?second:first;c.gpr[3]=0x817f1000;c.gpr[4]=0x817f0000;if(!call(m,&c,0x800211d8))return 10;group=((unsigned)ram[0x17f1006+c.gpr[3]*8]<<8|ram[0x17f1007+c.gpr[3]*8])&0x7fff;}}
   memset(ram+0x17f2000,0,64);ram[0x17f203c]=first;ram[0x17f203d]=second;c.gpr[3]=0x817f1000;c.gpr[4]=0x817f2000;c.gpr[5]=0x803c19c0;if(!call(m,&c,0x80062340)||memcmp(data,ram+0x17f2000,64))return 11;++joins;
  }
 }
 nflstreet_variant_facts f={0};f.kind_byte=1;nflstreet_variant_second_facts s={0};uint8_t sentinel=91;
 if(nflstreet_variant_second(&f,&s,&sentinel)||sentinel!=91)return 12;
 nflstreet_request_facts r={0};r.speed=NAN;unsigned sentinel_request=91;if(nflstreet_variant_request(&r,&sentinel_request)||sentinel_request!=91)return 12;
 printf("%u bounded ordinary-phase controls exact; %u original request predicates exact; %u complete second-key decisions exact; %u joined calculated-key curve payloads exact; missing/nonfinite rollback passed\n",phase_cases,requests,seconds,joins);
 free(seed);free(ram);FreeLibrary(h);return 0;
}
