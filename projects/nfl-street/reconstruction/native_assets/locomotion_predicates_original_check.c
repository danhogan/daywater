/* Original-module oracle. Only 80177EF4, 80076C48 and 80076E18 return at
   entry, recording their exact arguments. No original predicate, RNG,
   turn helper or state handler is replaced. Every byte outside the
   oracle stack is compared after each isolated invocation. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "moderngekko/module_abi.h"
#include "locomotion_predicates.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define STACK_LO 0x80f38000u
#define STACK_HI 0x80f47000u
#define CASES 401u
#define ACTORS 14u
#define SCRATCH 0x80f30000u
#define rd32 nflstreet_rd32
#define wr32 nflstreet_wr32
#define bits nflstreet_float_bits
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;
static uint8_t *at(uint8_t *p,uint32_t a,uint32_t n){return a>=BASE&&(uint64_t)a-BASE+n<=BYTES?p+(a-BASE):NULL;}
static uint8_t *span(void *p,uint32_t a,uint32_t n){if(a<STACK_HI&&(uint64_t)a+n>STACK_LO)return NULL;return at(p,a,n);}
static uint32_t r32(uint8_t *p,uint32_t a){const uint8_t *b=at(p,a,4);return b?rd32(b):0;}
static void w32(uint8_t *p,uint32_t a,uint32_t v){uint8_t *b=at(p,a,4);if(!b){fprintf(stderr,"bad fixture %08x\n",a);exit(4);}wr32(b,v);}
static void w16(uint8_t *p,uint32_t a,uint32_t v){nflstreet_wr16(at(p,a,2),v);}
static void w8(uint8_t *p,uint32_t a,uint32_t v){*at(p,a,1)=(uint8_t)v;}
static uint32_t seed=0x13579bdu;
static uint32_t rnd(void){seed=seed*1103515245u+12345u;return seed>>8;}
static float rndf(float lo,float hi){return lo+(hi-lo)*(float)(rnd()&65535u)/65535.0f;}
static uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
typedef struct {uint32_t function,args[6],weight;} call;
typedef struct {call calls[8];unsigned count;} trace;
static trace native_trace,original_trace;
static int append(trace *t,uint32_t function,const uint32_t *args,unsigned count,uint32_t weight){
 if(t->count>=8)return 0;call *v=&t->calls[t->count++];memset(v,0,sizeof *v);v->function=function;v->weight=weight;
 memcpy(v->args,args,4u*count);return 1;}
static int request(void *user,const nflstreet_locomotion_request *r){
 const uint32_t a[]={r->manager,r->set,r->channels,r->key,r->actor};return append(user,0x80177ef4u,a,5,bits(r->weight));}
static int effect(void *user,uint32_t function,uint32_t actor,uint32_t mode){const uint32_t a[]={actor,mode};return append(user,function,a,2,0);}
static unsigned entries[11],predicate_calls,random_calls,turn_calls;
static int original(uint32_t pc,uint32_t g,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=g;c.pc=pc;
 for(unsigned step=0;c.pc;++step){
  if(step>1000000u){fprintf(stderr,"original did not return entry=%08x pc=%08x\n",pc,c.pc);return 0;}
  const uint32_t fn=c.pc;
  if(fn==0x8006510cu)++predicate_calls;if(fn==0x801ee1c0u)++random_calls;if(fn==0x8005335cu)++turn_calls;
  if(fn==0x80177ef4u||fn==0x80076c48u||fn==0x80076e18u){
   uint32_t a[5];for(unsigned k=0;k<5;++k)a[k]=c.gpr[3+k];
   if(!append(&original_trace,fn,a,fn==0x80177ef4u?5:2,fn==0x80177ef4u?bits((float)c.fpr[1]):0))return 0;
   c.gpr[3]=0;c.pc=c.lr;continue;
  }
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"original exception entry=%08x pc=%08x lr=%08x\n",pc,c.pc,c.lr);return 0;}
 }
 *result=c.gpr[3];return 1;
}
static uint32_t gameplay(uint8_t *p,unsigned actor){return r32(p,r32(p,R13-0x7c30u))+0x14d4u*actor;}
static void rng_fixture(uint8_t *p,unsigned kind){
 const uint32_t wrapper=SCRATCH+0x1000,table=wrapper+0x40,s=table+0x40;
 w32(p,R13-0x61f8u,wrapper);memset(at(p,wrapper,0x300),0,0x300);
 w32(p,wrapper+8,rnd());w32(p,wrapper+0xc,rnd());w32(p,wrapper+0x14,table);w32(p,table,s);w32(p,s,kind);
 if(kind==0)w32(p,s+4,rnd()*197u);
 if(kind==1){w32(p,s+4,(rnd()*131u)&0x7fffffffu);w32(p,s+0x88,1+rnd()%2147483646u);
  for(unsigned i=0;i<32;++i)w32(p,s+8+4*i,1+rnd()%2147483646u);}
 if(kind==2){w32(p,s+4,rnd()%56);w32(p,s+8,rnd()%56);
  for(unsigned i=0;i<56;++i)w32(p,s+0x10+4*i,rnd()*37u%1000000000u);}
}
static void prepare(uint8_t *p,uint32_t g,unsigned actor,unsigned index){
 if(!index)return;const unsigned n=index-1;
 static const uint32_t angles[]={0,1,0x1c71c6,0x1c71c7,0x2e38e2,0x2e38e3,0x400000,0x400001,
  0x56c16b,0x56c16c,0x5fffff,0x600000,0x71c71b,0x71c71c,0x755555,0x755556,0x800000,0xaaaa9,0xaaaaa};
 static const uint32_t keys[]={0x19,0x20,0x21,0x30,0x31,0x37,0x38,0x39,0x3c,0x3d,0x3e,0x49,0x4a,0x4f,0x54,0x64,0x69,0x6a,0x6c,0x6d,0x81,0x82,0x83,0x9d,0x9f,0xa0,0xffff};
 static const unsigned statuses[]={1,2,0x12,0x14,0x1c,0x20,0x22,0x23,0x24,0x3d,0x50};
 const unsigned selector=(n+actor)%11,scenario=n/11;
 /* Every input/movement word is edited. Restore valid selector/status bytes
    and finite fields afterwards; unrelated lanes remain randomized. */
 for(unsigned i=0;i<12;++i)w32(p,g+0x200+4*i,rnd()*179u);
 for(unsigned i=0;i<21;++i)w32(p,g+0x1a8+4*i,bits(rndf(-40,40)));
 const uint32_t h=scenario%4==0?rnd()*131u:rnd()&0xffffffu;
 const uint32_t d=angles[(scenario+actor)%((unsigned)(sizeof angles/sizeof *angles))];
 w32(p,g+0x204,h);w32(p,g+0x208,h+((scenario&1)?d:0u-d));
 w32(p,g+0x200,bits(scenario%5==0?0:scenario%5==1?-0.25f:rndf(0.01f,1.1f)));
 w8(p,g+0x20e,selector);w8(p,g+0x20f,scenario%8);
 w32(p,g+0x214,h+angles[(scenario+actor+6)%19]);
 w32(p,g+0x1c0,h+angles[(scenario+actor+9)%19]);w32(p,g+0x1c8,h+angles[(scenario+actor+13)%19]);
 w32(p,g+0x1c4,bits(scenario%16==15?0:rndf(0,10)));w32(p,g+0x1ec,bits(scenario%3==0?0:scenario%3==1?1:100));
 const uint32_t m=SCRATCH,set=m+0x200,channels=m+0x400;
 memset(at(p,m,400),0,400);w32(p,g+0x318,m);w32(p,g+0x31c,set+4*(rnd()%8));w32(p,g+0x320,channels+4*(rnd()%8));
 const unsigned slot=rnd()%4;const uint32_t key=keys[(scenario+actor)%27];
 for(unsigned i=0;i<4;++i){w16(p,m+100*i+4,keys[rnd()%27]);w16(p,m+100*i+6,1+rnd()%2);
  w32(p,m+100*i+0xc,bits(scenario%2?0:rndf(0.01f,4)));w32(p,m+100*i+0x10,h);w32(p,m+100*i+0x14,h+d);}
 w16(p,m+100*slot+4,key);w16(p,m+100*slot+6,scenario%13==12?1:3);
 w32(p,g+0x408,scenario%7);
 uint32_t flags=r32(p,g+0xc);flags&=~(4u|0x400u|0x4000u|0x400000u);
 flags|=(scenario&1?4u:0)|(scenario&2?0x400u:0)|(scenario&4?0x4000u:0)|(scenario&8?0x400000u:0);w32(p,g+0xc,flags);
 w8(p,g+3,scenario%16==15?1:scenario%5==0?2:scenario%5==4?0:1);w8(p,g+8,scenario&4?0:255);
 w8(p,g+0xb5e,scenario%4==0?0:scenario%4==1?1:scenario%4==2?3:20);w8(p,g+0x168,scenario%2);
 w8(p,r32(p,g+0x310),statuses[(scenario+actor)%11]);
 const uint32_t w=r32(p,R13-0x7078u),team=*at(p,g+2,1);
 w8(p,w+0x38,scenario%2?team:team^1u);w32(p,w+0x34,scenario%17==16?1:0);
 const uint32_t teams=r32(p,R13-0x5a98u);
 for(unsigned t=0;t<2;++t){w8(p,teams+t*0x7878u+0x38+0x16,actor%7);w32(p,teams+t*0x7878u+0xd1c+0x14,scenario%9==8?0:2);}
 const uint32_t ctrl=SCRATCH+0x800;memset(at(p,ctrl,0x100),0,0x100);w32(p,R13-0x73d0u,scenario%19==18?0:ctrl);
 w8(p,ctrl,1);w32(p,ctrl+0xbc,r32(p,g));w32(p,r32(p,R13-0x7cf8u),scenario%23==22?2:3);
 w32(p,R13-0x7cf4u,0xdeadbeefu);w8(p,r32(p,R13-0x7328u),scenario%4);
 w32(p,R13-0x70c0u,bits(0.1f+rndf(0,3)));w32(p,R13-0x70bcu,bits(0.1f+rndf(0,3)));
 w16(p,g+0xbb4,scenario%11==0?0:1+rnd()%200);w16(p,R13-0x7af8u,250);
 const uint32_t parameter=SCRATCH+0xa00;w32(p,g+0x30c,scenario%3?parameter:0);w32(p,parameter+8,rnd()%14);
 rng_fixture(p,(n+actor)%3);
 /* Distinguish constants which happen to hold identical 1.0 weights in
    the saved game; the original loads each handler's particular word. */
 static const uint32_t weights[]={0x80244d70u,0x80244d74u,0x80244d7cu,0x80244d80u,0x80244d84u,0x80244d88u,0x80244d94u};
 for(unsigned i=0;i<7;++i)w32(p,weights[i],bits(0.25f+(float)i*0.125f));
 w32(p,0x80244d64u,bits(rndf(0.25f,0.5f)));w32(p,0x80244d68u,bits(rndf(0.5f,0.75f)));
 w32(p,0x80244d90u,bits(rndf(0.1f,0.6f)));w32(p,0x80244d6cu,bits(rndf(0.75f,1.25f)));
 /* Directed Q/suppression calls with both direction tests, preserving a
    nonzero selected magnitude and preventing the 21 guard shortcut. */
 if(selector==8&&scenario%3==0){
  w8(p,g+3,1);w8(p,w+0x38,team^1u);w16(p,m+100*slot+4,0x49);w16(p,m+100*slot+6,3);
  w32(p,g+0x200,bits(0.6f));w32(p,g+0x204,0);w32(p,g+0x208,0);w32(p,g+0x1c0,0x800000);
  w32(p,g+0x1c8,scenario%2?0:0x800000);w32(p,g+0x1c4,bits(100));w8(p,r32(p,g+0x310),0x22+scenario%3);
 }
}
static int same(uint8_t *native){
 const uint32_t lo=STACK_LO-BASE,hi=STACK_HI-BASE;
 if(memcmp(native,oracle,lo)||memcmp(native+hi,oracle+hi,BYTES-hi)){
  for(uint32_t i=0;i<BYTES;++i)if((i<lo||i>=hi)&&native[i]!=oracle[i]){
   fprintf(stderr,"RAM mismatch %08x: original=%08x native=%08x\n",BASE+i,r32(oracle,(BASE+i)&~3u),r32(native,(BASE+i)&~3u));break;}return 0;}
 if(native_trace.count!=original_trace.count||memcmp(&native_trace,&original_trace,sizeof native_trace)){
  fprintf(stderr,"trace mismatch: original=%u native=%u\n",original_trace.count,native_trace.count);
  for(unsigned i=0;i<original_trace.count||i<native_trace.count;++i){
   const call *o=&original_trace.calls[i],*n=&native_trace.calls[i];fprintf(stderr,"call %u fn=%08x/%08x weight=%08x/%08x args",i,o->function,n->function,o->weight,n->weight);
   for(unsigned k=0;k<5;++k)fprintf(stderr," %08x/%08x",o->args[k],n->args[k]);fprintf(stderr,"\n");}return 0;}
 return 1;
}
static uint8_t *load(const char *name){FILE *f=fopen(name,"rb");if(!f)return NULL;uint8_t *p=malloc(BYTES);
 if(!p||fread(p,1,BYTES,f)!=BYTES){free(p);fclose(f);return NULL;}fclose(f);return p;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 uint8_t *pristine=load(argv[1]),*prepared=malloc(BYTES),*native=malloc(BYTES);oracle=malloc(BYTES);
 if(!pristine||!prepared||!native||!oracle)return 3;
 unsigned selects=0,gameplays=0,suppressions=0,pvalues[2]={0},svalues[2]={0},requests=0,effects[2]={0},rngs[3]={0};uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index)for(unsigned actor=0;actor<ACTORS;++actor){
  const uint32_t g=gameplay(pristine,actor);memcpy(prepared,pristine,BYTES);prepare(prepared,g,actor,index);
  /* Full dispatcher for all actors; independently check BOTH stateful
     predicates on actor zero, including saved-as-is. */
  for(unsigned mode=0;mode<(actor==0?3u:1u);++mode){
   if(mode==2&&index&&index%5==1){
    /* Make all three return/service outcomes mandatory, independently of
       the captured probability tables and seeds. Case zero stays as-is. */
    w8(prepared,r32(prepared,g+0x310),0x22);w16(prepared,g+0xbb4,0);
    w32(prepared,g+0x30c,SCRATCH+0xa00);w32(prepared,SCRATCH+0xa08,3);
    w32(prepared,0x8026e0d0u+24,bits(index%3==0?2.0f:-1.0f));
    w32(prepared,0x8026e0d0u+28,bits(index%3==2?-1.0f:2.0f));w16(prepared,R13-0x7af8u,0);
   }
   memcpy(native,prepared,BYTES);memcpy(oracle,prepared,BYTES);memset(&native_trace,0,sizeof native_trace);memset(&original_trace,0,sizeof original_trace);
   nflstreet_locomotion_context c={span,native,request,effect,&native_trace,0,0};uint32_t nv=0,ov=0;int ok;
   const uint32_t pc=mode==0?0x800150e8u:mode==1?0x8006510cu:0x8004f214u;
   if(mode==0){const uint32_t selector=*at(prepared,g+0x20e,1);if(selector>10)return 4;++entries[selector];ok=nflstreet_locomotion_select(&c,g);++selects;}
   else if(mode==1){ok=nflstreet_locomotion_gameplay(&c,g,&nv);++gameplays;}
   else{ok=nflstreet_locomotion_suppression(&c,g,&nv);++suppressions;const uint32_t s=r32(prepared,r32(prepared,r32(prepared,R13-0x61f8u)+0x14));++rngs[r32(prepared,s)];}
   if(ok!=1){fprintf(stderr,"native rejected mode=%u actor=%u case=%u g=%08x unsupported=%08x\n",mode,actor,index,g,c.unsupported);return 5;}
   if(!original(pc,g,&ov)){fprintf(stderr,"original failed mode=%u actor=%u case=%u\n",mode,actor,index);return 6;}
   if(!same(native)||(mode!=0&&nv!=ov)){
    fprintf(stderr,"comparison mismatch mode=%u actor=%u case=%u value=%u/%u selector=%u oldmanager=%08x\n",mode,actor,index,ov,nv,*at(prepared,g+0x20e,1),r32(prepared,g+0x318));return 7;}
   if(mode==1)++pvalues[nv!=0];if(mode==2)++svalues[nv!=0];
   for(unsigned i=0;i<native_trace.count;++i){const call *v=&native_trace.calls[i];requests+=v->function==0x80177ef4u;
    effects[0]+=v->function==0x80076c48u;effects[1]+=v->function==0x80076e18u;digest=fnv(digest,v,sizeof *v);}
   digest=fnv(digest,&nv,sizeof nv);digest=fnv(digest,native+(g-BASE),0x14d4);
   digest=fnv(digest,native+(R13-BASE)-0x7d00u,0x3000);digest=fnv(digest,native+(SCRATCH-BASE),0x1300);
  }
 }
 if(!pvalues[0]||!pvalues[1]||!svalues[0]||!svalues[1]||!requests||!effects[0]||!effects[1]||!turn_calls)return 9;
 for(unsigned i=0;i<11;++i)if(!entries[i])return 9;
 printf("Original locomotion predicates: actors=14 cases=401 select=%u gameplay=%u suppression=%u; all RAM outside oracle stack and service arguments match; digest=%08x.\n",selects,gameplays,suppressions,digest);
 printf("Coverage: gameplay=%u/%u suppression=%u/%u requests=%u effects=%u/%u rng=%u/%u/%u predicate_calls=%u random_calls=%u turn_calls=%u; selectors",pvalues[0],pvalues[1],svalues[0],svalues[1],requests,effects[0],effects[1],rngs[0],rngs[1],rngs[2],predicate_calls,random_calls,turn_calls);
 for(unsigned i=0;i<11;++i)printf(" %u",entries[i]);printf(".\n");return 0;
}
