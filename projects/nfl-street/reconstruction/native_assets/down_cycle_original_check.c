/* GNNE69 r0 down cycle vs UNHOOKED original. Candidate pending external gate. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "down_cycle.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define LO NFLSTREET_AI_POLICY_STACK_LO
#define HI NFLSTREET_AI_POLICY_STACK_HI
#define CASES 401u
static const ModernGekkoModuleDesc *module;
static uint8_t *native,*oracle,*saved;
static nflstreet_down_cycle policy;
static uint32_t seed=0x646f776eu;
static unsigned case_index,side,stage;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint8_t *span(void *p,uint32_t a,uint32_t n){return a>=BASE&&(uint64_t)a-BASE+n<=BYTES?(uint8_t *)p+a-BASE:NULL;}
static uint32_t r32(void *p,uint32_t a){const uint8_t *q=span(p,a,4);if(!q){fprintf(stderr,"bad fixture %08x\n",a);exit(4);}return(uint32_t)q[0]<<24|(uint32_t)q[1]<<16|(uint32_t)q[2]<<8|q[3];}
static unsigned r8(void *p,uint32_t a){const uint8_t *q=span(p,a,1);if(!q)exit(4);return *q;}
static void w32(void *p,uint32_t a,uint32_t v){uint8_t *q=span(p,a,4);if(!q)exit(4);q[0]=v>>24;q[1]=v>>16;q[2]=v>>8;q[3]=v;}
static void w16(void *p,uint32_t a,unsigned v){uint8_t *q=span(p,a,2);if(!q)exit(4);q[0]=v>>8;q[1]=v;}
static void wf(void *p,uint32_t a,float f){uint32_t v;memcpy(&v,&f,4);w32(p,a,v);}
/* CPU instruction boundary, not an original-function hook. The unchanged
   oracle chunk delegates MTDEC to ppc_fallback_instruction and returns to
   its dispatcher. Implement only MTSPR SPR_DEC(22),rS: retain the operand
   in call-local CPU state and resume at the following original instruction
   (801EF624's BLR), never at the guest LR. No guest RAM/GPR/CR/LR writes,
   clock advancement, interrupt delivery or native down-cycle call occurs.
   Dolphin Interpreter_SystemRegisters.cpp assigns spr[index]=gpr[RD]; its
   timer/interrupt effects are outside this synchronous host boundary.
   Every other fallback remains an explicit checker failure. */
static void original_cpu_instruction(CPUState *c,uint32_t instruction,uint32_t pc){
 if((instruction&0xfc1fffffu)==0x7c1603a6u&&!(c->msr&0x4000u)){
  *(uint32_t *)c->external_user_data=c->gpr[(instruction>>21)&31u];c->pc=pc+4u;return;}
 fprintf(stderr,"unsupported oracle CPU instruction %08x at %08x\n",instruction,pc);exit(6);}
static int original_raw(uint32_t pc,const uint32_t *args,uint32_t *result){
 uint32_t decrementer=0;CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.instruction_fallback=original_cpu_instruction;c.external_user_data=&decrementer;
 c.fpr[1]=1.0;c.gpr[1]=HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 for(unsigned k=0;k<6;++k)c.gpr[3+k]=args[k];c.pc=pc;
 for(unsigned step=0;c.pc;++step){
  if(step>5000000u){fprintf(stderr,"original timeout %08x at %08x lr %08x\n",pc,c.pc,c.lr);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"original exception %08x at %08x lr %08x\n",pc,c.pc,c.lr);return 0;}}
 *result=c.gpr[3];return 1;}
/* Host execution boundary, also used by nflstreet_ai_policy_service: these
   are actual unmodified SDK calls through the original module, not hooks.
   The host has no Dolphin OS thread to yield. Deferring scheduling through
   the SDK's own nesting counter keeps policy/queue writes and pending work;
   the matching enable call restores the exact previous nesting depth.
   Saved RAM and prepare() inputs are unchanged. Raw SDK tests below compare
   both helpers and the scheduler's original guarded path independently. */
/* Independent oracle-side caller selection, without using ai_policy_host.h.
   The SDK publishes state 2 before its E4 write (801F47B0/47C8). A RAM-only
   capture can therefore have E4 naming READY while another record is RUNNING.
   A host call borrows the sole running identity, or the active-list head
   in idle captures. Restore E4 afterward; no mutex is reset or reassigned. */
static int original_caller(void *ram,uint32_t *selected){
 uint32_t node=r32(ram,0x800000dcu),head=node,running=0,visited[256];unsigned count=0;
 if(!head)return 0;
 while(node){if(count==256)return 0;
  for(unsigned i=0;i<count;++i)if(visited[i]==node)return 0;visited[count++]=node;
  if(!r8(ram,node+0x2c8u)&&r8(ram,node+0x2c9u)==2){if(running)return 0;running=node;}
  node=r32(ram,node+0x2fcu);}
 *selected=running?running:head;return 1;}
static int original_host(uint32_t pc,const uint32_t *args,uint32_t *result){
 const uint32_t zero[6]={0};uint32_t ignored,caller;
 if(!original_caller(oracle,&caller)){fprintf(stderr,"invalid oracle host caller list\n");return 0;}
 const uint32_t previous=r32(oracle,0x800000e4u);w32(oracle,0x800000e4u,caller);
 if(!original_raw(0x801f429cu,zero,&ignored)){w32(oracle,0x800000e4u,previous);return 0;}
 const int ok=original_raw(pc,args,result);
 const int released=original_raw(0x801f42dcu,zero,&ignored);
 w32(oracle,0x800000e4u,previous);return ok&&released;}
static int same(void){const unsigned lo=LO-BASE,hi=HI-BASE;
 if(memcmp(native,oracle,lo)||memcmp(native+hi,oracle+hi,BYTES-hi)){
  for(unsigned k=0;k<BYTES;++k)if((k<lo||k>=hi)&&native[k]!=oracle[k]){
   fprintf(stderr,"RAM mismatch case %u team %u stage %u at %08x original %02x native %02x\n",case_index,side,stage,BASE+k,oracle[k],native[k]);break;}return 0;}return 1;}
static void reset(void){memcpy(native,saved,BYTES);nflstreet_down_cycle_init(&policy,(nflstreet_actor_ai){span,native,R13});
 memcpy(policy.native.stack,saved+LO-BASE,sizeof policy.native.stack);}
static int rejected(void){fprintf(stderr,"native rejected case %u team %u stage %u pc %08x address %08x target %08x budget %d\n",
 case_index,side,stage,policy.native.failure_pc,policy.native.failure_address,policy.native.unsupported,policy.native.failure_budget_exhausted);return 5;}
#define RESULT 0x80f37000u
#define META (RESULT+0x40u)
#define XY (RESULT+0x80u)
static uint32_t field(void *p){return r32(p,R13-0x7078u);}
static uint32_t phase(void *p){return r32(p,r32(p,R13-0x7cf8u));}
static uint32_t ball(void *p){uint32_t pool=r32(p,R13-0x73d0u);return pool+8u+0xf4u*r8(p,pool+1u);}
static uint32_t carrier(void *p){uint32_t ref=r32(p,ball(p)+0xb4u),roster=r32(p,R13-0x7c30u);
 if((ref&255u)!=1)return 0;return r32(p,roster)+0x14d4u*(((ref>>8)&255u)*(r32(p,roster+8u)>>16)+((ref>>16)&255u));}
static int pair(uint32_t pc,uint32_t a,uint32_t b,uint32_t d,int returns){
 const uint32_t args[6]={a,b,d,0,0,0};uint32_t nv,ov;
 if(!nflstreet_down_cycle_call(&policy,pc,args,1,&nv))return rejected();
 if(!original_host(pc,args,&ov))return 6;if(!same())return 7;
 if(returns&&nv!=ov){fprintf(stderr,"return mismatch case %u pc %08x %08x/%08x\n",case_index,pc,nv,ov);return 7;}return 0;}
static int o_call(uint32_t pc,uint32_t a,uint32_t b,uint32_t d){const uint32_t args[6]={a,b,d,0,0,0};uint32_t v;
 return original_host(pc,args,&v);}
/* Independent host schedule around ORIGINAL calls. No native event decision,
   result builder, callback, span, thread helper or football rule is used here. */
static int original_update(void){uint32_t p=phase(oracle);
 if(p==3){const uint32_t zero[6]={0};uint32_t actor;if(!original_host(0x800be3f0u,zero,&actor))return 0;
  return o_call(0x800f9c50u,actor,0,0);}
 if(p==4){uint32_t w=r32(oracle,R13-0x7b98u);
  if(!o_call(0x8004ebf0u,w,0,0))return 0;
  if(!r8(oracle,w+0x100u))return o_call(0x8002e45cu,5,0,0);return 1;}
 return p==2||p==5;}
static unsigned whistles[5],commits,selections,snaps,updates;
static int step(void){nflstreet_down_cycle_event e;uint32_t before=phase(oracle);
 if(!nflstreet_down_cycle_update(&policy,&e))return rejected();if(!original_update())return 6;
 if(!same())return 7;++updates;
 if(e.phase!=phase(oracle)||e.previous_phase!=before||e.whistle!=(before==3&&phase(oracle)==4)||e.select_play!=(phase(oracle)==5)){
  fprintf(stderr,"return mismatch cycle event\n");return 7;}
 if(e.whistle){uint32_t result=r32(oracle,R13-0x7b98u)+0x104u,flags=r32(oracle,result+0x18u);unsigned score=r8(oracle,result+0x1du);
  unsigned k=score==6?2:flags&0x4000u?3:flags&0x20000u?1:0;++whistles[k];}
 if(before==4&&phase(oracle)==5){++commits;++selections;}
 return 0;}
static int next_down(void){
 int status;uint32_t nv,play,actor;const uint32_t team=r8(oracle,field(oracle)+0x38u);
 status=pair(0x8003729cu,0,0,0,0);if(status)return status;
 if(!nflstreet_down_cycle_lineup(&policy))return rejected();
 if(!o_call(0x800f2a20u,0,0,0)||!o_call(0x800f2a20u,1,0,0)||
    !o_call(0x800f8ae8u,0,0,0)||!o_call(0x800f8ae8u,1,0,0)||!o_call(0x8002e45cu,2,0,0))return 6;
 if(!same())return 7;
 const uint32_t args[6]={team,0,0,0,0,0};if(!original_host(0x800f2e7cu,args,&play))return 6;
 const uint32_t lookup[6]={team,r8(oracle,play+0x16u),0,0,0,0};if(!original_host(0x8003a4a0u,lookup,&actor))return 6;
 /* The getters can update cached words. Run them natively before comparing
    the composite snap call, which executes those same getters itself. */
 if(!nflstreet_down_cycle_snap(&policy))return rejected();
 if(!o_call(0x800f946cu,0,0,0)||!o_call(0x800815b4u,actor,0,0))return 6;
 if(!same())return 7;nv=phase(native);if(nv!=3){fprintf(stderr,"return mismatch snap phase %u\n",nv);return 7;}++snaps;return 0;}
static void prepare(unsigned index){
 if(!index)return;
 const uint32_t f=field(native),b=ball(native),roster=r32(native,R13-0x7c30u),base=r32(native,roster);
 /* These are edits of original fields, never fabricated event-log records.
    Attach the active ball using the actor's original four-byte reference. */
 unsigned team=r8(native,f+0x38u),slot=(index/5u)%7u;uint32_t a=base+0x14d4u*(7u*team+slot);
 w32(native,r32(native,R13-0x7cf8u),3);w32(native,f+0x30u,1+index%4u);
 w32(native,f+0x34u,8u);span(native,f+0xa4u,1)[0]=(uint8_t)team;
 w32(native,b+0xb4u,r32(native,a));w32(native,b+0xb8u,r32(native,a));w32(native,b+0xa4u,1);
 w32(native,b+0xa8u,0);w32(native,b+0xacu,1);w32(native,b+0xb0u,0);
 w32(native,b+0xc4u,0);w32(native,b+0xbcu,100);wf(native,b+0x9cu,0);
 w32(native,a+0xcu,r32(native,a+0xcu)&~0x1000800u);wf(native,a+0x1b0u,2.0f);
 float x=(float)((int)(rnd()%2001)-1000)/100.0f,y=(float)((int)(rnd()%4001)-2000)/100.0f;
 const unsigned mode=(index-1u)%5u;
 if(mode==0){x=(index&1u?1.0f:-1.0f)*(40.0f+(float)(rnd()%1000)/100.0f);
  w32(native,b+0xc4u,1);w32(native,b+0xacu,4);}
 if(mode==1){y=45.0f+(float)(rnd()%500)/100.0f;w32(native,a+0xcu,r32(native,a+0xcu)|0x1000000u);}
 if(mode==2){w32(native,b+0xc4u,4);w32(native,b+0xacu,4);wf(native,a+0x1b0u,0);} /* actual contact-mode consumer */
 if(mode==3){w32(native,b+0xb4u,0);w32(native,b+0xa4u,2);w32(native,b+0xc4u,0);}
 if(mode==4)y=-45.0f;
 wf(native,a+0x1a8u,x);wf(native,a+0x1acu,y);
 wf(native,b+0xcu,x);wf(native,b+0x10u,y);wf(native,b+0x14u,0.1f);
 for(unsigned i=0;i<3;++i)wf(native,b+0x54u+4*i,0);
 w16(native,f+0x3cu,rnd()%40);w16(native,f+0x3eu,rnd()%40);
 wf(native,XY,x);wf(native,XY+4u,y);wf(native,XY+8u,0);
}
static uint32_t digest(uint32_t d,const void *p,unsigned n){const uint8_t *b=p;for(unsigned i=0;i<n;++i)d=(d^b[i])*16777619u;return d;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *p=malloc(BYTES);
 int ok=p&&fread(p,1,BYTES,f)==BYTES&&fgetc(f)==EOF;fclose(f);if(!ok){free(p);return NULL;}return p;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 saved=load(argv[1]);native=malloc(BYTES);oracle=malloc(BYTES);if(!saved||!native||!oracle)return 3;
 uint32_t hash=2166136261u;unsigned leaves=0;int status;
 for(case_index=0;case_index<CASES;++case_index){
  reset();prepare(case_index);memcpy(oracle,native,BYTES);stage=0;
  /* The original collision classification, with output distance and all RAM. */
  if(case_index){status=pair(0x80103ea8u,XY,XY+16u,0,1);if(status)return status;++leaves;}
  if(case_index&&(case_index-1u)%5u==3){
   /* Actual incompletion event consumer, followed by the actual whistle.
      No invented input event: FC2C8's argument 1 is C1D1C's own call. */
   stage=1;status=pair(0x800fc2c8u,ball(native),1,0,1);if(status)return status;
   stage=2;status=pair(0x80103cf0u,0,0,0,0);if(status)return status;
   if(phase(oracle)==4){uint32_t result=r32(oracle,R13-0x7b98u)+0x104u;
    if(r32(oracle,result+0x18u)&0x4000u)++whistles[3];}
  }
  for(unsigned tick=0;tick<(case_index?8u:1u);++tick){stage=3+tick;status=step();if(status)return status;}
  hash=digest(hash,native,LO-BASE);hash=digest(hash,native+HI-BASE,BYTES-(HI-BASE));
 }
 /* Persistent sequences: begin from distinct edited carrier/contact/position
    contexts and keep BOTH memories across 96 updates, through the whistle
    queue and the real result commit. Check every update, never just last. */
 for(unsigned sequence=0;sequence<24;++sequence){case_index=CASES+sequence;reset();prepare(1+sequence%3u);memcpy(oracle,native,BYTES);
  for(unsigned tick=0;tick<96;++tick){stage=100+tick;status=step();if(status)return status;
   if(phase(native)==5){status=next_down();if(status)return status;}}
  hash=digest(hash,native,LO-BASE);hash=digest(hash,native+HI-BASE,BYTES-(HI-BASE));}
 /* Independently exercise dirty output destinations and real down/distance
    writers, including line-to-gain thresholds and score mutations. */
 for(case_index=0;case_index<CASES;++case_index){reset();memcpy(oracle,native,BYTES);stage=300;
  if(case_index){uint32_t f=field(native);wf(native,f+0x10u,(float)((int)(rnd()%6001)-3000)/100.0f);
   wf(native,f+8u,(float)((int)(rnd()%6001)-3000)/100.0f);w32(native,f+0x30u,rnd()%7u);
   wf(native,XY,(float)((int)(rnd()%4001)-2000)/100.0f);wf(native,XY+4u,(float)((int)(rnd()%12001)-6000)/100.0f);
   memset(span(native,RESULT,0xa0u),0xa5,0x80u);memcpy(oracle,native,BYTES);}
  status=pair(0x80024fb8u,RESULT,0,0,0);if(status)return status;
  status=pair(0x80024f58u,META,0,0,0);if(status)return status;
  if(case_index){span(native,META,1)[0]=span(oracle,META,1)[0]=1;
   w32(native,RESULT+8u,r32(native,XY));w32(native,RESULT+0xcu,r32(native,XY+4u));
   w32(oracle,RESULT+8u,r32(oracle,XY));w32(oracle,RESULT+0xcu,r32(oracle,XY+4u));}
  stage=301;status=pair(0x80025840u,META,RESULT,0,0);if(status)return status;
  stage=302;status=pair(0x80025f98u,RESULT,0,0,0);if(status)return status;
  stage=303;status=pair(0x80103c30u,0,0,0,0);if(status)return status;
  stage=304;status=pair(0x8010397cu,case_index%7u,0,0,0);if(status)return status;
  leaves+=6;hash=digest(hash,native,LO-BASE);hash=digest(hash,native+HI-BASE,BYTES-(HI-BASE));
 }
 if(!whistles[0]||!whistles[1]||!whistles[2]||!whistles[3]||!commits||!snaps){fprintf(stderr,"incomplete cycle coverage: contact %u OOB %u TD %u incomplete %u commits %u\n",whistles[0],whistles[1],whistles[2],whistles[3],commits);return 9;}
 printf("Original down cycle: 401 saved/edited cases, 24 x 96 persistent updates; %u updates, %u leaves; whistles contact %u bounds %u touchdown %u incompletion %u; commits %u snaps %u; all RAM outside oracle stack and defined returns match; no original hooks; digest %08x.\n",updates,leaves,whistles[0],whistles[1],whistles[2],whistles[3],commits,snaps,hash);
 return 0;}
