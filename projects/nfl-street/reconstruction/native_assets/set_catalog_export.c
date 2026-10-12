/* Checked source catalog export; no game data is embedded in public code. */
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
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();
 size_t rn;uint8_t*ram=read(argv[2],&rn);if(!ram||rn!=24*1024*1024||m->cpu_state_size!=sizeof(CPUState))return 4;
 unsigned reg=u32(ram+0x3a4ad4),registered=u32(ram+0x3a4ad0);nflstreet_asset_store store={0x80000000,ram,ram+(reg-0x80000000),rn,registered*28};
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=rn;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 FILE*out=fopen(argv[3],"wb");if(!out)return 5;uint8_t header[8]={'N','S','C','1',0,0,0,2};if(fwrite(header,1,8,out)!=8)return 5;
 for(unsigned set=0;set<2;++set){uint8_t*actor=ram+0x3c19c0;actor[0xb65]=set?0:2;
  nflstreet_variant_facts f={0};f.kind_byte=actor[3];f.side_byte=actor[2];f.status_byte=actor[0x21f];f.variant_byte=actor[0xb65];f.callback_tag=ram[u32(actor+0x310)-0x80000000];f.global_selector=actor[2];f.selector_word=u32(actor+0x408);f.available=15;f.special_word=u32(actor+0x4c8);f.special_enabled=actor[0x4d3]!=0;
  nflstreet_request_facts request={0};request.selector_word=f.selector_word;request.request_state=u32(actor+0x418);request.speed=real(actor+0x1c4);if(!nflstreet_variant_request(&request,&f.predicate))return 6;
  c.gpr[3]=0x803c19c0;if(!call(m,&c,0x8006510c))return 6;f.special_predicate=c.gpr[3];
  unsigned root=u32(ram+0x3a29c8)-0x80000000;nflstreet_phase_facts phase={0};phase.comparison=real(actor+0x1dc);phase.speed=request.speed;phase.heading=u32(actor+0x1c0);phase.same_actor=1;phase.callback_tag=f.callback_tag;phase.global_flags_available=1;phase.global_flags=u32(ram+root+0x34);
  nflstreet_variant_second_facts facts={0};facts.same_actor=1;facts.available=15;facts.aux_byte=actor[0xb5e];facts.side_flag=actor[0xb5d];facts.request_state=request.request_state;
  if(!nflstreet_variant_phase_simple(&phase,&facts.phase_predicate)||!nflstreet_variant_additional_simple(f.callback_tag,&facts.additional_predicate))return 6;
  if(!call(m,&c,0x80104238))return 6;facts.ending_predicate=c.gpr[3];
  uint8_t data[64]={0},second;nflstreet_variant_choice choice;if(!nflstreet_variant_resolve_pair(&store,1,&f,&facts,2,data,&choice,&second))return 7;
  ram[0x17f8000]=2;c.gpr[3]=0x803c19c0;c.gpr[4]=actor[2];c.gpr[5]=0;c.gpr[6]=0x817f8000;if(!call(m,&c,0x80062c30)||choice.key!=c.gpr[3])return 8;
  c.gpr[3]=0x803c19c0;c.gpr[4]=0x803c19c0;c.gpr[5]=actor[2];c.gpr[6]=0;c.gpr[7]=ram[0x17f8000];if(!call(m,&c,0x80062e2c)||second!=(uint8_t)c.gpr[3])return 8;
  c.gpr[3]=1;c.gpr[4]=0x4a;c.gpr[5]=0x817f1000;if(!call(m,&c,0x80175af8))return 8;
  memset(ram+0x17f2000,0,64);ram[0x17f203c]=choice.key;ram[0x17f203d]=second;
  c.gpr[3]=0x817f1000;c.gpr[4]=0x817f2000;c.gpr[5]=0x803c19c0;c.gpr[6]=0;
  if(!call(m,&c,0x80062594)||memcmp(data,ram+0x17f2000,64))return 9;
  uint8_t profile[5000]={'N','L','C','2',0,1,0,0,0,0,0,3};profile[6]=choice.key;profile[7]=second;size_t at=12;
  for(unsigned i=0;i<3;++i){unsigned id=u32(data+16+i*4);const uint8_t*curve=nflstreet_asset_read(&store,id,8);if(!curve)return 10;
   unsigned n=24+u32(curve+4)*12;curve=nflstreet_asset_read(&store,id,n);if(!curve||n>1600||at+12+n>sizeof profile)return 10;
   profile[at]=data[0x37+i];put(profile+at+4,n);put(profile+at+8,id);memcpy(profile+at+12,curve,n);at+=12+n;
  }
  uint8_t length[4];put(length,(unsigned)at);if(fwrite(length,1,4,out)!=4||fwrite(profile,1,at,out)!=at)return 11;
  printf("Verified set%u: calculated keys=%02x/%02x, curves=%08x/%08x/%08x, complete62594 payload exact\n",set,choice.key,second,u32(data+16),u32(data+20),u32(data+24));
 }
 fclose(out);free(ram);FreeLibrary(h);return 0;
}
