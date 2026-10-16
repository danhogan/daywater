/* GNNE69 r0 selection/install vs UNHOOKED original. Candidate until gate passes. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "play_selection.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define LO NFLSTREET_AI_POLICY_STACK_LO
#define HI NFLSTREET_AI_POLICY_STACK_HI
#define CASES 401u
static const ModernGekkoModuleDesc *module;
static uint8_t *native,*oracle,*saved;
static nflstreet_play_selection policy;
static uint32_t seed=0x706c6179u;
static unsigned case_index,side,stage;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint8_t *span(void *p,uint32_t a,uint32_t n){return a>=BASE&&(uint64_t)a-BASE+n<=BYTES?(uint8_t *)p+a-BASE:NULL;}
static uint32_t r32(void *p,uint32_t a){const uint8_t *q=span(p,a,4);if(!q){fprintf(stderr,"bad fixture %08x\n",a);exit(4);}return(uint32_t)q[0]<<24|(uint32_t)q[1]<<16|(uint32_t)q[2]<<8|q[3];}
static unsigned r8(void *p,uint32_t a){const uint8_t *q=span(p,a,1);if(!q)exit(4);return *q;}
static void w32(void *p,uint32_t a,uint32_t v){uint8_t *q=span(p,a,4);if(!q)exit(4);q[0]=v>>24;q[1]=v>>16;q[2]=v>>8;q[3]=v;}
static void w16(void *p,uint32_t a,unsigned v){uint8_t *q=span(p,a,2);if(!q)exit(4);q[0]=v>>8;q[1]=v;}
static void wf(void *p,uint32_t a,float f){uint32_t v;memcpy(&v,&f,4);w32(p,a,v);}
static int original_raw(uint32_t pc,const uint32_t *args,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
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
static void reset(void){memcpy(native,saved,BYTES);nflstreet_play_selection_init(&policy,(nflstreet_actor_ai){span,native,R13});
 memcpy(policy.stack,saved+LO-BASE,sizeof policy.stack);}
static int rejected(void){fprintf(stderr,"native rejected case %u team %u stage %u pc %08x address %08x target %08x budget %d\n",
 case_index,side,stage,policy.failure_pc,policy.failure_address,policy.unsupported,policy.failure_budget_exhausted);return 5;}
static uint32_t team_base(void *p,unsigned team){return r32(p,R13-0x5a98u)+0x7878u*team;}
static int original_select(unsigned team,nflstreet_selected_play *out){
 uint32_t ignored,result;const unsigned offense=r8(oracle,r32(oracle,R13-0x7078u)+0x38u);
 const uint32_t flags[6]={3,0,0,0,0,0},zero[6]={0},expand[6]={team,0,0,0,0,0};
 if(!original_host(0x800f3264u,flags,&ignored)||
    !original_host(team==offense?0x800f4e54u:0x800f4ee0u,zero,&result)||
    !original_host(0x800f2718u,expand,&ignored)||!original_host(0x800f3278u,flags,&ignored))return 0;
 out->team=team;const uint32_t play=team_base(oracle,team)+0xd1cu;out->id=r32(oracle,play);
 for(unsigned i=0;i<28;++i)out->name[i]=(char)r8(oracle,play+0x1f0u+i);out->name[28]=0;
 out->script_result=(int32_t)result;return 1;}
static int original_install(unsigned team){const uint32_t b=team_base(oracle,team),args[6]={b+0x38u,b+0xd1cu,team,0,0,0};uint32_t ignored;
 return original_host(0x800ef234u,args,&ignored);}
/* Evidence: 80103964 down, 80103C18 target line, 801039D4 ball XY,
   80104274 scores; 80188008 RNG state+4; 801EE7F8 elapsed tick pair.
   Timer variation is not a claim that this untimed match has a game clock. */
static void prepare(unsigned index){
 if(!index)return;const uint32_t f=r32(native,R13-0x7078u);
 w32(native,f+0x30u,1+(index-1)%4);w16(native,f+0x3cu,rnd()%50);w16(native,f+0x3eu,rnd()%50);
 const float y=(float)((int)(rnd()%9001)-4500)/100.0f;
 wf(native,f+0x10u,y);wf(native,f+8u,y+(float)(1+rnd()%3500)/100.0f);
 wf(native,f+0xcu,(float)((int)(rnd()%2001)-1000)/100.0f);
 uint32_t rng=r32(native,R13-0x61f8u),object=r32(native,r32(native,rng+0x14u));
 w32(native,object+4u,rnd());w32(native,rng+8u,rnd());w32(native,rng+0xcu,rnd());
 const uint32_t timer=r32(native,R13-0x61f0u);if(timer){uint32_t t=rnd();w32(native,timer,t);w32(native,timer+4u,t+rnd()%36001);}
 /* Source fields only; preserve all pointers, callback bindings, option enums,
    actual playbook DB contents and scheduler/mutex ownership. */
}
static uint32_t digest(uint32_t d,const void *p,unsigned n){const uint8_t *b=p;for(unsigned i=0;i<n;++i)d=(d^b[i])*16777619u;return d;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *p=malloc(BYTES);
 int ok=p&&fread(p,1,BYTES,f)==BYTES&&fgetc(f)==EOF;fclose(f);if(!ok){free(p);return NULL;}return p;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 saved=load(argv[1]);native=malloc(BYTES);oracle=malloc(BYTES);if(!saved||!native||!oracle)return 3;
 uint32_t hash=2166136261u,ids[2][CASES];unsigned distinct[2]={0},changed[2]={0},runs=0;
 for(side=0;side<2;++side)for(case_index=0;case_index<CASES;++case_index){
  reset();prepare(case_index);memcpy(oracle,native,BYTES);stage=0;nflstreet_selected_play nv,ov;
  if(!nflstreet_play_select(&policy,side,&nv))return rejected();if(!original_select(side,&ov))return 6;
  if(!same())return 7;if(nv.team!=ov.team||nv.id!=ov.id||nv.script_result!=ov.script_result||memcmp(nv.name,ov.name,sizeof nv.name)){fprintf(stderr,"return mismatch case %u team %u\n",case_index,side);return 7;}
  unsigned j;for(j=0;j<distinct[side];++j)if(ids[side][j]==nv.id)break;if(j==distinct[side])ids[side][distinct[side]++]=nv.id;
  stage=1;const uint32_t actors=r32(native,r32(native,R13-0x7c30u));uint32_t before=2166136261u;
  for(j=0;j<7;++j){uint32_t a=actors+0x14d4u*(7u*side+j);before=digest(before,span(native,r32(native,a+0x310u),40),40);}
  if(!nflstreet_play_install(&policy,side))return rejected();if(!original_install(side))return 6;if(!same())return 7;
  uint32_t after=2166136261u;for(j=0;j<7;++j){uint32_t a=actors+0x14d4u*(7u*side+j);after=digest(after,span(native,r32(native,a+0x310u),40),40);}
  changed[side]+=before!=after;hash=digest(hash,&nv.id,4);hash=digest(hash,native,LO-BASE);hash=digest(hash,native+HI-BASE,BYTES-(HI-BASE));++runs;
 }
 /* RNG is independently tested with changed seeds and bounds, including 0/1.
    The fixture RNG is used ONLY to prepare edits; production uses 801EE24C. */
 for(case_index=0;case_index<CASES;++case_index){reset();prepare(case_index);memcpy(oracle,native,BYTES);stage=2;
  const uint32_t args[6]={0,case_index,0,0,0,0};uint32_t nv,ov;
  if(!nflstreet_play_selection_call(&policy,0x801ee24cu,args,&nv))return rejected();if(!original_host(0x801ee24cu,args,&ov))return 6;
  if(!same())return 7;if(nv!=ov){fprintf(stderr,"return mismatch RNG case %u\n",case_index);return 7;}}
 if(distinct[0]<2||distinct[1]<2||!changed[0]||!changed[1]){fprintf(stderr,"incomplete selection coverage ids %u/%u installed %u/%u\n",distinct[0],distinct[1],changed[0],changed[1]);return 9;}
 printf("Original play selection: %u cases (saved + 400 edited) x 2 teams = %u selections and installations; RNG 401; distinct %u/%u; changed queues %u/%u; all RAM outside oracle stack and returns match; no original hooks; digest %08x.\n",CASES,runs,distinct[0],distinct[1],changed[0],changed[1],hash);
 return 0;}
