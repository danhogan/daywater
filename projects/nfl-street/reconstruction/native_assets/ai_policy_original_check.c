/* GNNE69 r0 policy oracle. Neither side skips or hooks any original routine.
   Native code never calls the DLL. Compare every MEM1 byte outside the
   private call stack, after EACH actor operation, and callback return values.
   Candidate until the external Windows runner accepts the pinned sources. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "moderngekko/module_abi.h"
#include "ai_policy.h"
#include "ai_policy_transaction.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define LO NFLSTREET_AI_POLICY_STACK_LO
#define HI NFLSTREET_AI_POLICY_STACK_HI
#define ACTORS 14u
#define CASES 401u
#define SEQUENCES 24u
#define UPDATES 32u
static const ModernGekkoModuleDesc *module;
static uint8_t *native,*oracle,*saved;
static nflstreet_ai_policy policy;
static uint32_t seed=0x6e2e4aac;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint8_t *span(void *p,uint32_t a,uint32_t n){return a>=BASE&&(uint64_t)a-BASE+n<=BYTES?(uint8_t *)p+a-BASE:NULL;}
static uint32_t r32(void *p,uint32_t a){const uint8_t *q=span(p,a,4);if(!q){fprintf(stderr,"bad fixture %08x\n",a);exit(4);}return(uint32_t)q[0]<<24|(uint32_t)q[1]<<16|(uint32_t)q[2]<<8|q[3];}
static unsigned r8(void *p,uint32_t a){const uint8_t *q=span(p,a,1);if(!q)exit(4);return *q;}
static void w32(void *p,uint32_t a,uint32_t v){uint8_t *q=span(p,a,4);if(!q)exit(4);q[0]=v>>24;q[1]=v>>16;q[2]=v>>8;q[3]=v;}
static void w16(void *p,uint32_t a,unsigned v){uint8_t *q=span(p,a,2);if(!q)exit(4);q[0]=v>>8;q[1]=v;}
static void w8(void *p,uint32_t a,unsigned v){uint8_t *q=span(p,a,1);if(!q)exit(4);*q=(uint8_t)v;}
static void wf(void *p,uint32_t a,float f){uint32_t v;memcpy(&v,&f,4);w32(p,a,v);}
static float random_float(float lo,float hi){return lo+(hi-lo)*(float)(rnd()>>8)/16777216.0f;}
static uint32_t actor(void *p,unsigned i){return r32(p,r32(p,R13-0x7c30u))+0x14d4u*i;}
static uint32_t callback(void *p,uint32_t a){uint32_t desc=r32(p,r32(p,R13-0x7860u));return r32(p,r32(p,desc+4)+24*r8(p,r32(p,a+0x310))+8);}
static const uint32_t roots[]={0x800aac08u,0x800afc88u,0x800a0d10u,0x8006e2e4u,0x8007c180u};
static unsigned population[5],counts[5],seen[5][ACTORS];
static unsigned mode_case,mode_actor,mode_step;
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
static int original_host(uint32_t pc,const uint32_t *args,uint32_t *result){
 const uint32_t zero[6]={0};uint32_t ignored;
 if(!original_raw(0x801f429cu,zero,&ignored))return 0;
 const int ok=original_raw(pc,args,result);
 const int released=original_raw(0x801f42dcu,zero,&ignored);return ok&&released;}
static int same(uint32_t pc){const unsigned lo=LO-BASE,hi=HI-BASE;
 if(memcmp(native,oracle,lo)||memcmp(native+hi,oracle+hi,BYTES-hi)){
  for(unsigned k=0;k<BYTES;++k)if((k<lo||k>=hi)&&native[k]!=oracle[k]){
   fprintf(stderr,"RAM mismatch pc %08x case %u actor %u step %u at %08x original %02x native %02x\n",pc,mode_case,mode_actor,mode_step,BASE+k,oracle[k],native[k]);break;}return 0;}return 1;}
static void reset(void){memcpy(native,saved,BYTES);memcpy(oracle,saved,BYTES);
 nflstreet_ai_policy_init(&policy,(nflstreet_actor_ai){span,native,R13});
 /* Stack starts from the SAME captured bytes. Each side retains its own
    call temporaries between calls. It is the only excluded memory span. */
 memcpy(policy.stack,saved+LO-BASE,sizeof policy.stack);}
static int scheduler_boundary_check(void){
 /* These RAW calls exercise the actual SDK operations without the host
    boundary around them, including nested preexisting lock depths. */
 const uint32_t operations[]={0x801f429cu,0x801f429cu,0x801f45d0u,0x801f42dcu,0x801f42dcu};
 const uint32_t zero[6]={0};
 for(unsigned initial=0;initial<8;++initial){reset();w32(native,R13-0x5588u,initial);w32(oracle,R13-0x5588u,initial);
  unsigned depth=initial;
  for(unsigned i=0;i<5;++i){const nflstreet_ai_call call={operations[i],{0}};uint32_t nv,ov;
   if(!nflstreet_ai_policy_call(&policy,&call,0,&nv)||!original_raw(call.pc,zero,&ov))return 6;
   if(!same(call.pc)||nv!=ov||nv!=(call.pc==0x801f45d0u?0:depth))return 7;
   if(call.pc==0x801f429cu)++depth;else if(call.pc==0x801f42dcu)--depth;
   if(r32(native,R13-0x5588u)!=depth||r32(oracle,R13-0x5588u)!=depth)return 7;
  }
 }
 /* Host-owned invalid target, not a claimed game address: rejection must
    release the scheduler lock and retain the BODY diagnostic. */
 reset();const nflstreet_ai_call invalid={0xfffffffcu,{0}};uint32_t result;
 if(nflstreet_ai_policy_host_call(&policy,&invalid,0,&result)||policy.ok||
    policy.unsupported!=invalid.pc||memcmp(native,saved,BYTES))return 7;
 return 0;}
static void prepare(unsigned index,uint32_t a,uint32_t pc){
 if(!index)return;
 /* Preserve actual queues, handles, pointer-bearing unions and asset tables.
    Dirty output fields even if the saved value already equals a mutant's
    value. Positions, angles and ratings span ordinary finite edited cases. */
 wf(native,a+0x1a8,random_float(-28,28));wf(native,a+0x1ac,random_float(-45,45));
 wf(native,a+0x1c4,random_float(0,6));w32(native,a+0x1c0,rnd()&0xffffffu);w32(native,a+0x1c8,rnd()&0xffffffu);
 wf(native,a+0x200,random_float(0,1));wf(native,a+0x210,random_float(0,1));
 /* 800150E8's actual eleven-entry selector indexes this byte. An arbitrary
    dirty 0xA5 was INVALID when a policy preserves its previous request.
    Dirty all legal selector values instead; keep arbitrary heading outputs. */
 /* 800F6BD8 (selector/active status 5) unconditionally resolves +40C
    with 80020944, then reads the returned actor's +1A8 in 801DE5B8.
    Injecting 5 without an actor-kind handle fabricates a NULL dereference
    (context 14, actor 7, edited sequence 16). Exclude ONLY that invalid
    dirty-input combination. Saved inputs and policy-generated requests are
    unchanged; original paired movement is still native and tested whenever
    the saved actor supplies a real partner. No partner handle is invented. */
 unsigned selector=index%11;
 if(selector==5&&(r32(native,a+0x40c)&255u)!=1)selector=0;
 w8(native,a+0x20e,selector);w8(native,a+0x20f,0);w32(native,a+0x204,rnd());w32(native,a+0x208,rnd());
 for(unsigned k=0;k<10;++k)w16(native,a+0xbb4+2*k,32+rnd()%224);
 /* Independent completion/gate combinations without changing unrelated
    actor flags. No policy path is replaced or hooked to keep these valid. */
 uint32_t flags=r32(native,a+0xc);flags=(flags&~0x4004u)|(index%5==0?4u:0)|(index%7==0?0x4000u:0);w32(native,a+0xc,flags);
 if(pc==roots[0]){w8(native,a+0x15b,index%2?0:0x5a);w16(native,a+0x154,rnd()%80);}
 if(pc==roots[1]){w16(native,a+0x150,rnd());w16(native,a+0x152,index%3==0?0:index%3==1?1:2+rnd()%128);
  w32(native,a+0x154,rnd()&0xffffffu);w32(native,a+0x158,rnd()&0xffffffu);}
 if(pc==roots[2]){w8(native,a+0x165,index%2);w8(native,a+0x164,index%3==0);w32(native,a+0x160,2+rnd()%90);}
 if(pc==roots[3]){w8(native,a+0x15c,index%4?2+rnd()%100:0);w8(native,a+0x15f,index%3?2+rnd()%100:0);}
 /* Preserve 7C180's target handle and legal state, exercise both completion
    flags through the flag edit above. Other states need original entry. */
}
static int one(uint32_t pc,uint32_t a,int dispatch,int compare_return){
 const uint32_t args[6]={dispatch?0:a,dispatch?a+0xbe4:0,dispatch?a:0,0,0,0};uint32_t nv=0,ov=0;
 /* Retain the original UNGUARDED five-service saved/edited comparisons.
    Only host driver/dispatch operations use the explicit host boundary. */
 int raw=0;for(unsigned k=0;k<5;++k)raw|=!dispatch&&pc==roots[k];int ok;
 if(dispatch){nflstreet_ai_dispatch d={{span,native,R13},nflstreet_ai_policy_service,&policy,LO+0x40u,0,0};
  ok=nflstreet_ai_dispatch_step(&d,0,a+0xbe4,a,0);
  if(!ok&&d.unsupported)fprintf(stderr,"dispatch service %08x\n",d.unsupported);
 }else{const nflstreet_ai_call call={pc,{a,0,0,0,0,0}};
  ok=raw?nflstreet_ai_policy_call(&policy,&call,0,&nv):nflstreet_ai_policy_service(&policy,&call,&nv);}
 if(!ok){fprintf(stderr,"native rejected pc %08x case %u actor %u step %u source %08x address %08x target %08x budget %d\n",
  pc,mode_case,mode_actor,mode_step,policy.failure_pc,policy.failure_address,policy.unsupported,policy.failure_budget_exhausted);
  if(policy.failure_budget_exhausted&&policy.failure_pc==0x801f471cu)
   fprintf(stderr,"remaining original scheduler idle wait 801f471c: runnable bitmap [r13-5590]=%08x; external interrupt/thread resumption is not reconstructed\n",r32(native,R13-0x5590u));
  return 6;}
 if(!(raw?original_raw(pc,args,&ov):original_host(pc,args,&ov)))return 6;
 if(!same(pc))return 7;
 if(compare_return&&nv!=ov){fprintf(stderr,"return mismatch pc %08x case %u actor %u step %u original %u native %u\n",pc,mode_case,mode_actor,mode_step,ov,nv);return 7;}return 0;}
static uint32_t hash(uint32_t h){for(unsigned k=0;k<BYTES;++k)if(k<LO-BASE||k>=HI-BASE)h=(h^native[k])*16777619u;return h;}
static uint8_t *load(const char *name){FILE *f=fopen(name,"rb");if(!f)return NULL;uint8_t *p=malloc(BYTES);
 if(!p||fread(p,1,BYTES,f)!=BYTES){free(p);fclose(f);return NULL;}fclose(f);return p;}
/* Host-only journal check: all RAM is checked, without an oracle hook or
   native stand-in. Cover retained pointers, repeated access to one page,
   cross-page writes, the last MEM1 byte, commit and a later rollback. */
static int transaction_check(void){
 nflstreet_ai_transaction t;memcpy(native,saved,BYTES);memcpy(oracle,saved,BYTES);
 if(!nflstreet_ai_transaction_init(&t,span,native))return 0;
 nflstreet_ai_transaction_begin(&t);
 uint8_t *cross=nflstreet_ai_transaction_span(&t,BASE+4094,8);
 if(!cross){nflstreet_ai_transaction_close(&t);return 0;}
 for(unsigned i=0;i<8;++i)cross[i]^=(uint8_t)(0x61u+i);
 /* Reacquire an already MODIFIED page: its first snapshot must survive. */
 uint8_t *again=nflstreet_ai_transaction_span(&t,BASE+4095,1);
 uint8_t *last=nflstreet_ai_transaction_span(&t,BASE+BYTES-1,1);
 if(!again||!last){nflstreet_ai_transaction_close(&t);return 0;}
 *again^=0x43;*last^=0xc7;
 int ok=nflstreet_ai_transaction_end(&t,0)&&!memcmp(native,saved,BYTES);
 nflstreet_ai_transaction_begin(&t);last=nflstreet_ai_transaction_span(&t,BASE+BYTES-1,1);
 if(!last){nflstreet_ai_transaction_close(&t);return 0;}*last^=0x69;oracle[BYTES-1]^=0x69;
 ok=ok&&nflstreet_ai_transaction_end(&t,1)&&!memcmp(native,oracle,BYTES);
 nflstreet_ai_transaction_begin(&t);last=nflstreet_ai_transaction_span(&t,BASE+BYTES-1,1);
 cross=nflstreet_ai_transaction_span(&t,BASE+4094,8);
 if(!last||!cross){nflstreet_ai_transaction_close(&t);return 0;}*last^=0x18;memset(cross,0xa5,8);
 ok=ok&&nflstreet_ai_transaction_end(&t,0)&&!memcmp(native,oracle,BYTES);
 nflstreet_ai_transaction_close(&t);if(!ok)fprintf(stderr,"host transaction rollback differs from full RAM snapshot\n");return ok;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 saved=load(argv[1]);native=malloc(BYTES);oracle=malloc(BYTES);if(!saved||!native||!oracle)return 3;
 if(!transaction_check())return 9;
 {int rc=scheduler_boundary_check();if(rc){fprintf(stderr,"SDK scheduler boundary check failed %d\n",rc);return rc;}}
 /* The scheduler's saved r13-61C0 registration is an actual BLR function.
    Compare its all-RAM and unchanged-r3 behavior against the original,
    including nonzero arguments. This is not an original hook. */
 const uint32_t notification_args[]={0,actor(saved,0),0xffffffffu};
 for(unsigned i=0;i<3;++i){reset();int rc=one(0x801f4124u,notification_args[i],0,1);if(rc)return rc;}
 for(unsigned i=0;i<ACTORS;++i)if(r8(saved,actor(saved,i)+8)==255){uint32_t pc=callback(saved,actor(saved,i));
  for(unsigned k=0;k<5;++k)population[k]+=pc==roots[k];}
 for(unsigned k=0;k<5;++k)if(!population[k]){fprintf(stderr,"missing saved service %08x\n",roots[k]);return 4;}
 uint32_t digest=2166136261u;unsigned dispatched=0,persistent=0;int result;
 for(unsigned index=0;index<CASES;++index){mode_case=index;mode_step=0;
  /* Standalone service bodies: every saved actor actually assigned to EACH
     service, on its own prepared copy. No caller-dependent union fabricated. */
  for(unsigned i=0;i<ACTORS;++i){mode_actor=i;uint32_t a=actor(saved,i);if(r8(saved,a+8)!=255)continue;
   uint32_t pc=callback(saved,a);unsigned k=0;while(k<5&&pc!=roots[k])++k;if(k==5)continue;
   reset();prepare(index,a,pc);memcpy(oracle,native,BYTES);
   if((result=one(pc,a,0,1)))return result;++counts[k];++seen[k][i];}
  /* Actual state table dispatch and final input lifecycle, all 13 actors.
     Include the saved non-five callback (80094E60) without substituting it. */
  reset();for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor(saved,i);if(r8(saved,a+8)==255)prepare(index,a,callback(saved,a));}
  memcpy(oracle,native,BYTES);
  for(unsigned i=0;i<ACTORS;++i){mode_actor=i;uint32_t a=actor(saved,i);if(r8(saved,a+8)!=255)continue;
   if((result=one(0x8007e0b4u,a,1,0))||(result=one(0x800f8214u,a,0,0)))return result;++dispatched;}
  digest=hash(digest);
 }
 for(unsigned sequence=0;sequence<SEQUENCES;++sequence){reset();mode_case=CASES+sequence;
  if(sequence)for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor(saved,i);if(r8(saved,a+8)==255)prepare(sequence,a,callback(saved,a));}
  memcpy(oracle,native,BYTES);
  for(unsigned step=0;step<UPDATES;++step){mode_step=step;
   for(unsigned i=0;i<ACTORS;++i){mode_actor=i;uint32_t a=actor(saved,i);if(r8(saved,a+8)!=255)continue;
    if((result=one(0x8007e0b4u,a,1,0))||(result=one(0x800150e8u,a,0,0))||(result=one(0x800f86bcu,a,0,0)))return result;
    ++persistent;}
  }digest=hash(digest);
 }
 for(unsigned k=0;k<5;++k)for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor(saved,i);
  if(r8(saved,a+8)==255&&callback(saved,a)==roots[k]&&seen[k][i]!=CASES)return 8;}
 printf("Original AI policy: 401 cases (saved + 400 edited), services 800AAC08/800AFC88/800A0D10/8006E2E4/8007C180 actors %u/%u/%u/%u/%u runs %u/%u/%u/%u/%u; dispatch %u; persistent 24 x 32 x 13 = %u; all RAM outside oracle stack and returns match; no original hooks; digest %08x.\n",
 population[0],population[1],population[2],population[3],population[4],counts[0],counts[1],counts[2],counts[3],counts[4],dispatched,persistent,digest);
 return 0;}
