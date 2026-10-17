/* GNNE69 rev 0: controlled scalar producer, not a fresh-match/FNIG owner.
   Original 8014B588..8014B708 wrappers execute unchanged. Only 80149614/66C
   return explicit identical service responses; no native producer runs there.
   Compare every MEM1 byte except ONE declared 0x100-byte oracle stack. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "moderngekko/module_abi.h"
#include "venue_setup.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define CONFIG 0x802a301cu
#define LO 0x80f46f00u
#define HI 0x80f47000u
#define QUERIES 9u
static const uint32_t readers[QUERIES]={0x8014b588u,0x8014b5b8u,0x8014b5e8u,0x8014b618u,
 0x8014b648u,0x8014b678u,0x8014b6a8u,0x8014b6d8u,0x8014b708u};
static const uint32_t tags[QUERIES]={0x4c545645u,0x4c505645u,0x46505645u,0x44575645u,
 0x4c575645u,0x4c465645u,0x43465645u,0x53445645u,0x44455645u};
typedef struct {uint32_t words[QUERIES];double reals[QUERIES];} responses;
typedef struct {uint32_t reader;uint8_t config[0x74],clears[2];} event;
typedef struct {uint8_t *ram;responses *values;event events[QUERIES];unsigned count;int bad;} service;
static const ModernGekkoModuleDesc *module;
static uint8_t *oracle;
static uint32_t seed=0x26ccc34u;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(uint8_t *r,uint32_t a){return r+(a-BASE);}
static float rf(uint8_t *r,uint32_t a){uint32_t b=rd(at(r,a));float f;memcpy(&f,&b,4);return f;}
static void wf(uint8_t *r,uint32_t a,float f){uint32_t b;memcpy(&b,&f,4);wr(at(r,a),b);}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *r=malloc(BYTES);
 if(!r||fread(r,1,BYTES,f)!=BYTES||fgetc(f)!=EOF){free(r);r=NULL;}fclose(f);return r;}
static unsigned note(service *s,uint32_t reader){
 unsigned n=s->count++;if(n>=QUERIES){s->bad=1;return 0;}
 event *e=&s->events[n];e->reader=reader;memcpy(e->config,at(s->ram,CONFIG),sizeof e->config);
 memcpy(e->clears,at(s->ram,0x802a391cu),2);
 if(reader!=readers[n])s->bad=1;return n;}
static uint32_t query_word(void *user,uint32_t reader){service *s=user;unsigned n=note(s,reader);return s->values->words[n];}
static double query_real(void *user,uint32_t reader){service *s=user;unsigned n=note(s,reader);return s->values->reals[n];}
static int original(uint32_t pc,service *s,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=pc;
 unsigned wrapper=QUERIES,seen=0;
 for(unsigned step=0;c.pc;++step){
  if(step>=10000u){fprintf(stderr,"Oracle timeout at %08x\n",c.pc);return 0;}
  for(unsigned n=0;n<QUERIES;++n)if(c.pc==readers[n]){wrapper=n;++seen;}
  if(c.pc==0x80149614u||c.pc==0x8014966cu){
   unsigned n=s->count;
   if(wrapper>=QUERIES||n>=QUERIES||seen!=n+1||c.gpr[3]!=0x8030b184u||c.gpr[4]!=tags[wrapper]||
      c.pc!=((wrapper==0||wrapper==6||wrapper==7)?0x80149614u:0x8014966cu)){
    fprintf(stderr,"Unexpected FNIG query at %08x wrapper %u\n",c.pc,wrapper);return 0;}
   note(s,readers[wrapper]);
   if(c.pc==0x80149614u)c.gpr[3]=s->values->words[wrapper];
   else {c.fpr[1]=s->values->reals[wrapper];c.ps1[1]=c.fpr[1];}
   c.pc=c.lr;continue;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"Oracle exception %u pc %08x lr %08x\n",c.exception,c.pc,c.lr);return 0;}}
 if(result)*result=c.gpr[3];return 1;}
static int same(uint8_t *native,unsigned index,const char *phase){
 if(!memcmp(native,oracle,LO-BASE)&&!memcmp(native+(HI-BASE),oracle+(HI-BASE),BYTES-(HI-BASE)))return 1;
 for(uint32_t n=0;n<BYTES;++n)if((n<LO-BASE||n>=HI-BASE)&&native[n]!=oracle[n]){
  fprintf(stderr,"MISMATCH RAM case %u phase %s at %08x: original %02x native %02x\n",index,phase,BASE+n,oracle[n],native[n]);return 0;}
 return 0;}
static void prepare(uint8_t *r,unsigned index,responses *q){
 memset(q,0,sizeof *q);
 q->words[0]=rd(at(r,CONFIG+0x40));q->words[6]=rd(at(r,CONFIG+0x68));q->words[7]=rd(at(r,CONFIG+0x6c));
 const unsigned offsets[QUERIES]={0,0x4c,0x50,0,0x58,0x64,0,0,0x70};
 for(unsigned n=0;n<QUERIES;++n)if(offsets[n])q->reals[n]=(double)rf(r,CONFIG+offsets[n]);
 q->reals[3]=1.25; /* Explicit controlled FNIG response, including as-is RAM. */
 if(!index)return;
 for(unsigned n=0;n<0x74;++n)at(r,CONFIG)[n]=(uint8_t)((rnd()>>16)|1u);
 at(r,0x802a391cu)[0]=(uint8_t)((rnd()>>16)|1u);at(r,0x802a391du)[0]=(uint8_t)((rnd()>>16)|1u);
 wr(at(r,CONFIG),(index-1)%8);
 /* The original defines a default branch for out-of-range raw keys too. */
 if(index>480)wr(at(r,CONFIG),8u+rnd());
 static const uint32_t selectors[]={0,1,2,3,4,0xffffffffu,0x80000000u,0x100u};
 q->words[0]=selectors[((index-1)/8)%8];q->words[6]=rnd();q->words[7]=rnd()^0x55aa55aau;
 /* Constrain generated doubles to be exactly representable as floats, so the
     native (host C cast) and oracle (Gekko frsp via emulated FNIG wrapper)
     double->float conversions agree. The original FNIG wrapper 8014966C returns
     float in f1; the check intercepts there and must supply a float-equivalent
     double. Host C (float)double and Gekko frsp differ on values not exactly
     representable as float due to rounding mode/FTZ differences. */
  for(unsigned n=1;n<QUERIES;++n){
    float f=(float)(int32_t)(rnd()%200000u)/97.0f-1000.0f;
    q->reals[n]=(double)f;
  }
 static const float scales[]={60.0f,1.0f,0.01f,1.25f,-3.0f,0.0f,0.5f,10000000.0f};
 float threshold=(float)(int32_t)(rnd()%10000u)/4096.0f;
 wf(r,0x80245820u,scales[(index-1)%8]);wf(r,0x80245824u,threshold);
 switch(((index-1)+(index-1)/8)%4){
 case 0:q->reals[1]=(double)threshold;break;
 case 1:q->reals[1]=(double)nextafterf(threshold,INFINITY);break;
 case 2:q->reals[1]=(double)nextafterf(threshold,-INFINITY);break;
 default:q->reals[1]=(double)threshold+0.000000000001;break;}
 /* fmuls rounding across integer boundaries and signed truncation. */
 static const double scaled_inputs[]={1.99999999,-1.99999999,0.99999999,-0.99999999,123.456,-123.456,0.0,10000.0};
 q->reals[3]=scaled_inputs[((index-1)/8)%8];
 /* Exercise fcmpu unordered without relying on NaN payload conversions. */
 if(index%31==0)wr(at(r,0x80245824u),0x7fc00000u);
}
int main(int argc,char **argv){
 if(argc!=4)return 2;char *end=NULL;unsigned long edited=strtoul(argv[3],&end,10);
 if(!end||*end||edited<32||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll){fprintf(stderr,"LoadLibrary failed %lu\n",GetLastError());return 3;}
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||
    module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69")){
  fprintf(stderr,"Original module ABI/game mismatch\n");return 3;}
 uint8_t *saved=load(argv[1]),*ram=malloc(BYTES);oracle=malloc(BYTES);
 if(!saved||!ram||!oracle)return 3;
 unsigned keys=0,types=0,below=0,equal=0,above=0,unordered=0,overlays[3]={0};uint32_t digest=2166136261u;
 for(unsigned index=0;index<=edited;++index){
  memcpy(ram,saved,BYTES);responses q;prepare(ram,index,&q);memcpy(oracle,ram,BYTES);
  service ns={0},os={0};ns.ram=ram;ns.values=&q;os.ram=oracle;os.values=&q;
  uint32_t raw=rd(at(ram,CONFIG)),want=0,base=nflstreet_venue_archive_base(raw);
  if(raw<8)keys|=1u<<raw;if(q.words[0]<4)types|=1u<<q.words[0];
  float rain=(float)q.reals[1],threshold=rf(ram,0x80245824u);
  if(isnan(threshold)||isnan(rain))++unordered;else if(rain<threshold)++below;else if(rain==threshold)++equal;else ++above;
  if(!original(0x80026c34u,&os,&want))return 4;
  if(want!=base){fprintf(stderr,"MISMATCH RETURN case %u original %08x native %08x\n",index,want,base);return 7;}
  if(!same(ram,index,"base"))return 7;
  nflstreet_venue_setup_services services={&ns,query_word,query_real};
  if(!nflstreet_venue_setup(ram,BYTES,&services)){fprintf(stderr,"Native rejected case %u\n",index);return 5;}
  if(!original(0x80026cccu,&os,NULL))return 4;
  if(!same(ram,index,"setup"))return 7;
  uint32_t overlay=rd(at(ram,CONFIG+0x48));if(overlay<3)++overlays[overlay];
  if(ns.bad||os.bad||ns.count!=QUERIES||os.count!=QUERIES||memcmp(ns.events,os.events,sizeof ns.events)){
   fprintf(stderr,"MISMATCH SERVICE case %u: native %u original %u\n",index,ns.count,os.count);return 8;}
  /* Stable digest excludes only the declared original stack. */
  digest=hash(digest,ram,LO-BASE);digest=hash(digest,ram+(HI-BASE),BYTES-(HI-BASE));
 }
 if(keys!=255u||types!=15u||!below||!equal||!above||!unordered||!overlays[0]||!overlays[1]||!overlays[2]){fprintf(stderr,"Incomplete coverage\n");return 9;}
 printf("Original venue setup: %lu cases (1 saved + %lu edited); 8 raw keys; 9 ordered queries/case; base return and all RAM outside [80f46f00,80f47000) match; thresholds below/equal/above/unordered %u/%u/%u/%u; digest %08x.\n",
  edited+1,edited,below,equal,above,unordered,digest);
 free(saved);free(ram);free(oracle);FreeLibrary(dll);return 0;}
