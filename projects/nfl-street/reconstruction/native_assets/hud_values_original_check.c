/* Original-module checks, never a native replacement at an oracle entry.
   Producer slice only: the full UIS script/tick and score messages are probed
   separately and are NOT included in the comparison claim. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "hud_values.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define STACK_LO 0x80f38000u
#define STACK_HI 0x80f47000u
#define CASES 401u
#define UPDATES 8u
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;static unsigned probing;
static uint32_t seen[2048];static unsigned seen_count;
static uint32_t seed=0x1973459u;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(void *r,uint32_t a,uint32_t n){return a>=BASE&&(uint64_t)a-BASE+n<=BYTES?(uint8_t *)r+(a-BASE):NULL;}
static uint8_t *span(void *r,uint32_t a,uint32_t n){if((uint64_t)a+n>STACK_LO&&a<STACK_HI)return NULL;return at(r,a,n);}
static uint32_t word(uint8_t *r,uint32_t a){const uint8_t *p=at(r,a,4);return p?rd(p):0;}
static void wh(uint8_t *r,uint32_t a,unsigned v){uint8_t *p=at(r,a,2);p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put(uint8_t *r,uint32_t a,uint32_t v){wr(at(r,a,4),v);}
static uint32_t bits(float f){uint32_t b;memcpy(&b,&f,4);return b;}
static uint32_t hash(uint32_t h,const uint8_t *p,size_t n){for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *p=malloc(BYTES);
 if(!p||fread(p,1,BYTES,f)!=BYTES||fgetc(f)!=EOF){free(p);p=NULL;}fclose(f);return p;}
static int original(uint32_t pc,uint32_t a,uint32_t b,double *fraction){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=a;c.gpr[4]=b;c.pc=pc;
 for(unsigned step=0;c.pc;++step){
  if(step>2000000u){fprintf(stderr,"Original %08x did not return at %08x lr %08x\n",pc,c.pc,c.lr);return 0;}
  if(probing){unsigned i=0;while(i<seen_count&&seen[i]!=c.pc)++i;if(i==seen_count&&seen_count<2048)seen[seen_count++]=c.pc;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"Original exception %08x pc %08x lr %08x\n",pc,c.pc,c.lr);return 0;}}
 if(fraction)*fraction=c.fpr[1];return 1;}
static int same(uint8_t *r,unsigned index,unsigned step){
 if(!memcmp(r,oracle,STACK_LO-BASE)&&!memcmp(r+STACK_HI-BASE,oracle+STACK_HI-BASE,BYTES-(STACK_HI-BASE)))return 1;
 for(uint32_t i=0;i<BYTES;++i)if((i<STACK_LO-BASE||i>=STACK_HI-BASE)&&r[i]!=oracle[i]){
  fprintf(stderr,"RAM differs case %u update %u at %08x: original %02x native %02x (words %08x/%08x)\n",
   index,step,BASE+i,oracle[i],r[i],word(oracle,(BASE+i)&~3u),word(r,(BASE+i)&~3u));return 0;}return 0;}
static uint32_t gameplay(uint8_t *r,unsigned i){return word(r,word(r,R13-0x7c30u))+i*0x14d4u;}
static uint32_t screen(uint8_t *r,unsigned i){return word(r,R13-(i?0x6c84u:0x6c94u));}
/* This array location follows the actual retained timer's object argument and
   its bytecode's +A8 operand. No captured object address is compiled in. */
static uint32_t payload(uint8_t *r){uint32_t s=screen(r,0),t=word(r,s+0x58),array=word(r,t+0x18)+0xa8;
 if(word(r,array)!=1||word(r,array+4)!=4)return 0;return array+8;}
static void prepare(uint8_t *r,unsigned index){
 if(!index)return;uint32_t t=word(r,R13-0x79fcu);
 static const uint32_t values[]={0,1,100,0x3b9aca00u,0xfffffff0u,0xffffffffu};
 static const unsigned keys[]={0x2c,0x30,0x43,0x46,0x4a,0x54,0xa5,0xc1,0xc3,0xc4,0xc5,0xcf,0xd0,0xd1,0xd2,0,0xffff,0x53,0xc2,0xce,0xd3};
 for(unsigned i=0;i<4;++i){
  unsigned j=(index*4+i)%14;uint32_t a=gameplay(r,j),record=t+24*i;
  put(r,record+4,((j%7)<<16)|((j/7)<<8)|1u);
  if(index%19==0)put(r,record+4,0); /* unresolved reference: no recharge */
  at(r,a+8,1)[0]=255; /* neutral-control boundary; other actors retain saved input */
  uint32_t group=word(r,a+0x318);for(unsigned k=0;k<4;++k){wh(r,group+100*k+6,k==index%4?3:0);wh(r,group+100*k+4,keys[(index+i+k)%21]);}
  at(r,a+0x21f,1)[0]=(uint8_t)((index+i)%13);
  put(r,record+8,index%3?rnd():values[(index+i)%6]);
  put(r,record+0x10,index%4?rnd():values[(index+i+1)%6]);
  put(r,record+0x18,values[(index+i+2)%6]);
  at(r,record+0x14,2)[0]=(uint8_t)((index+i)%4);at(r,record+0x15,1)[0]=(uint8_t)rnd();
  wh(r,record+0x16,index%3?rnd()&0xffffu:(index+i)%3);
 }
 uint32_t gb=word(r,R13-0x7a18u);
 static const uint32_t gbvalues[]={0,1,9999,10000,10001,0xffffffffu,0x80000000u,16777217};
 for(unsigned i=0;i<2;++i)put(r,gb+8+4*i,index%3?rnd():gbvalues[(index+i)%8]);
 put(r,R13-0x4e50u,(index%7==0)?1u:1u+rnd()%20000);
 uint32_t p=payload(r);for(unsigned i=0;i<4;++i)put(r,p+4*i,rnd());
 for(unsigned k=0;k<2;++k){uint32_t s=screen(r,k),n=word(r,s+0x54),timers=word(r,s+0x58);
  for(unsigned i=0;i<n;++i)put(r,timers+0x34*i+0x14,(index+i)%4);}
}
static int probe(uint8_t *pristine){
 memcpy(oracle,pristine,BYTES);probing=1;
 const uint32_t entry[]={0x80109030u,0x80109030u,0x801090d8u};
 for(unsigned i=0;i<3;++i)if(!original(entry[i],i==1?1u:0u,0,NULL))return 0;
 for(unsigned i=0;i<8;++i)if(!original(0x801cf610u,screen(oracle,0),16,NULL))return 0;
 printf("HUD publication probe (original only, not a native comparison): %u distinct dispatch entries\n",seen_count);
 for(unsigned i=0;i<seen_count;++i)printf("entry %08x\n",seen[i]);
 unsigned words=0;for(uint32_t i=0;i<BYTES;i+=4)if((i<STACK_LO-BASE||i>=STACK_HI-BASE)&&memcmp(oracle+i,pristine+i,4)){
  if(words<256)printf("changed %08x %08x -> %08x\n",BASE+i,rd(pristine+i),rd(oracle+i));++words;}
 printf("HUD publication probe: %u changed words outside oracle stack; no hooks.\n",words);return 1;}
int main(int argc,char **argv){
 if(argc!=3&&argc!=4)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 uint8_t *pristine=load(argv[1]),*r=malloc(BYTES);oracle=malloc(BYTES);if(!pristine||!r||!oracle)return 3;
 if(argc==4){if(strcmp(argv[3],"probe"))return 2;return probe(pristine)?0:4;}
 unsigned comparisons=0,turbo_fractions=0,gb_fractions=0;uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index){memcpy(r,pristine,BYTES);prepare(r,index);uint32_t p=payload(r);if(!p)return 3;
  memcpy(oracle,r,BYTES);nflstreet_hud_values c={span,r,R13,0};
  for(unsigned step=0;step<UPDATES;++step){
   if(!nflstreet_hud_turbo_update(&c)||!nflstreet_hud_turbo_publish(&c,p)){
    fprintf(stderr,"Native rejected case %u update %u token/routine %08x\n",index,step,c.rejected_address);return 5;}
   if(!original(0x8005d96cu,0,0,NULL)||!original(0x80119590u,p,0,NULL))return 4;
   for(unsigned k=0;k<2;++k)if(!nflstreet_hud_timer_cleanup(&c,screen(r,k))||!original(0x801d15b0u,screen(oracle,k),0,NULL))return 4;
   if(!same(r,index,step))return 6;++comparisons;
   for(unsigned i=0;i<6;++i){float f;double original_f;
    int ok=i<4?nflstreet_hud_turbo_fraction(&c,i,&f):nflstreet_hud_gamebreaker_fraction(&c,i-4,&f);
    if(!ok||!original(i<4?0x8005d418u:0x80059920u,i<4?i:i-4,0,&original_f))return 4;
    if(bits(f)!=bits((float)original_f)){
     fprintf(stderr,"Fraction differs case %u update %u field %u: original %08x native %08x\n",index,step,i,bits((float)original_f),bits(f));return 6;}
    if(i<4)++turbo_fractions;else ++gb_fractions;}
   /* All getter calls must also leave every nonstack RAM byte unchanged. */
   if(!same(r,index,step))return 6;++comparisons;
   digest=hash(digest,at(r,word(r,R13-0x79fcu),100),100);digest=hash(digest,at(r,p,16),16);
  }}
 printf("Original HUD value producers: 401 cases x 8 updates; %u all-RAM comparisons; %u turbo and %u GameBreaker fractions; turbo UIS payload and timer cleanup match; no hooks; digest %08x.\n",comparisons,turbo_fractions,gb_fractions,digest);
 free(pristine);free(r);free(oracle);FreeLibrary(h);return 0;}
