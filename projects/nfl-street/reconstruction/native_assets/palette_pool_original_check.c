/* Original creator, flip and reservation execute unhooked.
   External heap API8018B7B8 returns early; its owner0 mapping is source-derived. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "palette_pool.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define LO (STACK-16u)
#define HI (STACK+8u)
#define GLOBALS (0x803a9a40u-0x5680u)
#define ARENA 0x81000004u
#define ARENA_BYTES 0xc0000u
#define DESC 0x81400000u
typedef struct {
 uint32_t owner,size,flags,result;
 uint8_t globals[20],descriptor[16],arena[ARENA_BYTES];
} event;
typedef struct {uint8_t *ram;event events[4];unsigned count,bad;uint32_t response;} service;
static uint8_t *ram,*oracle;
static const ModernGekkoModuleDesc *module;
static uint32_t seed=0x9bd21f6u,digest=2166136261u;
static unsigned current;
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(uint8_t *r,uint32_t a){return r+(a-BASE);}
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t heap(void *u,uint32_t owner,uint32_t size,uint32_t flags){service *s=u;
 if(s->count>=4){s->bad=1;return 0;}event *e=&s->events[s->count++];memset(e,0,sizeof *e);
 e->owner=owner;e->size=size;e->flags=flags;e->result=s->response;
 memcpy(e->globals,at(s->ram,GLOBALS),20);memcpy(e->descriptor,at(s->ram,DESC),16);memcpy(e->arena,at(s->ram,ARENA),ARENA_BYTES);
 return s->response;}
static int original(uint32_t pc,uint32_t a,uint32_t b,service *s){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=pc;c.gpr[3]=a;c.gpr[4]=b;
 for(unsigned step=0;c.pc;++step){
  if(step>=1000000){fprintf(stderr,"Oracle timeout %08x\n",c.pc);return 0;}
  if(c.pc==0x8018b7b8u){c.gpr[3]=heap(s,0,c.gpr[3],c.gpr[4]);c.pc=c.lr;continue;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"Oracle exception %u at %08x\n",c.exception,c.pc);return 0;}}
 return 1;}
static int same(const char *phase){
 if(!memcmp(ram,oracle,LO-BASE)&&!memcmp(at(ram,HI),at(oracle,HI),BYTES-(HI-BASE)))return 1;
 for(uint32_t i=0;i<BYTES;++i)if((i<LO-BASE||i>=HI-BASE)&&ram[i]!=oracle[i]){
  fprintf(stderr,"MISMATCH RAM case %u phase %s at %08x: original %02x native %02x\n",current,phase,BASE+i,oracle[i],ram[i]);return 0;}return 0;}
static int traces(service *n,service *o){
 if(n->bad||o->bad||n->count!=o->count||memcmp(n->events,o->events,n->count*sizeof(event))){
  fprintf(stderr,"MISMATCH TRACE case %u native %u original %u\n",current,n->count,o->count);return 0;}
 digest=hash(digest,n->events,n->count*sizeof(event));return 1;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *r=malloc(BYTES);
 if(!r||fread(r,1,BYTES,f)!=BYTES||fgetc(f)!=EOF){free(r);r=NULL;}fclose(f);return r;}
#define HOST(x) do{if(!(x)){fprintf(stderr,"Native rejected case %u line %d\n",current,__LINE__);return 5;}}while(0)
#define ORIG(pc,a,b) do{if(!original(pc,a,b,os))return 4;}while(0)
#define SAME(p) do{if(!same(p))return 7;}while(0)
#define TRACE() do{if(!traces(ns,os))return 8;}while(0)
static int invalid(nflstreet_palette_pool_context x){
 wr(at(ram,GLOBALS+8),0);uint8_t *before=malloc(BYTES);if(!before)return 0;memcpy(before,ram,BYTES);unsigned count=0;
#define REJECT(e) do{if(e){fprintf(stderr,"Host guard failed line %d\n",__LINE__);free(before);return 0;}++count;}while(0)
 REJECT(nflstreet_palette_pool_create(&x,0xffffffffu,0xffffffffu));
 nflstreet_palette_pool_context bad=x;bad.globals=GLOBALS+1;
 REJECT(nflstreet_palette_pool_flip(&bad));bad=x;bad.globals=BASE+BYTES-4;
 REJECT(nflstreet_palette_pool_reserve(&bad,DESC,1));bad=x;bad.services=NULL;
 REJECT(nflstreet_palette_pool_create(&bad,1,1));
 REJECT(nflstreet_palette_pool_reserve(&x,DESC+1,1));
 REJECT(nflstreet_palette_pool_reserve(&x,BASE+BYTES-4,1));
 size_t size=123;REJECT(nflstreet_palette_pool_span(&x,BASE+BYTES-4,&size));if(size){free(before);return 0;}
 if(memcmp(before,ram,BYTES)){free(before);return 0;}
 wr(at(ram,GLOBALS+8),2);memcpy(oracle,ram,BYTES);
 REJECT(nflstreet_palette_pool_reserve(&x,DESC,1));if(memcmp(oracle,ram,BYTES)){free(before);return 0;}
 free(before);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<40||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);
 if(!saved||!ram||!oracle||!ns||!os)return 3;
 unsigned creates=0,failed=0,two=0,other=0,reservations=0,flips=0,wrapped=0,payloads=0,sequences=0,recreated=0;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);memset(ns,0,sizeof *ns);memset(os,0,sizeof *os);ns->ram=ram;os->ram=oracle;
  nflstreet_palette_pool_services services={ns,heap};nflstreet_palette_pool_context x={{ram,BYTES,BASE},GLOBALS,&services};
  if(!current){memcpy(oracle,ram,BYTES);HOST(nflstreet_palette_pool_flip(&x));ORIG(0x801ca494u,0,0);SAME("saved flip");++flips;}
  else{
   for(unsigned j=0;j<ARENA_BYTES+16;++j)at(ram,ARENA-4)[j]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned j=0;j<32;++j)at(ram,DESC-4)[j]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned j=0;j<5;++j)wr(at(ram,GLOBALS+4*j),rnd());
   static const uint32_t caps[]={0,1,2,3,14,255,256,511,7800,8192};unsigned startup=current%16==0;
   uint32_t capacity=startup?0x1e78u:caps[(current-1)%10],multiplier=startup?1:(current/3)%4;
   if(!startup&&current%31==0){capacity=0xffffffffu;multiplier=0;}
   ns->response=os->response=!startup&&current%5==0?0:ARENA;
   if(!startup&&current%19==0)ns->response=os->response=0xffffffe0u;
   static const uint32_t active[]={0,1,2,0xffffffffu,0x80000000u};wr(at(ram,GLOBALS+8),active[(current-1)%5]);
   memcpy(oracle,ram,BYTES);
   HOST(nflstreet_palette_pool_create(&x,capacity,multiplier));ORIG(0x801ca3f4u,capacity,multiplier);SAME("create");TRACE();++creates;
   if(!ns->response)++failed;if(multiplier==2)++two;else ++other;
   for(unsigned k=0;k<3;++k){
    static const uint32_t requested[]={0,1,2,3,14,255,256,0xfffffffeu,0xffffffffu,0x80000000u};
    static const uint32_t cursors[]={0,1,2,7799,7800,0x55555555u,0x80000000u,0xfffffff0u,0xffffffffu};
    uint32_t n=startup?(k==0?3:k==1?14:1):requested[(current+k)%10];
    if(!startup){uint32_t cursor=cursors[(current+k)%9];wr(at(ram,GLOBALS+16),cursor);wr(at(oracle,GLOBALS+16),cursor);}
    uint32_t previous=rd(at(ram,GLOBALS+16)),opaque=rd(at(ram,DESC+4));
    HOST(nflstreet_palette_pool_reserve(&x,DESC,n));ORIG(0x801ca4b0u,DESC,n);SAME("reserve raw");++reservations;
    if(rd(at(ram,GLOBALS+16))<previous||n==0xffffffffu)++wrapped;
    if(rd(at(ram,DESC+4))!=opaque)return 9;
    /* Test usable returned spans separately from raw arithmetic. Controlled
       payload writes are identical fixture effects, not original palette math. */
    size_t bytes;uint8_t *data=nflstreet_palette_pool_span(&x,DESC,&bytes);uint32_t token=rd(at(ram,DESC));
    uint64_t length=(uint64_t)rd(at(ram,DESC+8))*48u;
    unsigned mapped=token>=BASE&&(uint64_t)token+length<=BASE+BYTES;
    if((data!=NULL)!=mapped||(data&&bytes!=length)||(!data&&bytes)||(data&&data!=at(ram,token)))return 9;
    if(data&&token>=ARENA&&(uint64_t)token+bytes<=ARENA+ARENA_BYTES&&bytes<=256*48u){
     for(size_t j=0;j<bytes;++j){uint8_t b=(uint8_t)(rnd()>>17);data[j]=b;at(oracle,token)[j]=b;}++payloads;SAME("controlled caller payload");}
   }
   for(unsigned k=0;k<3;++k){HOST(nflstreet_palette_pool_flip(&x));ORIG(0x801ca494u,0,0);SAME("flip retains payload");++flips;}
   if(startup){HOST(nflstreet_palette_pool_reserve(&x,DESC,1));ORIG(0x801ca4b0u,DESC,1);SAME("startup reuse");++sequences;++reservations;}
   if(current%7==0){ns->response=os->response=ARENA+0x80000u;
    HOST(nflstreet_palette_pool_create(&x,1,2));ORIG(0x801ca3f4u,1,2);SAME("recreate without free");TRACE();++recreated;}
  }
  TRACE();digest=hash(digest,at(ram,GLOBALS),20);digest=hash(digest,at(ram,DESC),16);digest=hash(digest,at(ram,ARENA),ARENA_BYTES);
 }
 int guards=invalid((nflstreet_palette_pool_context){{ram,BYTES,BASE},GLOBALS,&(nflstreet_palette_pool_services){ns,heap}});
 if(guards!=8||!creates||!failed||!two||!other||!reservations||!flips||!wrapped||!payloads||!sequences||!recreated)return 9;
 printf("Original palette pool: %u cases (1 saved + %u edited); create/heap-zero/two-root/other %u/%u/%u/%u; reserve/flip/wrap/payload %u/%u/%u/%u; startup/recreate %u/%u; 8 host guards; void entries, ordered heap observations and all RAM outside [80f46ff0,80f47008) match; digest %08x.\n",edited+1,edited,creates,failed,two,other,reservations,flips,wrapped,payloads,sequences,recreated,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
