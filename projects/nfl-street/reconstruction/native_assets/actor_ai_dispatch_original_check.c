/* All-RAM oracle for the dispatcher and its supported policy boundary.
   Saved dispatcher callbacks not yet reconstructed are executed by the
   ORIGINAL module on the native side too: this tests the scheduler, not
   those policies. Edited table tests use actual native-supported callbacks.
   80177EF4 / 80076C48 / 80076E18 / 80021B58 / 80021AFC return at entry
   on BOTH sides, recording arguments. No native stand-in runs in the oracle.
   No complete match/gameplay integration is asserted. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "moderngekko/module_abi.h"
#include "actor_ai_dispatch.h"
#include "gekko_float.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define STACK_LO 0x80f38000u
#define STACK_HI 0x80f47000u
#define SCRATCH 0x80f30000u
#define TARGET (STACK_LO+0x40u)
#define CASES 401u
#define ACTORS 14u
#define SEQUENCES 24u
#define UPDATES 32u
static const ModernGekkoModuleDesc *module;
static uint8_t *oracle;
static uint8_t *at(void *p,uint32_t a,uint32_t n){return a>=BASE&&(uint64_t)a-BASE+n<=BYTES?(uint8_t *)p+a-BASE:NULL;}
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t r32(void *p,uint32_t a){const uint8_t *q=at(p,a,4);if(!q){fprintf(stderr,"bad fixture %08x\n",a);exit(4);}return rd(q);}
static void w32(void *p,uint32_t a,uint32_t v){uint8_t *q=at(p,a,4);if(!q)exit(4);q[0]=v>>24;q[1]=v>>16;q[2]=v>>8;q[3]=v;}
static void w16(void *p,uint32_t a,uint32_t v){uint8_t *q=at(p,a,2);q[0]=v>>8;q[1]=v;}
static void w8(void *p,uint32_t a,unsigned v){*at(p,a,1)=(uint8_t)v;}
static uint32_t bits(float f){uint32_t w;memcpy(&w,&f,4);return w;}
static uint32_t seed=0x11c0ffeeu;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static float random_float(float lo,float hi){return lo+(hi-lo)*(float)(rnd()>>8)/16777216.0f;}
static uint32_t actor(void *p,unsigned i){return r32(p,r32(p,R13-0x7c30u))+0x14d4u*i;}
static uint32_t hash(uint32_t h,const uint8_t *p,size_t n){for(size_t k=0;k<n;++k)h=(h^p[k])*16777619u;return h;}
typedef struct {uint32_t pc,args[5],weight;} call;
typedef struct {call calls[256];unsigned count;} trace;
static trace native_trace,original_trace;
static unsigned bridged,skips,entry_hits,pop_hits,remove_hits,follow_hits,eligible_hits,face_hits,interact_hits,commit_hits;
static int append(trace *t,uint32_t pc,const uint32_t *a,uint32_t weight){
 if(t->count>=256)return 0;call *q=&t->calls[t->count++];q->pc=pc;memcpy(q->args,a,20);q->weight=weight;return 1;}
static unsigned effect_args(uint32_t pc){switch(pc){case 0x80177ef4u:case 0x80021b58u:return 5;
 case 0x80076c48u:case 0x80076e18u:return 2;case 0x80021afcu:return 3;default:return 0;}}
static int effect(trace *t,uint32_t pc,const uint32_t *args,uint32_t weight){
 uint32_t a[5]={0};memcpy(a,args,4*effect_args(pc));return append(t,pc,a,weight);}
static int original(uint8_t *ram,trace *t,uint32_t pc,const uint32_t *args,float weight,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 for(unsigned k=0;k<6;++k)c.gpr[3+k]=args[k];c.fpr[1]=weight;c.pc=pc;
 for(unsigned n=0;c.pc;++n){
  if(n>1000000){fprintf(stderr,"timeout entry %08x at %08x\n",pc,c.pc);return 0;}
  if(ram==oracle){entry_hits+=c.pc==0x8007dde8u;pop_hits+=c.pc==0x8007dd80u;remove_hits+=c.pc==0x8007e5a4u;}
  if(effect_args(c.pc)){if(!effect(t,c.pc,&c.gpr[3],c.pc==0x80177ef4u?bits((float)c.fpr[1]):0))return 0;
   ++skips;c.gpr[3]=0;c.pc=c.lr;continue;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"exception entry %08x at %08x lr %08x\n",pc,c.pc,c.lr);return 0;}}
 *result=c.gpr[3];return 1;
}
typedef struct {uint8_t *ram;trace *trace;} bridge;
static int service(void *user,const nflstreet_ai_call *call,uint32_t *result){
 bridge *b=user;if(effect_args(call->pc)){*result=0;return effect(b->trace,call->pc,call->args,0);}
 ++bridged;return original(b->ram,b->trace,call->pc,call->args,0,result);}
static int request(void *user,const nflstreet_locomotion_request *r){
 bridge *b=user;const uint32_t a[]={r->manager,r->set,r->channels,r->key,r->actor};return effect(b->trace,0x80177ef4u,a,bits(r->weight));}
static int locomotion_effect(void *user,uint32_t pc,uint32_t a,uint32_t b){bridge *v=user;const uint32_t args[5]={a,b,0,0,0};return effect(v->trace,pc,args,0);}
static int same(uint8_t *ram,unsigned mode,unsigned index,unsigned i,unsigned step){
 const unsigned lo=STACK_LO-BASE,hi=STACK_HI-BASE;
 if(memcmp(ram,oracle,lo)||memcmp(ram+hi,oracle+hi,BYTES-hi)){
  for(unsigned k=0;k<BYTES;++k)if((k<lo||k>=hi)&&ram[k]!=oracle[k]){
   fprintf(stderr,"RAM mismatch mode %u case %u actor %u step %u at %08x: %02x/%02x\n",mode,index,i,step,BASE+k,oracle[k],ram[k]);break;}return 0;}
 if(native_trace.count!=original_trace.count||memcmp(&native_trace,&original_trace,sizeof native_trace)){
  fprintf(stderr,"effect trace mismatch mode %u case %u actor %u step %u counts %u/%u\n",mode,index,i,step,original_trace.count,native_trace.count);return 0;}
 return 1;}
static void reset_traces(void){memset(&native_trace,0,sizeof native_trace);memset(&original_trace,0,sizeof original_trace);}
static void table_fixture(uint8_t *p,unsigned index,uint32_t a){
 /* Edited state table uses only original routines, never generated PPC or
    invented callback semantics. Preserve queue argument bytes, exercise
    unequal capacities and dead bytes. Slot zero's entry always returns 0. */
 const uint32_t root=SCRATCH,desc=root+0x40,table=root+0x80;
 w32(p,R13-0x7860u,root);w32(p,root,desc);w16(p,desc,0x45);w16(p,desc+2,3+index%8);w32(p,desc+4,table);
 /* Eligibility can enter 21; 8009E590 can push 44. Give every row through
    44 valid ORIGINAL leaves; extended rows retain the command at entry.
    An empty queue must accept exit, matching the saved empty-state row. */
 for(unsigned k=0;k<0x45;++k){w32(p,table+24*k,k&&k<8&&k%3==0?0x8007ea18u:0x8007ea10u);
  w32(p,table+24*k+4,k==0||k>=8||index%3?0x8007ea18u:0x8007ea10u);
  w32(p,table+24*k+8,k>=8?0x8007ea10u:k%2?0x8008d988u:0x8007ea18u);
  w32(p,table+24*k+12,k>=8?0x8007ea10u:k%2?0x8007ea18u:0x8008d988u);
  w32(p,table+24*k+16,0x8007ea20u);w32(p,table+24*k+20,0);}
 const uint32_t q=a+0xbe4;
 for(unsigned k=0;k<10;++k)w32(p,q+4*k,((1+(index+k)%7)<<24)|(rnd()&0xffffffu));
 w8(p,q+4*(2+index%8),0);w32(p,a+0x310,q);
 /* Most edited queues use no-selected; index%6==2 exercises a resolved
    actor. Real follow entry initializes its union before dispatch below.
    8009AD80 rejects natively in phase 3. */
 const uint32_t balls=SCRATCH+0x1000;w32(p,R13-0x73d0u,balls);memset(at(p,balls,0x200),0,0x200);
 if(index%6==2)w32(p,balls+0xbc,r32(p,a));
 w32(p,r32(p,R13-0x7cf8u),3);w32(p,R13-0x7cf4u,rnd());
 w8(p,a+0xbe0,index%3==0?0x39:0);w32(p,a+0xbdc,(index%3==0)?50:rnd()%40);
 w32(p,a+0x1c4,bits(random_float(0,5)));w32(p,a+0x1c8,rnd());
 w32(p,a+0x124,0);w8(p,r32(p,R13-0x7a18u)+0x5d9,0);
 w8(p,a+0xc0c,0xa5);
}
static void prepare(uint8_t *p,unsigned index,unsigned mode,uint32_t a,unsigned i){
 if(!index)return;
 if(mode==0||mode==1){table_fixture(p,index,a);return;}
 for(unsigned k=0;k<12;++k)w32(p,a+0x200+4*k,rnd());
 w32(p,a+0x200,bits(index%9?random_float(0,1.5f):0));w8(p,a+0x20e,(index+i)%11);w8(p,a+0x20f,index%8);
 const uint32_t angles[]={0,1,0x2e38e2,0x2e38e3,0x400000,0x56c16b,0x56c16c,0x71c71b,0x71c71c,0x800000};
 const uint32_t h=rnd(),d=angles[(index+i)%10];w32(p,a+0x204,h);w32(p,a+0x208,h+((index&1)?d:0u-d));
 w32(p,a+0x1ec,bits(index%3==0?0:index%3==1?1:100));w32(p,a+0x1c0,rnd());
 w32(p,a+0x1c8,rnd());w32(p,a+0x1c4,bits(random_float(0,5)));
 if(mode==7){
  w32(p,a+0x210,bits(random_float(-0.25f,2)));w8(p,a+0x20f,(index+i)%40);
  w16(p,a+0x21c,index%3==0?0:index%3==1?r32(p,R13-0x70a8u)>>16:5);
  return;
 }
 if(mode==2)return; /* commit inputs; original managers/policy context retained */
 table_fixture(p,index,a);const uint32_t other=actor(p,(i+1)%ACTORS),q=r32(p,other+0x310);
 w8(p,q,index%5==0?1:index%5==1?0x12:index%5==2?0x19:2);w8(p,q+1,index%2?*at(p,a+1,1):255);w8(p,q+2,index%3?9:8);
 w32(p,a+0x154,bits(random_float(-4,4)));w32(p,a+0x158,bits(random_float(-4,4)));
 for(unsigned k=0;k<2;++k){w32(p,a+0x1a8+4*k,bits(random_float(-20,20)));w32(p,other+0x1a8+4*k,bits(random_float(-20,20)));}
 w32(p,a+0x124,index%2?0:r32(p,other));w32(p,a+0x128,rnd());
 /* Resolve selected actor, self and no-selected, from a real handle encoding. */
 const uint32_t balls=r32(p,R13-0x73d0u);w32(p,balls+0xbc,index%4==0?0:index%4==1?r32(p,a):r32(p,other));
 w32(p,a+0xc,(r32(p,a+0xc)&~0x4000u)|(index%7==0?0x4000u:0));
}
static int one(uint8_t *ram,unsigned mode,unsigned index,unsigned i,unsigned step){
 const uint32_t a=actor(ram,i),b=actor(ram,(i+1)%ACTORS),q=a+0xbe4;
 bridge bridge={ram,&native_trace};nflstreet_ai_dispatch c={{at,ram,R13},service,&bridge,TARGET,0,0};
 nflstreet_locomotion_context loco={at,ram,request,locomotion_effect,&bridge,0,0};
 uint32_t pc=0,nv=0,ov=0,args[6]={0};float weight=0;int ok=0;reset_traces();
 if(mode==0){pc=0x8007e0b4u;args[1]=q;args[2]=a;args[3]=index?index%2:(*at(ram,a+8,1)!=255);
  ok=nflstreet_ai_dispatch_step(&c,0,q,a,(int)args[3]);}
 if(mode==1){pc=index%2?0x8007dde8u:0x8007e650u;args[1]=q;
  if(index%2){args[2]=a;ok=nflstreet_ai_dispatch_enter(&c,0,q,a);}else{const uint32_t command=0x01020304u;w32(ram,SCRATCH+0x700,command);w32(oracle,SCRATCH+0x700,command);
   args[2]=SCRATCH+0x700;args[3]=a;ok=nflstreet_ai_dispatch_replace(&c,0,q,command,a);}}
 if(mode==2){pc=0x800150e8u;args[0]=a;ok=nflstreet_ai_commit(&loco,a);++commit_hits;}
 if(mode==3){pc=0x8008d988u;args[0]=a;ok=nflstreet_ai_follow(&c,a,&nv);++follow_hits;}
 if(mode==4){pc=0x8008d654u;args[0]=a;args[1]=b;ok=nflstreet_ai_eligible(&c,a,b,&nv);++eligible_hits;}
 if(mode==5){pc=0x8005335cu;args[0]=a;args[1]=r32(ram,a+0x204);args[2]=index%4;args[3]=index%2;
  weight=index?random_float(-1,2):1;ok=nflstreet_ai_face(&c,a,args[1],weight,args[2],args[3]);++face_hits;}
 if(mode==7){pc=0x800f8214u;args[0]=a;ok=nflstreet_ai_input_finish(&c,a);}
 if(mode==6){pc=0x80021ef0u;args[0]=a;args[1]=b;ok=nflstreet_ai_interact(&c,a,b,&nv);++interact_hits;}
 if(!ok){fprintf(stderr,"native rejected mode %u case %u actor %u step %u unsupported %08x offset %08x/%08x\n",mode,index,i,step,c.unsupported,r32(ram,a+0x154),r32(ram,a+0x158));return 0;}
 if(!original(oracle,&original_trace,pc,args,weight,&ov))return 0;
 if(!same(ram,mode,index,i,step))return 0;
 if((mode==3||mode==4||mode==6)&&nv!=ov){fprintf(stderr,"return mismatch mode %u case %u actor %u %u/%u\n",mode,index,i,ov,nv);return 0;}
 return 1;
}
/* Edited tables install FOLLOW update callbacks on actors whose saved
   +154/+158 unions belong to unrelated states. Initialize through the actual
   state entry on BOTH copies, once before the batch/sequence. Do not skip a
   selected-actor branch or auto-enter on every update. Saved dispatch stays
   untouched. Compare each entry too, rather than assuming the fixture writes. */
static int follow_entries(uint8_t *ram,unsigned mode,unsigned index){
 reset_traces();
 for(unsigned i=0;i<ACTORS;++i){
  nflstreet_actor_ai leaf={at,ram,R13};uint32_t args[6]={actor(ram,i),0,0,0,0,0},v;
  if(!nflstreet_actor_ai_enter(&leaf,args[0])||!original(oracle,&original_trace,0x8008d954u,args,0,&v))return 0;
  if(!same(ram,mode,index,i,0))return 0;
 }
 return 1;
}
static uint8_t *load(const char *name){FILE *f=fopen(name,"rb");if(!f)return NULL;uint8_t *p=malloc(BYTES);
 if(!p||fread(p,1,BYTES,f)!=BYTES){free(p);fclose(f);return NULL;}fclose(f);return p;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 uint8_t *saved=load(argv[1]),*ram=malloc(BYTES);oracle=malloc(BYTES);if(!saved||!ram||!oracle)return 3;
 uint32_t digest=2166136261u;unsigned runs[8]={0};
 for(unsigned index=0;index<CASES;++index)for(unsigned mode=0;mode<8;++mode){
  memcpy(ram,saved,BYTES);
  for(unsigned i=0;i<ACTORS;++i)prepare(ram,index,mode,actor(ram,i),i);
  memcpy(oracle,ram,BYTES);
  /* Standalone FOLLOW and edited tables that install its callback require
     real entry to initialize +150/+154/+158. Saved dispatcher/transition
     inputs remain as-is. Compare after each entry and each actor operation. */
  if((mode==3||(index&&(mode==0||mode==1)))&&!follow_entries(ram,mode,index))return 7;
  for(unsigned i=0;i<ACTORS;++i){
   if(!index){fprintf(stderr,"saved mode %u actor %u\n",mode,i);fflush(stderr);}
   if(!one(ram,mode,index,i,0))return 7;++runs[mode];}
  digest=hash(digest,ram,STACK_LO-BASE);digest=hash(digest,ram+STACK_HI-BASE,BYTES-(STACK_HI-BASE));}
 unsigned updates=0;
 for(unsigned sequence=0;sequence<SEQUENCES;++sequence){
  memcpy(ram,saved,BYTES);for(unsigned i=0;i<ACTORS;++i)prepare(ram,sequence+1,0,actor(ram,i),i);memcpy(oracle,ram,BYTES);
  if(!follow_entries(ram,8,sequence+1))return 7;
  for(unsigned step=0;step<UPDATES;++step){for(unsigned i=0;i<ACTORS;++i){
    if(!one(ram,0,sequence+1,i,step)||!one(ram,2,sequence+1,i,step)||!one(ram,7,sequence+1,i,step))return 7;++updates;}
   if(!same(ram,8,sequence,0,step))return 7;
   digest=hash(digest,ram,STACK_LO-BASE);digest=hash(digest,ram+STACK_HI-BASE,BYTES-(STACK_HI-BASE));}}
 printf("Original actor AI dispatch: 401 cases (saved + 400 edited) x 14 actors; step %u transition %u commit %u follow %u eligibility %u facing %u interaction %u finish %u; persistent 24 x 32 x 14 = %u; all RAM outside oracle stack and service effects match; entry/pop/remove %u/%u/%u; original-backed policy calls %u; digest %08x.\n",runs[0],runs[1],runs[2],runs[3],runs[4],runs[5],runs[6],runs[7],updates,entry_hits,pop_hits,remove_hits,bridged,digest);
 return 0;
}
