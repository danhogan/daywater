/* Untouched original initializer/wrapper and bounded composition. No game
   registry/pool/scene/factory/memset/init routine is intercepted. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "registry_init.h"
#include "registered_pool.h"
#include "scene_owner.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define INIT_LO (STACK-0x18u)
#define COMPOSE_LO (STACK-0xa0u)
#define HI (STACK+8u)
#define COUNT (0x803a9a40u-0x4f18u)
#define ROOT (0x803a9a40u-0x4f14u)
#define FLAG (0x803a9a40u-0x6568u)
#define ALIGNMENT (0x803a9a40u-0x66a8u)
#define TABLE 0x81000000u
#define TABLE2 0x81010000u
#define POOL 0x81100004u
#define SCENE 0x81200004u
#define LIST 0x81300004u
#define CTOR 0x800e1f7cu
#define TABLE_BYTES 0x3000u
typedef struct {
 uint32_t pc,a,b,c,result,root,count;uint8_t flag[8];
 uint8_t tables[2][TABLE_BYTES],pool[92],scene[56],list[100],object[32];
} event;
typedef struct {
 uint8_t *ram;event e[16];unsigned count,heaps,bad,composed;
 uint32_t responses[4],get_response,table_size[2],scene_size;
} service;
static uint8_t *oracle,*ram;
static const ModernGekkoModuleDesc *module;
static unsigned current;
static uint32_t seed=0xdabb311u,digest=2166136261u;
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(uint8_t *r,uint32_t a){return r+(a-BASE);}
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static void note(service *s,uint32_t pc,uint32_t a,uint32_t b,uint32_t c,uint32_t result){
 if(s->count>=16||s->table_size[0]>TABLE_BYTES||s->table_size[1]>TABLE_BYTES||s->scene_size>56){s->bad=1;return;}
 event *e=&s->e[s->count++];memset(e,0,sizeof *e);e->pc=pc;e->a=a;e->b=b;e->c=c;e->result=result;
 e->root=rd(at(s->ram,ROOT));e->count=rd(at(s->ram,COUNT));memcpy(e->flag,at(s->ram,FLAG-3),8);
 memcpy(e->tables[0],at(s->ram,TABLE),s->table_size[0]);memcpy(e->tables[1],at(s->ram,TABLE2),s->table_size[1]);
 if(s->composed){memcpy(e->pool,at(s->ram,POOL),92);memcpy(e->scene,at(s->ram,SCENE),s->scene_size);memcpy(e->list,at(s->ram,LIST),100);}
 if(pc==CTOR)memcpy(e->object,at(s->ram,a),32);}
static uint32_t heap(void *u,uint32_t a,uint32_t b,uint32_t c){service *s=u;
 if(s->heaps>=4){s->bad=1;return 0;}uint32_t result=s->responses[s->heaps++];note(s,0x8018b7e4u,a,b,c,result);return result;}
static uint32_t get_error(void *u){service *s=u;note(s,0x801af630u,0,0,0,s->get_response);return s->get_response;}
static void error(void *u,uint32_t code){note(u,0x801af57cu,code,0,0,0);}
static void construct(void *u,uint32_t fn,uint32_t object,uint32_t input){service *s=u;note(s,fn,object,input,0,0);
 wr(at(s->ram,object+4),input^0x5abc9081u);}
static int original(uint32_t pc,const uint32_t args[8],service *s,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=pc;
 for(unsigned i=0;i<8;++i)c.gpr[3+i]=args[i];
 for(unsigned step=0;c.pc;++step){
  if(step>=1000000){fprintf(stderr,"Oracle timeout %08x\n",c.pc);return 0;}
  if(c.pc==0x8018b7e4u){c.gpr[3]=heap(s,c.gpr[3],c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  if(c.pc==0x801af630u){c.gpr[3]=get_error(s);c.pc=c.lr;continue;}
  if(c.pc==0x801af57cu){error(s,c.gpr[3]);c.gpr[3]=0xdeadf00du;c.pc=c.lr;continue;}
  if(c.pc==CTOR){construct(s,c.pc,c.gpr[3],c.gpr[4]);c.gpr[3]=0xcafef00du;c.pc=c.lr;continue;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"Oracle exception %u at %08x\n",c.exception,c.pc);return 0;}}
 if(result)*result=c.gpr[3];return 1;}
static int same(const char *phase,uint32_t lo){
 if(!memcmp(ram,oracle,lo-BASE)&&!memcmp(at(ram,HI),at(oracle,HI),BYTES-(HI-BASE)))return 1;
 for(uint32_t i=0;i<BYTES;++i)if((i<lo-BASE||i>=HI-BASE)&&ram[i]!=oracle[i]){
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
#define ORIG(pc,args,out) do{if(!original(pc,args,os,out))return 4;}while(0)
#define RETURN(g,w,p) do{if(!returns(g,w,p))return 7;}while(0)
#define SAME(p,lo) do{if(!same(p,lo))return 7;}while(0)
static void reset_services(service *n,service *o){memset(n,0,sizeof *n);memset(o,0,sizeof *o);n->ram=ram;o->ram=oracle;}
static int invalid(nflstreet_registry_init_context x){
 at(ram,FLAG)[0]=0;uint8_t *before=malloc(BYTES);if(!before)return 0;memcpy(before,ram,BYTES);unsigned count=0;uint32_t out;
#define REJECT(e) do{out=123;if((e)||out){fprintf(stderr,"Host guard failed line %d\n",__LINE__);free(before);return 0;}++count;}while(0)
 REJECT(nflstreet_registry_init(&x,0xffffffffu,&out));REJECT(nflstreet_registry_open(&x,0x80000000u,&out));
 nflstreet_registry_init_context bad=x;bad.count_slot=COUNT+1;
 REJECT(nflstreet_registry_init(&bad,1,&out));bad=x;bad.root_slot=BASE+BYTES-2;
 REJECT(nflstreet_registry_init(&bad,1,&out));bad=x;bad.flag_slot=BASE+BYTES;
 REJECT(nflstreet_registry_open(&bad,1,&out));bad=x;bad.services=NULL;
 REJECT(nflstreet_registry_init(&bad,1,&out));REJECT(nflstreet_registry_open(&bad,1,&out));
 if(memcmp(before,ram,BYTES)){free(before);return 0;}free(before);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<32||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||
 module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);
 if(!saved||!ram||!oracle||!ns||!os||at(saved,FLAG)[0]!=1)return 3;
 unsigned blocked=0,opened=0,failed=0,lowlevel=0,lowfailure=0,zeros=0,compositions=0,constructors=0,error_zero=0,error_high=0;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);reset_services(ns,os);
  nflstreet_registry_init_services services={ns,heap,get_error};
  nflstreet_registry_init_context x={{ram,BYTES,BASE},COUNT,ROOT,FLAG,&services};
  uint32_t args[8]={30,0,0,0,0,0,0,0},got,want;
  if(!current){
   memcpy(oracle,ram,BYTES);HOST(nflstreet_registry_open(&x,args[0],&got));
   ORIG(0x80195368u,args,&want);RETURN(got,want,"saved wrapper");SAME("saved wrapper",INIT_LO);++blocked;
  }else{
   for(unsigned i=0;i<TABLE_BYTES+32;++i){at(ram,TABLE)[i]=(uint8_t)((rnd()>>17)|1u);at(ram,TABLE2)[i]=(uint8_t)((rnd()>>17)|1u);}
   static const uint32_t counts[]={0,1,2,3,14,30,31,254,255,256,257,511,512};
   uint32_t n=counts[(current-1)%13],n2=counts[(current+4)%13];
   at(ram,FLAG)[0]=current%4?0:(uint8_t)(current%255+1);
   args[0]=at(ram,FLAG)[0]?0xffffffffu:n;
   ns->responses[0]=os->responses[0]=current%5?TABLE+(n==0):0;
   ns->responses[1]=os->responses[1]=TABLE2+(n==0);
   ns->table_size[0]=os->table_size[0]=n*20u;ns->table_size[1]=os->table_size[1]=n*20u;
   static const uint32_t errors[]={0,0xffffffffu,0x80000000u,0x1234u};ns->get_response=os->get_response=errors[(current/7)%4];
   memcpy(oracle,ram,BYTES);
   for(unsigned repeat=0;repeat<2;++repeat){
    unsigned initialized=at(ram,FLAG)[0];unsigned requests=ns->heaps;
    HOST(nflstreet_registry_open(&x,args[0],&got));ORIG(0x80195368u,args,&want);RETURN(got,want,"wrapper repeat");SAME("wrapper repeat",INIT_LO);
    if(initialized)++blocked;else if(at(ram,FLAG)[0])++opened;else {++failed;if(!ns->get_response)++error_zero;if(ns->get_response&0x80000000u)++error_high;}
    if(!initialized&&ns->heaps>requests&&ns->responses[requests]&&n==0)++zeros;
    if(!traces(ns,os))return 8;
   }
   /* Low-level reinitialization deliberately bypasses an already-set flag. */
   reset_services(ns,os);ns->responses[0]=os->responses[0]=current%7?TABLE2+(n2==0):0;
   ns->responses[1]=os->responses[1]=TABLE+(n==0);
   ns->table_size[0]=os->table_size[0]=n*20u;ns->table_size[1]=os->table_size[1]=n2*20u;
   uint32_t sizes[2]={n2,n};
   for(unsigned repeat=0;repeat<2;++repeat){args[0]=sizes[repeat];
    HOST(nflstreet_registry_init(&x,args[0],&got));ORIG(0x80195234u,args,&want);RETURN(got,want,"low-level reinit");SAME("low-level reinit",INIT_LO);
    if(got){++lowlevel;if(!args[0])++zeros;}else ++lowfailure;if(!traces(ns,os))return 8;}
   if(current%16==0){
    /* A fresh controlled stage on the same RAM, not a claimed boot path. */
    at(ram,FLAG)[0]=0;at(oracle,FLAG)[0]=0;wr(at(ram,ALIGNMENT),4);wr(at(oracle,ALIGNMENT),4);
    for(unsigned i=0;i<256;++i){at(ram,POOL)[i]=at(oracle,POOL)[i]=(uint8_t)((rnd()>>17)|1u);
     at(ram,SCENE)[i]=at(oracle,SCENE)[i]=(uint8_t)((rnd()>>17)|1u);at(ram,LIST)[i]=at(oracle,LIST)[i]=(uint8_t)((rnd()>>17)|1u);}
    reset_services(ns,os);ns->composed=os->composed=1;
    unsigned count=1+(current/16)%4;ns->scene_size=os->scene_size=40+count*4;
    ns->table_size[0]=os->table_size[0]=count*20;ns->responses[0]=os->responses[0]=TABLE;
    ns->responses[1]=os->responses[1]=POOL;ns->responses[2]=os->responses[2]=SCENE;ns->responses[3]=os->responses[3]=LIST;
    args[0]=count;HOST(nflstreet_registry_open(&x,count,&got));ORIG(0x80195368u,args,&want);RETURN(got,want,"composed open");SAME("composed open",INIT_LO);
    nflstreet_registered_services rs={{ns,heap,error},get_error,construct};
    nflstreet_registered_context rx={{ram,BYTES,BASE},ROOT,4,count,&rs};
    uint32_t ctor=(current/16)%2?CTOR:0,registration[8]={0,32,2,ctor,0,0,0,0};
    HOST(nflstreet_registered_register(&rx,0,32,2,ctor,0,&got));ORIG(0x80195400u,registration,&want);RETURN(got,want,"composed register");SAME("composed register",COMPOSE_LO);
    nflstreet_scene_owner_context sx={{ram,BYTES,BASE},0x80389e88u,4,count,&rs.pool};
    uint32_t scene_args[8]={1,2,0,0,0,1,0,0xffffffffu};
    HOST(nflstreet_scene_owner_create(&sx,1,2,0,0,0,1,0,0xffffffffu,&got));ORIG(0x801954e4u,scene_args,&want);RETURN(got,want,"composed scene");SAME("composed scene",COMPOSE_LO);
    nflstreet_factory_record scratch[4];
    for(unsigned k=0;k<3;++k){uint32_t actor_args[8]={0,(uint8_t)rnd(),rnd(),0,0,0,0,0};
     HOST(nflstreet_registered_create(&rx,scratch,4,0,(uint8_t)actor_args[1],actor_args[2],&got));
     ORIG(0x801956a4u,actor_args,&want);RETURN(got,want,"composed factory");SAME("composed factory",COMPOSE_LO);}
    if(ctor)constructors+=2;if(!traces(ns,os))return 8;++compositions;
   }
  }
  if(!traces(ns,os))return 8;
  uint32_t lo=current&&current%16==0?COMPOSE_LO:INIT_LO;
  digest=hash(digest,ram,lo-BASE);digest=hash(digest,at(ram,HI),BYTES-(HI-BASE));
 }
 reset_services(ns,os);int rejected=invalid((nflstreet_registry_init_context){{ram,BYTES,BASE},COUNT,ROOT,FLAG,
 &(nflstreet_registry_init_services){ns,heap,get_error}});
 if(rejected!=7||!blocked||!opened||!failed||!lowlevel||!lowfailure||!zeros||!compositions||!constructors||!error_zero||!error_high)return 9;
 printf("Original registry init: %u cases (1 saved + %u edited); guarded blocked/open/fail %u/%u/%u; failure-errors zero/high %u/%u; low-level success/fail %u/%u; zero-count publications %u; composed stages/constructors %u/%u; 7 host guards; returns, ordered services and all RAM outside phase stacks [80f46fe8,80f47008) or [80f46f60,80f47008) match; digest %08x.\n",
 edited+1,edited,blocked,opened,failed,error_zero,error_high,lowlevel,lowfailure,zeros,compositions,constructors,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
