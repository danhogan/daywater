/* Untouched DLL dispatch; original heap/error services alone return early.
   No original pool routine is intercepted or replaced with native code. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "object_pool.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define LO (STACK-0x38u)
#define HI (STACK+8u)
#define POOL 0x81000000u
#define OUTPUT 0x810f0000u
#define ALIGNMENT (0x803a9a40u-0x66a8u)
typedef struct {uint32_t pc,a,b,c,result;} event;
typedef struct {event e[1024];unsigned count;uint32_t response;int bad;} service;
static const ModernGekkoModuleDesc *module;
static uint8_t *oracle,*ram;
static uint32_t seed=0xb22a331u,digest=2166136261u;
static unsigned current;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(uint8_t *r,uint32_t a){return r+(a-BASE);}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static void note(service *s,event e){if(s->count>=1024){s->bad=1;return;}s->e[s->count++]=e;}
static uint32_t heap(void *u,uint32_t a,uint32_t b,uint32_t c){service *s=u;note(s,(event){0x8018b7e4u,a,b,c,s->response});return s->response;}
static void error(void *u,uint32_t code){note(u,(event){0x801af57cu,code,0,0,0});}
static int original(uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t e,service *s,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=pc;
 c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=d;c.gpr[6]=e;
 for(unsigned step=0;c.pc;++step){
  if(step>=1000000u){fprintf(stderr,"Oracle timeout %08x\n",c.pc);return 0;}
  if(c.pc==0x8018b7e4u){c.gpr[3]=heap(s,c.gpr[3],c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  if(c.pc==0x801af57cu){error(s,c.gpr[3]);c.gpr[3]=0xdeadf00du;c.pc=c.lr;continue;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"Oracle exception %u at %08x\n",c.exception,c.pc);return 0;}}
 if(result)*result=c.gpr[3];return 1;}
static int same(const char *phase){
 if(!memcmp(ram,oracle,LO-BASE)&&!memcmp(at(ram,HI),at(oracle,HI),BYTES-(HI-BASE)))return 1;
 for(uint32_t i=0;i<BYTES;++i)if((i<LO-BASE||i>=HI-BASE)&&ram[i]!=oracle[i]){
  fprintf(stderr,"MISMATCH RAM case %u phase %s at %08x: original %02x native %02x\n",current,phase,BASE+i,oracle[i],ram[i]);return 0;}
 return 0;}
static int returns(uint32_t got,uint32_t want,const char *phase){
 if(got==want)return 1;fprintf(stderr,"MISMATCH RETURN case %u phase %s original %08x native %08x\n",current,phase,want,got);return 0;}
static int traces(service *n,service *o){
 if(n->bad||o->bad||n->count!=o->count||memcmp(n->e,o->e,n->count*sizeof(event))){
  fprintf(stderr,"MISMATCH TRACE case %u native %u original %u\n",current,n->count,o->count);return 0;}
 digest=hash(digest,n->e,n->count*sizeof(event));return 1;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *r=malloc(BYTES);
 if(!r||fread(r,1,BYTES,f)!=BYTES||fgetc(f)!=EOF){free(r);r=NULL;}fclose(f);return r;}
#define HOST(x) do{if(!(x)){fprintf(stderr,"Native rejected case %u line %d\n",current,__LINE__);return 5;}}while(0)
#define ORIG(pc,a,b,c,d,s,out) do{if(!original(pc,a,b,c,d,s,out))return 4;}while(0)
#define RETURN(g,w,p) do{if(!returns(g,w,p))return 7;}while(0)
#define SAME(p) do{if(!same(p))return 7;}while(0)
static int invalid(nflstreet_pool_ram r){
 uint8_t before[128];memcpy(before,at(ram,POOL),128);uint32_t out=0x77777777u;service s={0};
 nflstreet_pool_services services={&s,heap,error};unsigned count=0;
#define REJECT(x) do{if(x){fprintf(stderr,"Invalid host input accepted at %d\n",__LINE__);return 0;}++count;}while(0)
 REJECT(nflstreet_pool_reset(r,POOL+1));REJECT(nflstreet_pool_push(r,BASE+BYTES-4,0));
 REJECT(nflstreet_pool_pop(r,POOL,NULL));REJECT(nflstreet_pool_allocate(r,POOL,NULL,&out));
 REJECT(nflstreet_pool_construct(r,1,1,1,POOL,0,&services,&out));
 REJECT(nflstreet_pool_construct(r,1,1,1,POOL,3,&services,&out));
 REJECT(nflstreet_pool_construct(r,1,0xffffffffu,0xffffffffu,POOL,4,&services,&out));
 REJECT(nflstreet_pool_construct(r,1,1,1,BASE+BYTES-4,4,&services,&out));
 REJECT(nflstreet_pool_construct(r,1,1,1,POOL+1,4,&services,&out));
 nflstreet_pool_ram tiny={ram,8,BASE};REJECT(nflstreet_pool_reset(tiny,BASE));
 REJECT(nflstreet_pool_layout(1,1,4,NULL,&out,&out));
 if(memcmp(before,at(ram,POOL),128)||out!=0x77777777u||s.count)return 0;
 return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<32||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll){fprintf(stderr,"LoadLibrary %lu\n",GetLastError());return 3;}
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||
 module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);if(!saved||!ram||!oracle)return 3;
 nflstreet_pool_ram r={ram,BYTES,BASE};unsigned heap_ok=0,heap_fail=0,supplied=0,wide=0,narrow=0,pops=0,exhausted=0,allocations=0;
 uint32_t registry=rd(at(saved,0x803a9a40u-0x4f14u));if(registry<BASE||registry>BASE+BYTES-20)return 3;
 uint32_t saved_pool=rd(at(saved,registry+16));if(saved_pool<BASE||saved_pool>BASE+BYTES-24)return 3;
 uint32_t saved_alignment=rd(at(saved,ALIGNMENT));
 if(saved_alignment!=4||rd(at(saved,saved_pool))!=14||rd(at(saved,saved_pool+4))!=0x1a90u||rd(at(saved,saved_pool+8))!=0){
  fprintf(stderr,"Saved pool provenance differs\n");return 3;}
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);memcpy(oracle,saved,BYTES);service ns={0},os={0};nflstreet_pool_services services={&ns,heap,error};
  uint32_t got,want,pool=saved_pool;
  if(!current){
   HOST(nflstreet_pool_pop(r,pool,&got));ORIG(0x801810a4u,pool,0,0,0,&os,&want);RETURN(got,want,"saved pop");SAME("saved pop");
   HOST(nflstreet_pool_allocate(r,pool,&services,&got));ORIG(0x801812d4u,pool,0,0,0,&os,&want);RETURN(got,want,"saved allocate");SAME("saved allocate");
   if(!traces(&ns,&os))return 8;
  }else{
   /* Scratch identities are harness choices. Dirty padding and data are not zeroed. */
   for(unsigned j=0;j<0x90000u;++j)at(ram,POOL-32)[j]=(uint8_t)(rnd()>>19);
   static const uint32_t capacities[]={0,1,2,3,14,254,255,256,257,511,1024,65535,65536};
   uint32_t n=capacities[(current-1)%13],stride=(rnd()%7u),a=1u<<((current-1)%7),owner=rnd();
   uint32_t x=rnd(),aligns[]={0,1,2,3,4,8,16,32,64,256,0x80000000u};
   uint32_t raw_a=aligns[(current-1)%11];wr(at(ram,ALIGNMENT),raw_a);memcpy(oracle,ram,BYTES);
   got=nflstreet_pool_align(x,raw_a);ORIG(0x80180328u,x,0,0,0,&os,&want);RETURN(got,want,"align");SAME("align");
   uint32_t ni,nd,nt;HOST(nflstreet_pool_layout(x,stride,raw_a,&ni,&nd,&nt));
   wr(at(ram,OUTPUT),ni);wr(at(ram,OUTPUT+4),nd);
   ORIG(0x80181040u,x,stride,OUTPUT,OUTPUT+4,&os,&want);RETURN(nt,want,"raw layout");SAME("raw layout");
   wr(at(ram,ALIGNMENT),a);wr(at(oracle,ALIGNMENT),a);
   uint32_t identity=POOL+4u*((current/3)%8);
   uint32_t supplied_block=current%3==0?identity:0;ns.response=os.response=current%3==2?0:identity;
   HOST(nflstreet_pool_construct(r,owner,n,stride,supplied_block,a,&services,&got));
   ORIG(0x801811f0u,owner,n,stride,supplied_block,&os,&want);RETURN(got,want,"construct");SAME("construct");
   if(supplied_block)++supplied;else if(got)++heap_ok;else ++heap_fail;
   if(!traces(&ns,&os))return 8;
   if(!got)continue;
   pool=got;if(n<=255)++narrow;else ++wide;
   /* Natural reset sequence is descending LIFO. Drain all boundary-sized
      pools, sample the large 65535/65536 pools without a huge trace buffer. */
   unsigned drain=n<=1024?n:8;
   for(unsigned k=0;k<drain;++k){
    HOST(nflstreet_pool_pop(r,pool,&got));ORIG(0x801810a4u,pool,0,0,0,&os,&want);RETURN(got,want,"LIFO pop");++pops;}
   SAME("drained");
   /* Push values explicitly exercise truncation, duplicates and over-capacity
      writes: the original has no capacity/value validation. */
   static const uint32_t indices[]={0x12345678u,0xffffffffu,0,0x100u};
   for(unsigned k=0;k<4;++k){HOST(nflstreet_pool_push(r,pool,indices[k]));ORIG(0x801810ecu,pool,indices[k],0,0,&os,NULL);}
   SAME("push");
   for(unsigned k=0;k<4;++k){
    HOST(nflstreet_pool_allocate(r,pool,&services,&got));ORIG(0x801812d4u,pool,0,0,0,&os,&want);RETURN(got,want,"allocate");++allocations;}
   SAME("allocated");
   if(n<=1024){HOST(nflstreet_pool_allocate(r,pool,&services,&got));ORIG(0x801812d4u,pool,0,0,0,&os,&want);
    RETURN(got,want,"exhausted allocate");++exhausted;SAME("exhausted");}
   HOST(nflstreet_pool_reset(r,pool));ORIG(0x80181128u,pool,0,0,0,&os,NULL);SAME("reset");
   if(!traces(&ns,&os))return 8;
  }
  digest=hash(digest,ram,LO-BASE);digest=hash(digest,at(ram,HI),BYTES-(HI-BASE));
 }
 int invalids=invalid(r);if(invalids!=11){fprintf(stderr,"Host invalid checks failed %d\n",invalids);return 9;}
 if(!heap_ok||!heap_fail||!supplied||!wide||!narrow||!pops||!exhausted)return 9;
 printf("Original object pool: %u cases (1 saved + %u edited); supplied/heap/failure %u/%u/%u; narrow/wide %u/%u; pops/allocations/exhaustion %u/%u/%u; 11 invalid host inputs; returns, ordered services and all RAM outside [80f46fc8,80f47008) match; saved pool %08x alignment %u; digest %08x.\n",
 edited+1,edited,supplied,heap_ok,heap_fail,narrow,wide,pops,allocations,exhausted,saved_pool,saved_alignment,digest);
 free(saved);free(ram);free(oracle);FreeLibrary(dll);return 0;}
