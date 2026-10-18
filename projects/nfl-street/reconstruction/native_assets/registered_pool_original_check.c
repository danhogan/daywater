/* Original registry/factory/pool/memset execute in the untouched DLL.
   Only external heap/error/geterror and explicitly supplied constructors return early. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "registered_pool.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define LO (STACK-0x48u)
#define HI (STACK+8u)
#define ROOT (0x803a9a40u-0x4f14u)
#define ALIGNMENT (0x803a9a40u-0x66a8u)
#define TABLE 0x81000000u
#define TABLE2 0x81008000u
#define POOL 0x81100004u
#define CTOR 0x800e1f7cu
#define CTOR2 0x800d2e94u
#define RECORDS 512u
typedef struct {
 uint32_t pc,a,b,c,result,root,object_size;
 uint8_t initial[20],current[20],object[256];
} event;
typedef struct {
 uint8_t *ram;event e[1024];unsigned count;uint32_t response,get_response,type,initial;
 unsigned create,change,changed,ctors,bad;
} service;
static uint8_t *oracle,*ram;
static const ModernGekkoModuleDesc *module;
static unsigned current;
static uint32_t seed=0xac39f21u,digest=2166136261u;
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(uint8_t *r,uint32_t a){return r+(a-BASE);}
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static void note(service *s,uint32_t pc,uint32_t a,uint32_t b,uint32_t c,uint32_t result){
 if(s->count>=1024){s->bad=1;return;}event *e=&s->e[s->count++];memset(e,0,sizeof *e);
 e->pc=pc;e->a=a;e->b=b;e->c=c;e->result=result;e->root=rd(at(s->ram,ROOT));
 memcpy(e->initial,at(s->ram,s->initial),20);memcpy(e->current,at(s->ram,e->root+s->type*20u),20);
 if(pc==CTOR||pc==CTOR2){uint32_t size=rd(e->current);if(size>256){s->bad=1;return;}
  e->object_size=size;memcpy(e->object,at(s->ram,a),size);}}
static uint32_t heap(void *u,uint32_t a,uint32_t b,uint32_t c){service *s=u;note(s,0x8018b7e4u,a,b,c,s->response);return s->response;}
static uint32_t get_error(void *u){service *s=u;note(s,0x801af630u,0,0,0,s->get_response);return s->get_response;}
static void error(void *u,uint32_t code){service *s=u;note(s,0x801af57cu,code,0,0,0);
 /* Controlled service effect, applied at exactly the same original boundary.
    It tests the source's root/size/constructor reloads after allocation. */
 if(s->create&&s->change&&!s->changed&&code==0){
  uint32_t oldroot=rd(at(s->ram,ROOT)),root=s->change&1u?TABLE2:oldroot;
  uint8_t *p=at(s->ram,root+s->type*20u);wr(at(s->ram,ROOT),root);
  wr(p,rd(p)+4u);wr(p+8,rd(p+8)?0:CTOR2);s->changed=1;}}
static void construct(void *u,uint32_t fn,uint32_t object,uint32_t input){service *s=u;
 note(s,fn,object,input,0,0);++s->ctors;
 uint32_t size=rd(at(s->ram,rd(at(s->ram,ROOT))+s->type*20u));
 /* Supplied constructor service response: dirty selected payload bytes. */
 wr(at(s->ram,object+4),input^0x5a12f084u);at(s->ram,object)[size-1]=(uint8_t)(input>>19);}
static int original(uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t e,uint32_t f,service *s,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=pc;
 c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=d;c.gpr[6]=e;c.gpr[7]=f;
 for(unsigned step=0;c.pc;++step){
  if(step>=1000000){fprintf(stderr,"Oracle timeout %08x\n",c.pc);return 0;}
  if(c.pc==0x8018b7e4u){c.gpr[3]=heap(s,c.gpr[3],c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  if(c.pc==0x801af630u){c.gpr[3]=get_error(s);c.pc=c.lr;continue;}
  if(c.pc==0x801af57cu){error(s,c.gpr[3]);c.gpr[3]=0xdeadf00du;c.pc=c.lr;continue;}
  if(c.pc==CTOR||c.pc==CTOR2){construct(s,c.pc,c.gpr[3],c.gpr[4]);c.gpr[3]=0x12345678u;c.pc=c.lr;continue;}
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
#define ORIG(pc,a,b,c,d,e,out) do{if(!original(pc,a,b,c,d,e,os,out))return 4;}while(0)
#define RETURN(g,w,p) do{if(!returns(g,w,p))return 7;}while(0)
#define SAME(p) do{if(!same(p))return 7;}while(0)
static int invalid(nflstreet_registered_context x,nflstreet_factory_record *scratch){
 uint8_t *before=malloc(BYTES);if(!before)return 0;memcpy(before,ram,BYTES);uint32_t out=123;unsigned count=0;
#define REJECT(expr) do{out=123;if((expr)||out){fprintf(stderr,"Host guard failed line %d\n",__LINE__);free(before);return 0;}++count;}while(0)
 REJECT(nflstreet_registered_register(&x,RECORDS,32,1,0,0,&out));
 REJECT(nflstreet_registered_create(&x,scratch,RECORDS,RECORDS,0,0,&out));
 REJECT(nflstreet_registered_create(&x,NULL,RECORDS,0,0,0,&out));
 REJECT(nflstreet_registered_create(&x,scratch,0,0,0,0,&out));
 nflstreet_registered_context bad=x;bad.registry_slot=ROOT+1;
 REJECT(nflstreet_registered_register(&bad,0,32,1,0,0,&out));
 bad=x;bad.registry_slot=BASE+BYTES-2;
 REJECT(nflstreet_registered_create(&bad,scratch,RECORDS,0,0,0,&out));
 bad=x;bad.services=NULL;
 REJECT(nflstreet_registered_register(&bad,0,32,1,0,0,&out));
 bad=x;bad.alignment=3;
 REJECT(nflstreet_registered_register(&bad,0,32,1,0,0,&out));
 REJECT(nflstreet_registered_register(&x,0,0xffffffffu,0xffffffffu,0,0,&out));
 if(memcmp(before,ram,BYTES)){free(before);return 0;}free(before);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<64||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||
 module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);
 nflstreet_factory_record *scratch=calloc(RECORDS,sizeof *scratch);
 if(!saved||!ram||!oracle||!ns||!os||!scratch)return 3;
 unsigned registrations=0,failed=0,nulls=0,ctors=0,objects=0,exhausted=0,changes=0,wide=0;uint8_t headers[256]={0};
 unsigned header_cases=edited<256?32:256;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);memset(ns,0,sizeof *ns);memset(os,0,sizeof *os);ns->ram=ram;os->ram=oracle;
  nflstreet_registered_services services={{ns,heap,error},get_error,construct};
  nflstreet_registered_context x={{ram,BYTES,BASE},ROOT,4,RECORDS,&services};
  uint32_t type=0,got,want;
  if(!current){
   uint32_t table=rd(at(ram,ROOT)),pool=rd(at(ram,table+16));
   if(rd(at(ram,pool))!=14||rd(at(ram,pool+4))!=0x1a90u||rd(at(ram,pool+8))||rd(at(ram,ALIGNMENT))!=4)return 3;
   x.record_count=1;ns->initial=os->initial=table;memcpy(oracle,ram,BYTES);
   HOST(nflstreet_registered_create(&x,scratch,RECORDS,0,0x9b,0x13579,&got));
   ORIG(0x801956a4u,0,0x9b,0x13579,0,0,&want);RETURN(got,want,"saved exhausted");SAME("saved exhausted");
  }else{
   for(unsigned j=0;j<0xc000;++j)at(ram,TABLE)[j]=(uint8_t)(rnd()>>17);
   for(unsigned j=0;j<0x40000;++j)at(ram,POOL-4)[j]=(uint8_t)(rnd()>>17);
   type=(current*37u)%RECORDS;uint32_t size=0x12u+rnd()%173u,a=1u<<((current-1)%7);
   static const uint32_t capacities[]={0,1,2,3,4,7,14,31,254,255,256,257,15};
   uint32_t capacity=current<=header_cases?1:capacities[(current-header_cases-1)%13];
   uint32_t ctor=current%2?CTOR:0,dtor=rnd(),input=rnd();
   wr(at(ram,ROOT),TABLE);wr(at(ram,ALIGNMENT),a);x.alignment=a;
   ns->type=os->type=type;ns->initial=os->initial=TABLE+20u*type;
   ns->response=os->response=current>header_cases&&current%5==0?0:POOL;
   ns->get_response=os->get_response=current%3==0?0:current%3==1?0xffffffffu:0x80000000u;
   memcpy(oracle,ram,BYTES);
   HOST(nflstreet_registered_register(&x,type,size,capacity,ctor,dtor,&got));
   ORIG(0x80195400u,type,size,capacity,ctor,dtor,&want);RETURN(got,want,"register");SAME("register");
   ++registrations;
   if(!ns->response){++failed;if(!traces(ns,os))return 8;continue;}
   if(capacity>=256)++wide;
   /* Explicit stage preparation: alternate root starts as the newly registered
      record. It is only selected by the controlled allocation-error service. */
   memcpy(at(ram,TABLE2+type*20u),at(ram,TABLE+type*20u),20);
   memcpy(at(oracle,TABLE2+type*20u),at(oracle,TABLE+type*20u),20);
   ns->create=os->create=1;
   ns->change=os->change=current>header_cases?(current%4):0;
   unsigned calls=capacity<=14?capacity+1:4;
   if(current>header_cases&&current%13==0)calls=capacity+1;
   for(unsigned k=0;k<calls;++k){
    uint8_t header=(uint8_t)(current-1+k);input^=rnd();
    HOST(nflstreet_registered_create(&x,scratch,RECORDS,type,header,input,&got));
    ORIG(0x801956a4u,type,header,input,0,0,&want);RETURN(got,want,"factory");SAME("factory");
    if(got){++objects;headers[header]=1;if(rd(at(ram,rd(at(ram,ROOT))+type*20u+8)))++ctors;else ++nulls;}
    else ++exhausted;
    if(!traces(ns,os))return 8;
   }
   if(ns->changed)++changes;
  }
  if(!traces(ns,os))return 8;
  digest=hash(digest,ram,LO-BASE);digest=hash(digest,at(ram,HI),BYTES-(HI-BASE));
 }
 /* Reset fixture roots before host-only malformed-input checks. */
 wr(at(ram,ROOT),TABLE);int guards=invalid((nflstreet_registered_context){{ram,BYTES,BASE},ROOT,4,RECORDS,
 &(nflstreet_registered_services){{ns,heap,error},get_error,construct}},scratch);
 unsigned header_count=0;for(unsigned i=0;i<256;++i)header_count+=headers[i];
 if(guards!=9||!registrations||!failed||!nulls||!ctors||!objects||!exhausted||!changes||!wide||
 (edited>=256&&header_count!=256)){fprintf(stderr,"Coverage failed guards %d headers %u\n",guards,header_count);return 9;}
 printf("Original registered pool: %u cases (1 saved + %u edited); registrations/failures %u/%u; objects/nullctor/ctor/exhaustion %u/%u/%u/%u; callback reloads %u; wide pools %u; header bytes %u; 9 host guards; returns, ordered services and all RAM outside [80f46fb8,80f47008) match; digest %08x.\n",
 edited+1,edited,registrations,failed,objects,nulls,ctors,exhausted,changes,wide,header_count,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);free(scratch);FreeLibrary(dll);return 0;}
