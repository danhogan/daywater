/* Controlled joined four-channel transition oracle, not full gameplay. */
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
int main(int argc,char**argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;size_t rn;uint8_t*ram=read(argv[2],&rn);if(!ram||rn!=24*1024*1024)return 5;
 uint8_t*seed=malloc(rn);if(!seed)return 5;memcpy(seed,ram,rn);unsigned reg=u32(ram+0x3a4ad4),registered=u32(ram+0x3a4ad0);
 nflstreet_asset_store store={0x80000000,ram,ram+(reg-0x80000000),rn,registered*28};
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=rn;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 unsigned checks=0,joins=0;uint8_t*actor=ram+0x3c19c0;
 for(unsigned status=0;status<256;++status)for(unsigned same=0;same<2;++same)for(unsigned variant=0;variant<6;++variant){
  memcpy(ram,seed,rn);actor[0x21f]=status;actor[0xb65]=variant;
  nflstreet_variant_facts facts={0};facts.kind_byte=actor[3];facts.side_byte=actor[2];facts.status_byte=status;facts.variant_byte=variant;
  facts.global_selector=actor[2]+!same;facts.selector_word=u32(actor+0x408);facts.callback_tag=ram[u32(actor+0x310)-0x80000000];
  /* Read the actual subordinate result, then restore before the composite
     original call. Claims cover decision composition, not native services. */
  c.gpr[3]=0x803c19c0;c.gpr[4]=1;if(!call(m,&c,0x80062834))return 6;facts.predicate=c.gpr[3];facts.available|=1;
  memcpy(ram,seed,rn);actor[0x21f]=status;actor[0xb65]=variant;c.gpr[3]=0x803c19c0;if(!call(m,&c,0x8006510c))return 6;
  facts.special_predicate=c.gpr[3];facts.available|=2;
  memcpy(ram,seed,rn);actor[0x21f]=status;actor[0xb65]=variant;c.gpr[3]=0x803c19c0;if(!call(m,&c,0x8004d838))return 6;
  facts.special_word=c.gpr[3];facts.available|=4;
  c.gpr[3]=0x803c19c0;if(!call(m,&c,0x8004da10))return 6;facts.special_enabled=c.gpr[3];facts.available|=8;
  memcpy(ram,seed,rn);actor[0x21f]=status;actor[0xb65]=variant;ram[0x17f8000]=91;
  c.gpr[3]=0x803c19c0;c.gpr[4]=facts.global_selector;c.gpr[5]=1;c.gpr[6]=0x817f8000;
  if(!call(m,&c,0x80062c30))return 7;nflstreet_variant_choice choice;
  if(!nflstreet_variant_first(&facts,91,&choice)||choice.key!=c.gpr[3]||choice.flag!=ram[0x17f8000]){fprintf(stderr,"First key mismatch status=%u same=%u variant=%u key=%u/%u flag=%u/%u\n",status,same,variant,choice.key,c.gpr[3],choice.flag,ram[0x17f8000]);return 8;}++checks;
  if(status<8&&same){uint8_t data[64]={0};nflstreet_variant_choice joined;
   if(!nflstreet_variant_resolve(&store,1,&facts,19,91,data,&joined))return 9;
   unsigned group=0x4a;for(unsigned stage=0;stage<3;++stage){c.gpr[3]=1;c.gpr[4]=group;c.gpr[5]=0x817f1000;
    if(!call(m,&c,0x80175af8))return 10;
    if(stage<2){memset(ram+0x17f0000,255,4);ram[0x17f0000]=stage?19:choice.key;c.gpr[3]=0x817f1000;c.gpr[4]=0x817f0000;
     if(!call(m,&c,0x800211d8))return 10;group=((unsigned)ram[0x17f1006+c.gpr[3]*8]<<8|ram[0x17f1007+c.gpr[3]*8])&0x7fff;}
   }
   memset(ram+0x17f2000,0,64);ram[0x17f203c]=choice.key;ram[0x17f203d]=19;c.gpr[3]=0x817f1000;c.gpr[4]=0x817f2000;c.gpr[5]=0x803c19c0;
   if(!call(m,&c,0x80062340)||memcmp(ram+0x17f2000,data,64))return 11;++joins;
  }
 }

 for(unsigned probe=0;probe<6;++probe){
  memcpy(ram,seed,rn);actor[0xb65]=2;
  if(probe==0)actor[3]=0;
  if(probe==2){actor[0x40b]=4;actor[0x41b]=1;}
  if(probe==3)ram[u32(actor+0x310)-0x80000000]=18;
  if(probe>=4){actor[0x21f]=probe==4?22:23;actor[0x4cb]=probe==4?3:4;actor[0x4d3]=probe==4?0:1;}
  uint8_t saved_actor[0x14d4];memcpy(saved_actor,actor,sizeof saved_actor);
  nflstreet_variant_facts fact={0};fact.kind_byte=actor[3];fact.side_byte=actor[2];fact.status_byte=actor[0x21f];fact.variant_byte=actor[0xb65];fact.global_selector=actor[2]+(probe==1);fact.selector_word=u32(actor+0x408);fact.callback_tag=ram[u32(actor+0x310)-0x80000000];
  if(probe==1){actor[0x40b]=4;fact.selector_word=4;memcpy(saved_actor,actor,sizeof saved_actor);}
  c.gpr[3]=0x803c19c0;c.gpr[4]=1;if(!call(m,&c,0x80062834))return 13;fact.predicate=c.gpr[3];
  memcpy(actor,saved_actor,sizeof saved_actor);c.gpr[3]=0x803c19c0;if(!call(m,&c,0x8006510c))return 13;fact.special_predicate=c.gpr[3];
  fact.special_word=u32(saved_actor+0x4c8);fact.special_enabled=saved_actor[0x4d3]!=0;fact.available=15;
  memcpy(actor,saved_actor,sizeof saved_actor);ram[0x17f8000]=91;c.gpr[3]=0x803c19c0;c.gpr[4]=fact.global_selector;c.gpr[5]=1;c.gpr[6]=0x817f8000;
  if(!call(m,&c,0x80062c30))return 13;nflstreet_variant_choice choice;
  if(!nflstreet_variant_first(&fact,91,&choice)||choice.key!=c.gpr[3]||choice.flag!=ram[0x17f8000])return 14;++checks;
  printf("Controlled first-key branch%u: key=%u flag=%u\n",probe,choice.key,choice.flag);
 }
 nflstreet_variant_facts f={0};f.kind_byte=1;f.side_byte=f.global_selector=1;f.variant_byte=2;nflstreet_variant_choice unchanged={123,91},out=unchanged;
 if(nflstreet_variant_first(&f,91,&out)||memcmp(&out,&unchanged,sizeof out))return 12;
 f.kind_byte=0;if(!nflstreet_variant_first(&f,91,&out)||out.key!=2||out.flag!=91)return 12;
 f.kind_byte=1;f.side_byte=0;f.selector_word=4;if(!nflstreet_variant_first(&f,91,&out)||out.key!=8||out.flag!=91)return 12;
 uint8_t data[64],before[64];memset(data,0x5a,64);memcpy(before,data,64);nflstreet_asset_store missing={0};out=unchanged;
 if(nflstreet_variant_resolve(&missing,1,&f,19,91,data,&out)||memcmp(data,before,64)||memcmp(&out,&unchanged,sizeof out))return 15;
 printf("%u full first-key/flag original decisions exact; %u joined source hierarchy payloads exact; all status bytes covered; unavailable-service rollback passed\n",checks,joins);
 free(seed);free(ram);FreeLibrary(h);return 0;
}
