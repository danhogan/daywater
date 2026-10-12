#include <windows.h>
#include "moderngekko/module_abi.h"
#include "controller_select.h"
#include "controller_callback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint32_t rng=0x19ac1234;
static uint32_t draw(void){rng=rng*1664525+1013904223;return rng;}
static nflstreet_controller_commit captured;
static uint32_t supplied_handles[4];
static int run(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t fn,uint32_t port,uint32_t arg,nflstreet_controller_select*s,const int8_t map[4],uint32_t*out){
 for(unsigned i=0;i<4;++i){put(ram+0x282590+4*i,s->slots[i]);put(ram+0x2f506c+4*i,s->sides[i]);ram[0x3a9a40-0x6e60+i]=(uint8_t)s->mapping[i];}
 memset(ram+0x17e0000,0x5a,20);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=port;c.gpr[4]=arg;c.gpr[5]=0x817e0000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.pc=fn;c.cycle_budget=10000000;
 if(fn==0x8011bc68){c.gpr[4]=0x817e0100;c.gpr[6]=0x817e0200;}
 for(unsigned step=0;c.pc&&step<2000;++step){c.downcount=-1000000000;
  if(c.pc==0x80009e5c||c.pc==0x80009e6c){c.gpr[3]=0;c.pc=c.lr;}
  else if(c.pc==0x80127798){if(c.gpr[4]!=4)return -1;memcpy(ram+c.gpr[3]-0x80000000,map,4);c.pc=c.lr;}
  else if(c.pc==0x80127394){c.gpr[3]=s->profile_count;c.pc=c.lr;}
  else if(c.pc==0x801273b4){if(c.gpr[3]>=4)return -1;memcpy(ram+c.gpr[4]-0x80000000,s->profiles[c.gpr[3]],strlen(s->profiles[c.gpr[3]])+1);c.pc=c.lr;}
  else if(c.pc==0x80129acc){if(c.gpr[3]>=4)return -1;c.gpr[3]=supplied_handles[c.gpr[3]];c.pc=c.lr;}
  else if(c.pc==0x801e02fc||c.pc==0x801e02b4||c.pc==0x8014c20c||c.pc==0x8013da04){if(captured.count>=7)return -1;nflstreet_controller_effect*q=&captured.effects[captured.count++];q->function=c.pc;q->argc=c.pc==0x801e02fc?0:c.pc==0x8013da04?3:2;for(unsigned i=0;i<q->argc;++i)q->args[i]=c.gpr[3+i];c.pc=c.lr;}
  else m->dispatch(&c,c.pc);
  if(c.exception)return -1;
 }*out=c.gpr[3];return c.pc?0:1;
}
static int compare_state(const uint8_t*ram,const nflstreet_controller_select*s){
 for(unsigned i=0;i<4;++i)if(s->slots[i]!=be(ram+0x282590+4*i)||s->sides[i]!=be(ram+0x2f506c+4*i)||s->mapping[i]!=(int8_t)ram[0x3a9a40-0x6e60+i])return 0;return 1;
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 unsigned cycles=0,guards=0,sets=0,gets=0,remaps=0,commits=0,callbacks=0;const int32_t directions[]={-1,0,1,INT32_MIN,INT32_MAX};
 for(unsigned test=0;test<5000;++test){nflstreet_controller_select s={0};s.profile_count=draw()%5;
  int8_t map[4];for(unsigned i=0;i<4;++i){s.slots[i]=draw()%7;s.sides[i]=draw()%3;if(s.sides[i]==2)s.sides[i]=UINT32_MAX;s.mapping[i]=(int8_t)(draw()%5)-1;map[i]=(int8_t)(draw()%5)-1;snprintf(s.profiles[i],20,"Supplied profile %u",i);}
  for(unsigned i=0;i<3;++i){const char*p=(char*)ram+0x256de0+(i+4)*10+12;size_t n=0;while(n<20&&p[n])++n;if(n==20)return 5;memcpy(s.special[i],p,n+1);}
  unsigned port=test%6;int32_t dir=directions[test%5];nflstreet_controller_select before=s,native=s;char name[20];memset(name,0x5a,20);int ok=nflstreet_controller_cycle(&native,port,dir,name);uint32_t result;int original=run(m,ram,0x8011b87c,port,(uint32_t)dir,&s,map,&result);
  if(!ok){if(memcmp(&before,&native,sizeof before)||original!=0){fprintf(stderr,"Cycle guard differs test%u original%d\n",test,original);return 6;}++guards;}
  else{if(original!=1||!compare_state(ram,&native)||memcmp(name,ram+0x17e0000,20)){fprintf(stderr,"Cycle differs test%u port%u direction%d original%d\n",test,port,dir,original);for(unsigned i=0;i<4;++i)fprintf(stderr," %u slot%u/%u side%x map%d name%s/%s\n",i,native.slots[i],be(ram+0x282590+4*i),native.sides[i],native.mapping[i],name,ram+0x17e0000);return 7;}++cycles;}
  native=s;if(!nflstreet_controller_remap(&native,map)||run(m,ram,0x8011b368,0,0,&s,map,&result)!=1||!compare_state(ram,&native))return 8;++remaps;
  native=s;unsigned value=test%10==9?UINT32_MAX:test%10;if(!nflstreet_controller_set(&native,map,port,value)||run(m,ram,0x8011bb74,port,value,&s,map,&result)!=1||!compare_state(ram,&native))return 9;++sets;
  int32_t value_out;if(!nflstreet_controller_get(&s,port,&value_out)||run(m,ram,0x8011bc10,port,0,&s,map,&result)!=1||(uint32_t)value_out!=result||!compare_state(ram,&s))return 10;++gets;
  for(unsigned i=0;i<4;++i)supplied_handles[i]=(test&(1u<<i))?0x12340000+i:UINT32_MAX;
  nflstreet_controller_commit wanted;if(!nflstreet_controller_confirm(&s,supplied_handles,test,&wanted))return 12;memset(&captured,0,sizeof captured);
  if(run(m,ram,0x8011b728,test,0,&s,map,&result)!=1||!compare_state(ram,&s))return 12;captured.choice=be(ram+0x3a9a40-0x6e5c);if(memcmp(&wanted,&captured,sizeof wanted))return 13;++commits;
  if(test<500){const uint32_t properties[]={0x80000002,0x80000003,0x80000004,0x80000005,0x80000007,0x80000009,0x8000000c,0x8000000d};
   for(unsigned k=0;k<8;++k){nflstreet_controller_callback cb={0};cb.selection=s;memcpy(cb.next_mapping,map,4);memcpy(cb.handles,supplied_handles,16);uint8_t bytes[1024];memset(bytes,0x5a,sizeof bytes);put(bytes+0x40,1);put(bytes+0x44,4);put(bytes+0x84,19);put(bytes+0x88,0x817e0000);uint32_t stack[256]={0},out=0xaabbccdd;
    nflstreet_ui_service_request q={0};q.kind=1;q.property=properties[k];q.argc=k==4?1:k==6?0:k==3?3:k==0||k==1?1:2;q.args[0]=k==1||k==2?0x817e0040:k==0?test:port;q.args[1]=k==2?0x817e0044:k==3?(uint32_t)dir:k==7?test%3:value;q.args[2]=0x817e0080;if(k==7)q.args[0]=port%4;
    for(unsigned i=0;i<q.argc;++i)put(bytes+0x100+i*4,q.args[i]);put(bytes+0x200,out);memcpy(ram+0x17e0000,bytes,sizeof bytes);memset(&captured,0,sizeof captured);
    int accepted=nflstreet_controller_callback_memory(&cb,bytes,sizeof bytes,0x817e0000,&q,&out,0x817c0000,stack);int finished=run(m,ram,0x8011bc68,q.property,0,&s,map,&result);
    if(!accepted){if(finished!=0){fprintf(stderr,"Callback unexpectedly rejected test%u property%x\n",test,q.property);return 14;}continue;}
    put(bytes+0x200,out);if(finished!=1||result!=1||!compare_state(ram,&cb.selection)||memcmp(bytes,ram+0x17e0000,sizeof bytes)){fprintf(stderr,"Callback differs test%u property%x finished%d\n",test,q.property,finished);return 15;}
    if(k==0){captured.choice=be(ram+0x3a9a40-0x6e5c);if(memcmp(&cb.committed,&captured,sizeof captured))return 16;}++callbacks;
   }
  }
 }
 nflstreet_controller_select invalid={0},copy;invalid.slots[1]=7;copy=invalid;char name[20];memset(name,0x5a,20);if(nflstreet_controller_cycle(&invalid,0,1,name)||memcmp(&invalid,&copy,sizeof copy)||name[0]!=0x5a)return 11;
 nflstreet_controller_callback cb={0},prior;cb.selection.slots[0]=4;for(unsigned i=0;i<4;++i)cb.selection.slots[i]=4;strcpy(cb.selection.special[0],"Supplied empty");prior=cb;
 uint8_t bytes[64]={0},saved[64];uint32_t stack[256]={0},out=0x12345678;put(bytes+4,1);put(bytes+8,0x81100020);memcpy(saved,bytes,sizeof saved);
 nflstreet_ui_service_request bad={0};bad.kind=1;bad.property=0x80000005;bad.argc=3;bad.args[2]=0x81100000;
 if(nflstreet_controller_callback_memory(&cb,bytes,sizeof bytes,0x81100000,&bad,&out,0x817c0000,stack)||memcmp(&cb,&prior,sizeof cb)||memcmp(bytes,saved,sizeof bytes)||out!=0x12345678)return 17;
 bad.property=0x80000004;bad.argc=2;bad.args[0]=0x817c0000;bad.args[1]=0x817c0400;
 if(nflstreet_controller_callback_memory(&cb,bytes,sizeof bytes,0x81100000,&bad,&out,0x817c0000,stack)||stack[0]||memcmp(bytes,saved,sizeof bytes))return 17;
 bad.args[1]=0x817c0004;if(!nflstreet_controller_callback_memory(&cb,bytes,sizeof bytes,0x81100000,&bad,&out,0x817c0000,stack)||stack[0]!=99||stack[1]!=100)return 17;
 bad.property=0x8000000e;bad.argc=1;if(nflstreet_controller_callback_memory(&cb,bytes,sizeof bytes,0x81100000,&bad,&out,0x817c0000,stack)||memcmp(&cb,&prior,sizeof cb))return 17;
 printf("Original local controller selection exact:%u cycles,%u remaps,%u assignments,%u reads,%u commits with ordered effects,%u actual screen callback comparisons; %u source nontermination cases reject atomically. Profile service inputs and logical handles supplied; binding/profile/audio requests captured; no hardware discovery, network branch or source UI events claimed.\n",cycles,remaps,sets,gets,commits,callbacks,guards);free(ram);FreeLibrary(h);return 0;
}
