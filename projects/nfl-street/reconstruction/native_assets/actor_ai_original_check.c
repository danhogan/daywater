/* Unhooked original 8008D5DC/954/764/864, isolated and composed. Every
   comparison covers all MEM1 outside the oracle stack. The saved case
   edits no input; scratch output is guarded. Edited cases seed all input
   bytes, signed assignment bytes, positions, offsets, width and constants.
   Persistent sequences repeat target/input without clearing +20F: target
   selection, input lifecycle and physics are deliberately NOT simulated.
   Sequence positions are explicit fixture edits, applied to both copies. */
#include <windows.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "actor_ai.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define STACK_LO 0x80f38000u
#define STACK_HI 0x80f47000u
#define OUTPUT 0x80f36000u
#define TARGET 0x80f35000u
#define CASES 401u
#define ACTORS 14u
#define SEQUENCES 24u
#define UPDATES 32u
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;
static uint32_t seed=0x1973459u;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(void *memory,uint32_t a,uint32_t n){return a>=BASE&&(uint64_t)a-BASE+n<=BYTES?(uint8_t *)memory+(a-BASE):NULL;}
static uint8_t *span(void *memory,uint32_t a,uint32_t n){if((uint64_t)a+n>STACK_LO&&a<STACK_HI)return NULL;return at(memory,a,n);}
static uint32_t word(uint8_t *r,uint32_t a){return rd(at(r,a,4));}
static float scalar(uint8_t *r,uint32_t a){uint32_t w=word(r,a);float f;memcpy(&f,&w,4);return f;}
static void wf(uint8_t *r,uint32_t a,float f){uint32_t w;memcpy(&w,&f,4);wr(at(r,a,4),w);}
static float rf(float lo,float hi){return lo+(hi-lo)*(float)(rnd()>>8)/16777216.0f;}
static uint32_t actor(uint8_t *r,unsigned i){return word(r,word(r,R13-0x7c30u))+0x14d4u*i;}
static uint32_t hash(uint32_t h,const uint8_t *p,size_t n){for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static int same(const uint8_t *r,unsigned index,unsigned mode,unsigned step){
 if(!memcmp(r,oracle,STACK_LO-BASE)&&!memcmp(r+STACK_HI-BASE,oracle+STACK_HI-BASE,BYTES-(STACK_HI-BASE)))return 1;
 for(uint32_t i=0;i<BYTES;++i)if((i<STACK_LO-BASE||i>=STACK_HI-BASE)&&r[i]!=oracle[i]){
  fprintf(stderr,"RAM mismatch case %u mode %u step %u at %08x: original %02x native %02x\n",index,mode,step,BASE+i,oracle[i],r[i]);return 0;}return 0;}
static int original(uint32_t pc,uint32_t a,uint32_t b,uint32_t c,uint32_t d){
 CPUState cpu;memset(&cpu,0,sizeof cpu);cpu.ram=oracle;cpu.ram_size=BYTES;cpu.msr=0x2000;
 cpu.gpr[1]=STACK_HI-0x40u;cpu.gpr[2]=0x803b9a40u;cpu.gpr[13]=R13;
 cpu.gpr[3]=a;cpu.gpr[4]=b;cpu.gpr[5]=c;cpu.gpr[6]=d;cpu.pc=pc;
 for(unsigned n=0;cpu.pc;++n){if(n>10000u){fprintf(stderr,"original timeout %08x at %08x\n",pc,cpu.pc);return 0;}
  cpu.downcount=-1000000000;module->dispatch(&cpu,cpu.pc);if(cpu.exception){fprintf(stderr,"original exception %08x at %08x\n",pc,cpu.pc);return 0;}}
 return 1;}
static void prepare(uint8_t *r,unsigned index){
 /* Output and guards are not inputs, but poison them even in case zero. */
 for(unsigned j=0;j<ACTORS*16u;++j)at(r,OUTPUT,ACTORS*16u)[j]=(uint8_t)(rnd()>>24);
 for(unsigned i=0;i<ACTORS;++i){uint32_t other=actor(r,(i+1)%ACTORS);
  memcpy(at(r,TARGET+16*i,8),at(r,other+0x1a8u,8),8);}
 if(!index)return;
 uint32_t side=word(r,R13-0x7078u);wf(r,side+0x90u,rf(5.0f,30.0f));
 for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor(r,i),assignment=word(r,a+0x310u);
  /* Exhaust every signed byte value, not just the saved -2 Y offset. */
  at(r,assignment+1,2)[0]=(uint8_t)(index+i*37u);at(r,assignment+1,2)[1]=(uint8_t)(255u-index-i*19u);
  at(r,a+0x150u,1)[0]=(uint8_t)(1u+index%255u);
  for(unsigned j=0;j<0x30;++j)at(r,a+0x200u,0x30)[j]=(uint8_t)(rnd()>>24);
  /* Explicit below-threshold preservation, both zero and dirty +20F. */
  at(r,a+0x20fu,1)[0]=(index+i)%3u==0?0:(uint8_t)(2u+(index+i)%253u);
  wf(r,a+0x1a8u,rf(-40,40));wf(r,a+0x1acu,rf(-60,60));
  wf(r,a+0x154u,rf(-8,8));wf(r,a+0x158u,rf(-8,8));
  /* Force both width-side paths and clamp endpoints. */
  unsigned branch=(index+i)%6u;float width=scalar(r,side+0x90u);
  if(branch<3){wf(r,a+0x154u,2.25f);wf(r,a+0x1a8u,width-(branch==0?0.0f:branch==1?2.75f:6.0f));}
  else {wf(r,a+0x154u,-2.25f);wf(r,a+0x1a8u,-width+(branch==3?0.0f:branch==4?2.75f:6.0f));}
  /* Axis/exact threshold and random XY distances for standalone project. */
  const float distances[12]={0,0.1f,0.3f,0.30000004f,0.5f,1,1.00000012f,2.4883718f,4.9999995f,5,8,80};
  unsigned k=(index+i)%16u;float dx=k<12?distances[k]:rf(-12,12),dy=k<12?0:rf(-12,12);
  if(k%3u==0){float tmp=dx;dx=dy;dy=tmp;}if(k%2u)dx=-dx;if(k%4u==0)dy=-dy;
  wf(r,TARGET+16*i,scalar(r,a+0x1a8u)+dx);wf(r,TARGET+16*i+4,scalar(r,a+0x1acu)+dy);
 }
 /* RAM constants are inputs. Exercise them without invalidating ranges. */
 if(index%4u==0){wf(r,0x80250a60u,rf(0.02f,0.15f));wf(r,0x80250a6cu,rf(0.75f,1.25f));wf(r,0x80250a70u,rf(3,6));
  wf(r,0x80250a74u,rf(0.8f,1.2f));wf(r,0x80250a78u,rf(0.1f,0.4f));wf(r,0x80250a7cu,rf(0.6f,0.95f));
  wf(r,0x80250a80u,rf(0.2f,0.5f));wf(r,0x80250a84u,rf(0.1f,0.4f));wf(r,0x80250a88u,rf(0.05f,0.2f));
  wf(r,0x80250a8cu,rf(-0.01f,0.01f));wf(r,0x80250a90u,rf(0.5f,0.9f));}
}
static unsigned runs[5],near_hits,medium_hits,far_hits,preserved,raised,positive,negative,clamp_lo,clamp_mid,clamp_hi;
static void coverage(uint8_t *r,uint32_t a,uint32_t t){
 double dx=(double)scalar(r,t)-scalar(r,a+0x1a8u),dy=(double)scalar(r,t+4)-scalar(r,a+0x1acu);
 double distance=sqrt(dx*dx+dy*dy);if(distance<=scalar(r,0x80250a84u))++near_hits;
 else if(distance<=scalar(r,0x80250a74u))++medium_hits;else ++far_hits;
 if(scalar(r,a+0x200u)>scalar(r,0x80250a90u))++raised;else ++preserved;}
static int batch(nflstreet_actor_ai *c,unsigned mode,unsigned index){
 uint8_t *r=c->memory;
 for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor(r,i),b=actor(r,(i+1)%ACTORS),out=OUTPUT+16*i,t=TARGET+16*i;int ok=0;
  if(mode==0)ok=nflstreet_actor_ai_decode(c,a,out)&&original(0x8008d5dcu,a,out,0,0);
  if(mode==1)ok=nflstreet_actor_ai_enter(c,a)&&original(0x8008d954u,a,0,0,0);
  if(mode==2){
   /* Saved +154 is a union; only finite saved lanes belong to this API. */
   if(!index&&(!isfinite(scalar(r,a+0x154u))||!isfinite(scalar(r,a+0x158u))))continue;
   float ox=scalar(r,a+0x154u),width=scalar(r,word(r,R13-0x7078u)+0x90u),x=scalar(r,a+0x1a8u);
   if(ox>0)++positive;else ++negative;float f=((ox>0?width-x:width+x)-scalar(r,0x80250a6cu))/(scalar(r,0x80250a70u)-scalar(r,0x80250a6cu));
   if(f<=0)++clamp_lo;else if(f>=1)++clamp_hi;else ++clamp_mid;
   ok=nflstreet_actor_ai_target(c,a,b,a+0x154u,out)&&original(0x8008d764u,a,b,a+0x154u,out);}
  if(mode==3){ok=nflstreet_actor_ai_project(c,a,t)&&original(0x8008d864u,a,t,0,0);coverage(r,a,t);}
  if(mode==4){ok=nflstreet_actor_ai_enter(c,a)&&original(0x8008d954u,a,0,0,0)&&
    nflstreet_actor_ai_target(c,a,b,a+0x154u,out)&&original(0x8008d764u,a,b,a+0x154u,out)&&
    nflstreet_actor_ai_project(c,a,out)&&original(0x8008d864u,a,out,0,0);coverage(r,a,out);}
  if(!ok){fprintf(stderr,"rejected case %u mode %u actor %u\n",index,mode,i);return 0;}++runs[mode];}
 return same(r,index,mode,0);}
static uint8_t *load(const char *name){FILE *f=fopen(name,"rb");if(!f)return NULL;uint8_t *r=malloc(BYTES);
 if(!r||fread(r,1,BYTES,f)!=BYTES){free(r);fclose(f);return NULL;}fclose(f);return r;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 uint8_t *saved=load(argv[1]),*before=malloc(BYTES),*r=malloc(BYTES);oracle=malloc(BYTES);if(!saved||!before||!r||!oracle)return 3;
 nflstreet_actor_ai c={span,r,R13};uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index){memcpy(before,saved,BYTES);prepare(before,index);
  for(unsigned mode=0;mode<5;++mode){memcpy(r,before,BYTES);memcpy(oracle,before,BYTES);if(!batch(&c,mode,index))return 7;
   digest=hash(digest,r,STACK_LO-BASE);digest=hash(digest,r+STACK_HI-BASE,BYTES-(STACK_HI-BASE));}}
 unsigned updates=0;
 for(unsigned sequence=0;sequence<SEQUENCES;++sequence){memcpy(r,saved,BYTES);prepare(r,sequence+1);memcpy(oracle,r,BYTES);
  /* State entry happens once; all subsequent calls retain their writes. */
  for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor(r,i);if(!nflstreet_actor_ai_enter(&c,a)||!original(0x8008d954u,a,0,0,0))return 8;}
  for(unsigned step=0;step<UPDATES;++step){
   for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor(r,i),b=actor(r,(i+1u+step%13u)%ACTORS),out=OUTPUT+16*i;
    /* External world samples; these edits are NOT reconstructed movement. */
    float x=rf(-28,28),y=rf(-40,40);wf(r,a+0x1a8u,x);wf(oracle,a+0x1a8u,x);wf(r,a+0x1acu,y);wf(oracle,a+0x1acu,y);
    if(!nflstreet_actor_ai_target(&c,a,b,a+0x154u,out)||!original(0x8008d764u,a,b,a+0x154u,out)||
      !nflstreet_actor_ai_project(&c,a,out)||!original(0x8008d864u,a,out,0,0))return 9;++updates;}
   if(!same(r,sequence,5,step))return 7;
   digest=hash(digest,r,STACK_LO-BASE);digest=hash(digest,r+STACK_HI-BASE,BYTES-(STACK_HI-BASE));}}
 if(!near_hits||!medium_hits||!far_hits||!preserved||!raised||!positive||!negative||!clamp_lo||!clamp_mid||!clamp_hi)return 10;
 printf("Original actor AI: %u cases (saved + 400 edited), 14 actors; decode %u enter %u target %u project %u composed %u; persistent %u sequences x %u updates x 14 actors = %u; all RAM outside oracle stack matches; coverage near/mid/far %u/%u/%u preserve/raise %u/%u sign %u/%u clamp %u/%u/%u; digest %08x.\n",
  CASES,runs[0],runs[1],runs[2],runs[3],runs[4],SEQUENCES,UPDATES,updates,near_hits,medium_hits,far_hits,preserved,raised,positive,negative,clamp_lo,clamp_mid,clamp_hi,digest);
 return 0;}
