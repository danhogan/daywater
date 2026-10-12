/* Actual title producer -> source background recipient -> native fade updates.
   Properties9/8 execute; archive readiness/busy and resource I/O supplied. Supplied loaded
   set/archive state is explicitly controlled, not fresh UI startup. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "broadcast.h"
#include "background.h"
#include "screen.h"
#include "title.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int run(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_ui_title*requests,nflstreet_ui_background*resources,unsigned fn){c->pc=fn;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x801d4b3c){if(c->gpr[6]!=9||requests->broadcasts>=32||c->gpr[8]>16)return 0;nflstreet_ui_service_request*q=requests->requests+requests->broadcasts++;memset(q,0,sizeof *q);q->kind=2;q->property=be(c->ram+c->gpr[7]-0x80000000);q->argc=c->gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(c->ram+c->gpr[9]-0x80000000+j*4);c->pc=c->lr;}else if(c->pc==0x801ee1c0){c->fpr[1]=resources->random_sample;c->pc=c->lr;}
 else if(c->pc==0x801d5918||c->pc==0x801d54a4){if(resources->effect_count>=512)return 0;resources->effects[resources->effect_count++]=(nflstreet_ui_resource_effect){c->pc==0x801d54a4,c->gpr[4],c->gpr[5]};c->pc=c->lr;}
 else if(c->pc==0x801878b0||c->pc==0x801af944||c->pc==0x801af9cc||c->pc==0x801af57c){c->pc=c->lr;}
 else m->dispatch(c,c->pc);if(c->exception)break;}
 if(c->pc||c->exception)fprintf(stderr,"Original flow stopped pc=%x exception=%x\n",c->pc,c->exception);return !c->pc&&!c->exception;
}
static int compare_rates(uint8_t*ram,const nflstreet_ui_rates*title,const nflstreet_ui_rates*background){unsigned count=be(ram+0x17f0054),t=0,b=0;if(count!=title->count+background->count)return 0;
 for(unsigned i=0;i<count;++i){uint8_t*r=ram+0x17e0000+i*52;unsigned screen=be(r+28);const unsigned*want;if(screen==0x817f1000){if(t>=title->count)return 0;want=title->records[t++].words;}else if(screen==0x817f1014){if(b>=background->count)return 0;want=background->records[b++].words;}else return 0;
  for(unsigned j=0;j<13;++j){unsigned v=want[j];if(j==7)v=screen;if(v!=be(r+j*4)){fprintf(stderr,"Rate flow mismatch record=%u word=%u native=%x original=%x\n",i,j,v,be(r+j*4));return 0;}}
 }return t==title->count&&b==background->count;
}
static int read(const char*path,uint8_t**b,size_t*n){FILE*f=fopen(path,"rb");if(!f)return 0;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size<32||size>1000000){fclose(f);return 0;}*n=(size_t)size;*b=malloc(*n);int ok=*b&&fread(*b,1,*n,f)==*n;fclose(f);return ok;}
int main(int argc,char**argv){if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);uint8_t*title,*background;size_t tn,bn;if(!read(argv[3],&title,&tn)||!read(argv[4],&background,&bn))return 4;unsigned result;if(!nflstreet_ui_relocate(title,tn,0x81100000,&result)||!nflstreet_ui_relocate(background,bn,0x81200000,&result))return 5;
 unsigned frames=0,dispatched=0;
 for(unsigned seed=0;seed<4;++seed){uint8_t*t=malloc(tn),*b=malloc(bn);if(!t||!b)return 4;memcpy(t,title,tn);memcpy(b,background,bn);
  /* Root and child activation execute below through actual source scripts.
     Class resource initialization/loaded-set production are not claimed. */
  memcpy(ram+0x1100000,t,tn);memcpy(ram+0x1200000,b,bn);memset(ram+0x17f0000,0,0x200);memset(ram+0x17f1000,0,40);memset(ram+0x17e0000,0,64*52);memset(ram+0x17c0000,0,1024);
  put(ram+0x17f0008,16);put(ram+0x17f000c,0x8012a414);put(ram+0x17f0058,0x817e0000);put(ram+0x17f0088,0x817c0000+16);put(ram+0x17f008c,0x81100220);put(ram+0x17f1004,3);put(ram+0x17f1010,0x81100000);put(ram+0x17f1024,0x81200000);put(ram+0x17c0004,0xfffe);put(ram+0x17c0008,0x81100098);
  nflstreet_ui_title state={0},requests={0};state.rates.clock=16;nflstreet_ui_background background_state={0},original_resources={0};background_state.rates.clock=16;background_state.random_sample=(float)seed/4;unsigned executed=0;
  memset(background_state.ready,seed&1?0:1,22);memset(background_state.busy,seed==3?1:0,22);original_resources=background_state;
  memset(ram+0x17b0000,0,0x400);put(ram+0x3a9a40-0x63dc,0x817b0000);put(ram+0x17b000c,0x817b0040);put(ram+0x17b0040+8*0x24+0x10,0x10);put(ram+0x17b0040+8*0x24+0x14,0x817b0200);ram[0x17b0215]=22;put(ram+0x17b0278,0x817b0300);memcpy(ram+0x17b0300,background_state.ready,22);for(unsigned k=0;k<22;++k)ram[0x17b0300+k]=(background_state.ready[k]?2:0)|(background_state.busy[k]?0x10:0);ram[0x3a9a40-0x6538]=1;put(ram+0x17f004c,0x817b0380);put(ram+0x17b0380,0x801d60dc);
  CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f007c;c.gpr[6]=0x817f1000;c.gpr[7]=0x81100098;
  if(!nflstreet_ui_background_activate(b,bn,0x81200000,&background_state))return 13;
  const unsigned activation_offsets[]={0x59c,0xa02},activation_roots[]={0x324,0x19c};
  for(unsigned ai=0;ai<2;++ai){put(ram+0x17f0088,0x817c0010);put(ram+0x17f008c,0x81200000+activation_offsets[ai]);put(ram+0x17c0000,0);put(ram+0x17c0004,0xfffe);put(ram+0x17c0008,0x81200000+activation_roots[ai]);put(ram+0x17c000c,0);c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f007c;c.gpr[6]=0x817f1014;c.gpr[7]=0x81200000+activation_roots[ai];if(!run(m,&c,&requests,&original_resources,0x801d16a0))return 13;}
  if(memcmp(b,ram+0x1200000,bn))return 13;
  put(ram+0x17f0088,0x817c0010);put(ram+0x17f008c,0x81100220);put(ram+0x17c0000,0);put(ram+0x17c0004,0xfffe);put(ram+0x17c0008,0x81100098);put(ram+0x17c000c,0);c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f007c;c.gpr[6]=0x817f1000;c.gpr[7]=0x81100098;
  if(!nflstreet_ui_title_activate(t,tn,0x81100000,&state)||!run(m,&c,&requests,&original_resources,0x801d16a0)||state.broadcasts!=3||memcmp(state.requests,requests.requests,3*sizeof *state.requests))return 6;
  for(unsigned qi=0;qi<state.broadcasts;++qi){const nflstreet_ui_service_request*q=state.requests+qi;
  if(q->property!=9&&q->property!=8)continue;
  if(!nflstreet_ui_broadcast(b,bn,0x81200000,&background_state.entry_flags,(uint16_t)q->property,q->argc,q->args,nflstreet_ui_background_service,&background_state,sizeof background_state,&executed)||executed!=1)return 7;
  put(ram+0x17f0088,0x817c0000);for(unsigned i=0;i<q->argc;++i)put(ram+0x17d0000+i*4,q->args[i]);c.gpr[3]=0x817f0000;c.gpr[4]=0x817f1014;c.gpr[5]=0x817f007c;c.gpr[6]=0;c.gpr[7]=q->property;c.gpr[8]=q->argc;c.gpr[9]=0x817d0000;
  if(!run(m,&c,&requests,&original_resources,0x801d4858)||memcmp(b,ram+0x1200000,bn)||!compare_rates(ram,&state.rates,&background_state.rates)){for(unsigned off=0;off<bn;++off)if(b[off]!=ram[0x1200000+off]){fprintf(stderr,"screen diff offset=%x native=%x original=%x\n",off,b[off],ram[0x1200000+off]);break;}fprintf(stderr,"dispatch failure property=%x seed=%u\n",q->property,seed);return 8;}++dispatched;
  }
  for(unsigned j=0;j<1600;++j){if(seed==1&&j==100){memset(background_state.ready,1,22);for(unsigned k=0;k<22;++k)ram[0x17b0300+k]=2;}unsigned elapsed=seed==0?16:seed==1?8:seed==2?32:48;if(!nflstreet_ui_title_advance(t,tn,0x81100000,&state,elapsed)||!nflstreet_ui_background_advance(b,bn,0x81200000,&background_state,elapsed)){fprintf(stderr,"native advance failed seed=%u step=%u rates=%u effects=%u\n",seed,j,background_state.rates.count,background_state.effect_count);return 9;}c.gpr[3]=0x817f0000;c.gpr[4]=elapsed;
   if(!run(m,&c,&requests,&original_resources,0x801cf610)||memcmp(t,ram+0x1100000,tn)||memcmp(b,ram+0x1200000,bn)||!compare_rates(ram,&state.rates,&background_state.rates)||state.entry_flags!=be(ram+0x17f1000)||background_state.entry_flags!=be(ram+0x17f1014)){fprintf(stderr,"Title/background flow mismatch seed=%u frame=%u\n",seed,j);return 10;}++frames;}
  if(background_state.effect_count!=original_resources.effect_count||memcmp(background_state.effects,original_resources.effects,background_state.effect_count*sizeof *background_state.effects))return 12;
  free(t);free(b);
 }
 unsigned guards=0;
 for(unsigned mode=0;mode<2;++mode){uint8_t*bad=malloc(bn),*before=malloc(bn);if(!bad||!before)return 4;memcpy(bad,background,bn);
  if(mode==0)bad[0xa7f]=0xff;else{unsigned table=be(bad+4)-0x81200000,list=be(bad+table+16)-0x81200000;put(bad+list+4,0);}
  memcpy(before,bad,bn);nflstreet_ui_background guarded={0},old;guarded.rates.clock=16;old=guarded;unsigned flags=0x12345678,executed=99,arg=1;
  if(nflstreet_ui_broadcast(bad,bn,0x81200000,&flags,9,1,&arg,nflstreet_ui_background_service,&guarded,sizeof guarded,&executed)||memcmp(bad,before,bn)||memcmp(&guarded,&old,sizeof old)||flags!=0x12345678||executed!=99)return 11;++guards;free(bad);free(before);}
 printf("Atomic broadcast rollback: %u late-unsupported/cyclic-node guards\n",guards);
 printf("Actual title properties9/8 -> background script execution exact: %u dispatches, %u complete two-screen/scheduler updates; original VM recipient executes; property6, loaded-set production and full navigation remain unfinished\n",dispatched,frames);free(title);free(background);free(ram);FreeLibrary(h);return 0;}
