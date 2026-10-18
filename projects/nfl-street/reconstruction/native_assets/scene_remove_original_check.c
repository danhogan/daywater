/* Default comparator and all registry/pool/scene/list/node/factory routines
   execute in the untouched DLL. Only explicit external services return early. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "scene_remove.h"
#include "scene_owner.h"
#include "registry_init.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define SETUP_LO (STACK-0xa0u)
#define REMOVE_LO (STACK-0x58u)
#define REORDER_LO (STACK-0x70u)
#define FIND_LO (STACK-0x40u)
#define RAW_LO (STACK-8u)
#define INSERT_LO (STACK-0x50u)
#define ALLOC_LO (STACK-0x10u)
#define UNMARKED_LO (STACK-0x18u)
#define RAWPOOL 0x81500000u
#define TARGET 0x81501000u
#define HI (STACK+8u)
#define TYPE_TABLE 0x80389e88u
#define ALIGNMENT (0x803a9a40u-0x66a8u)
#define ROOT (0x803a9a40u-0x4f14u)
#define COUNT (0x803a9a40u-0x4f18u)
#define FLAG (0x803a9a40u-0x6568u)
#define SCENE 0x81000004u
#define LIST 0x81100004u
#define OBJECTS 0x81200000u
#define REGISTRY 0x81300000u
#define ACTOR_POOL 0x81400004u
#define CUSTOM 0x80194fe4u
#define CTOR 0x800e1f7cu
#define LIST_BYTES 0x8000u
typedef struct {
 uint32_t pc,a,b,c,result,root,count;
 uint8_t flag[8],scene[256],list[LIST_BYTES],objects[512],registry[80],actor_pool[124],object[32];
} event;
typedef struct {
 uint8_t *ram;event e[128];unsigned count,heaps,bad,mode,comparisons,constructors;
 uint32_t responses[4],get_response,scene_size,list_size;
 unsigned composition;
} service;
static uint8_t *oracle,*ram;
static const ModernGekkoModuleDesc *module;
static unsigned current;
static uint32_t seed=0x13cad91u,digest=2166136261u;
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void half(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static uint8_t *at(uint8_t *r,uint32_t a){return r+(a-BASE);}
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t align(uint32_t v,uint32_t a){return(v-1u+a)&(0u-a);}
static uint32_t list_size(uint32_t cap,uint32_t a){return align(32,a)+align(24+cap*(cap<=255?1u:2u),a)+cap*(align(4,a)+16);}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static void note(service *s,uint32_t pc,uint32_t a,uint32_t b,uint32_t c,uint32_t result){
 if(s->count>=128||s->list_size>LIST_BYTES||s->scene_size>256){s->bad=1;return;}
 event *e=&s->e[s->count++];memset(e,0,sizeof *e);e->pc=pc;e->a=a;e->b=b;e->c=c;e->result=result;
 e->root=rd(at(s->ram,ROOT));e->count=rd(at(s->ram,COUNT));memcpy(e->flag,at(s->ram,FLAG-3),8);
 memcpy(e->scene,at(s->ram,SCENE),s->scene_size);memcpy(e->list,at(s->ram,LIST),s->list_size);
 memcpy(e->objects,at(s->ram,OBJECTS),512);
 if(s->composition){memcpy(e->registry,at(s->ram,REGISTRY),80);memcpy(e->actor_pool,at(s->ram,ACTOR_POOL),124);}
 if(pc==CTOR)memcpy(e->object,at(s->ram,a),32);}
static uint32_t heap(void *u,uint32_t a,uint32_t b,uint32_t c){service *s=u;
 if(s->heaps>=4){s->bad=1;return 0;}uint32_t result=s->responses[s->heaps++];note(s,0x8018b7e4u,a,b,c,result);return result;}
static uint32_t get_error(void *u){service *s=u;note(s,0x801af630u,0,0,0,s->get_response);return s->get_response;}
static void error(void *u,uint32_t code){note(u,0x801af57cu,code,0,0,0);}
static void construct(void *u,uint32_t fn,uint32_t object,uint32_t input){service *s=u;note(s,fn,object,input,0,0);++s->constructors;
 half(at(s->ram,object+16),(uint16_t)input);at(s->ram,object)[2]=(uint8_t)(input>>16);wr(at(s->ram,object+4),input^0xb12c9037u);}
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
  /* 80194F04 is intentionally never hooked. */
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
static void synchronize_stack(void){memcpy(at(ram,SETUP_LO),at(oracle,SETUP_LO),HI-SETUP_LO);}
static int invalid(nflstreet_scene_insert_context x){
 /* Explicit host-only unsupported callback fixture, not a game invocation. */
 wr(at(ram,SCENE+12),0x80194fe4u);wr(at(ram,RAWPOOL+4),0);uint8_t *before=malloc(BYTES);if(!before)return 0;memcpy(before,ram,BYTES);unsigned count=0;
#define REJECT(e) do{if(e){fprintf(stderr,"Host guard failed line %d\n",__LINE__);free(before);return 0;}++count;}while(0)
 REJECT(nflstreet_scene_remove(&x,SCENE,OBJECTS));REJECT(nflstreet_scene_reorder(&x,SCENE,OBJECTS,0));
 REJECT(nflstreet_scene_remove(&x,SCENE+1,OBJECTS));REJECT(nflstreet_scene_remove(&x,SCENE,BASE+BYTES));
 nflstreet_scene_insert_context bad=x;bad.type_table=BASE+BYTES-4;
 REJECT(nflstreet_scene_node_release(&bad,LIST,OBJECTS));REJECT(nflstreet_scene_node_release(&x,LIST,0));
 REJECT(nflstreet_scene_pool_release(x.ram,BASE+BYTES-4,0));
 REJECT(nflstreet_scene_pool_release(x.ram,RAWPOOL,0));
 if(memcmp(before,ram,BYTES)){free(before);return 0;}free(before);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<40||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||
 module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);
 if(!saved||!ram||!oracle||!ns||!os)return 3;
 unsigned removals=0,reorders=0,head=0,middle=0,tail=0,single=0,wraps=0,duplicates=0,reused=0,unmarked=0,raws=0,wide=0,absent=0,empty=0,compositions=0,actor_unchanged=0;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);reset_services(ns,os);nflstreet_pool_services ps={ns,heap,error};
  nflstreet_scene_insert_context x={{ram,BYTES,BASE},TYPE_TABLE,&ps,get_error,NULL};
  uint32_t cap=8,a=4,count=30;
  if(current){
   static const uint32_t caps[]={0,1,2,3,4,8,14,255,256,257};cap=caps[(current-1)%10];a=4u<<((current-1)%5);count=1+current%4;
   wr(at(ram,ALIGNMENT),a);wr(at(ram,COUNT),count);
   for(unsigned i=0;i<LIST_BYTES+32;++i)at(ram,LIST-4)[i]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned i=0;i<512;++i){at(ram,SCENE)[i]=(uint8_t)((rnd()>>17)|1u);at(ram,OBJECTS)[i]=(uint8_t)((rnd()>>17)|1u);}
  }
  static const uint16_t keys[]={0,0xffff,0x7fff,0x8000,1,0xfffe,2,0x8000};
  for(unsigned i=0;i<16;++i){half(at(ram,OBJECTS+32*i+16),current%3?keys[i%8]:0x8000);at(ram,OBJECTS+32*i)[2]=(uint8_t)(255u-i);}
  ns->scene_size=os->scene_size=40+count*4;ns->list_size=os->list_size=list_size(cap,a);
  ns->responses[0]=os->responses[0]=SCENE;ns->responses[1]=os->responses[1]=LIST;
  memcpy(oracle,ram,BYTES);nflstreet_scene_owner_context owner={{ram,BYTES,BASE},TYPE_TABLE,a,count,&ps};
  uint32_t args[8]={1,cap,0,0,0,1,0,0xffffffffu},got,want;
  HOST(nflstreet_scene_owner_create(&owner,1,cap,0,0,0,1,0,0xffffffffu,&got));ORIG(0x801954e4u,args,&want);RETURN(got,want,"setup scene");SAME("setup scene",SETUP_LO);
  unsigned members=cap<8?cap:8;
  for(unsigned k=0;k<members;++k){uint32_t object=OBJECTS+32u*(k%4==3?0:k),insert_args[8]={SCENE,object,0,0,0,0,0,0};
   HOST(nflstreet_scene_insert(&x,SCENE,object,&got));ORIG(0x8019573cu,insert_args,&want);RETURN(got,want,"setup insert");SAME("setup insert",SETUP_LO);
   if(k%4==3)++duplicates;}
  /* Search child is safe on absence/empty; scene removal is never called on
     those inputs because its original caller assumes a found payload. */
  synchronize_stack();uint32_t target,status;
  wr(at(ram,TARGET),OBJECTS+0x800);wr(at(oracle,TARGET),OBJECTS+0x800);
  HOST(nflstreet_scene_find(&x,LIST,OBJECTS+0x800,&target,&status));wr(at(ram,TARGET),target);
  uint32_t find_args[8]={LIST,0,TARGET,0,0x80195344u,1,0,0};
  ORIG(0x80180a14u,find_args,&want);RETURN(status,want,"absent search");SAME("absent search",FIND_LO);
  if(status==1)++empty;else if(!status)++absent;
  if(nflstreet_scene_remove(&x,SCENE,OBJECTS+0x800)){fprintf(stderr,"Absent host member accepted\n");return 9;}
  if(members){uint32_t first=rd(at(ram,LIST+24)),object=rd(at(ram,first+16));synchronize_stack();
   wr(at(ram,TARGET),object);wr(at(oracle,TARGET),object);HOST(nflstreet_scene_find(&x,LIST,object,&target,&status));wr(at(ram,TARGET),target);
   ORIG(0x80180a14u,find_args,&want);RETURN(status,want,"first present search");SAME("first present search",FIND_LO);}
  for(unsigned k=0;k<(members?3u:0u);++k){
   uint32_t first=rd(at(ram,LIST+24)),object=rd(at(ram,first+16));
   static const uint32_t orders[]={0xffffu,0,0x12348000u,0x10000u,0x8001u,0x7fffu};uint32_t order=orders[(current+k)%6];
   synchronize_stack();HOST(nflstreet_scene_reorder(&x,SCENE,object,order));uint32_t reorder_args[8]={SCENE,object,order,0,0,0,0,0};
   ORIG(0x801958e8u,reorder_args,NULL);SAME("reorder void",REORDER_LO);if(!traces(ns,os))return 8;++reorders;
  }
  unsigned removed_here=0;
  while(rd(at(ram,LIST+4))){
   uint32_t chosen=rd(at(ram,LIST+24)),size=rd(at(ram,LIST+4));
   if(removed_here%3==1)chosen=rd(at(ram,LIST+28));
   else if(removed_here%3==2)for(unsigned j=0;j<size/2;++j)chosen=rd(at(ram,chosen+12));
   uint32_t object=rd(at(ram,chosen+16));synchronize_stack();
   wr(at(ram,TARGET),object);wr(at(oracle,TARGET),object);
   HOST(nflstreet_scene_find(&x,LIST,object,&target,&status));wr(at(ram,TARGET),target);
   ORIG(0x80180a14u,find_args,&want);RETURN(status,want,"present search");SAME("present search",FIND_LO);
   uint32_t base=target-16,prev=rd(at(ram,base+8)),next=rd(at(ram,base+12));uint8_t before=at(ram,object)[2];
   if(!prev&&!next)++single;else if(!prev)++head;else if(!next)++tail;else ++middle;
   uint8_t stale[12];memcpy(stale,at(ram,base+8),12);synchronize_stack();
   HOST(nflstreet_scene_remove(&x,SCENE,object));uint32_t remove_args[8]={SCENE,object,0,0,0,0,0,0};
   ORIG(0x8019585cu,remove_args,NULL);SAME("remove void",REMOVE_LO);
   if(memcmp(stale,at(ram,base+8),12)){fprintf(stderr,"Freed node links/payload changed\n");return 9;}
   ++removals;if(!before)++wraps;++removed_here;
   if(removed_here==1){
    synchronize_stack();HOST(nflstreet_scene_insert(&x,SCENE,object,&got));ORIG(0x8019573cu,remove_args,&want);RETURN(got,want,"released slot reuse");SAME("released slot reuse",INSERT_LO);
    /* Last-pushed free index is immediately reused; remove its first matching
       object again, leaving the drain's net membership change at minus one. */
    uint32_t probe;HOST(nflstreet_scene_find(&x,LIST,object,&probe,&status));if(probe!=target){fprintf(stderr,"Released node not reused LIFO\n");return 9;}
    synchronize_stack();HOST(nflstreet_scene_remove(&x,SCENE,object));ORIG(0x8019585cu,remove_args,NULL);SAME("remove reused node",REMOVE_LO);++removals;++reused;
   }
   if(!traces(ns,os))return 8;
  }
  if(members){
   /* Raw unlinked-node release: allocate an index without node prefix init.
      Cleared flag skips unlink, preserving stale links/payload and listcount0. */
   uint32_t node_pool=rd(at(ram,LIST+20));synchronize_stack();HOST(nflstreet_pool_allocate(x.ram,node_pool,&ps,&got));
   uint32_t alloc_args[8]={node_pool,0,0,0,0,0,0,0};ORIG(0x801812d4u,alloc_args,&want);RETURN(got,want,"unmarked allocation");SAME("unmarked allocation",ALLOC_LO);
   if(rd(at(ram,got))&0x80000000u)return 9;
   synchronize_stack();HOST(nflstreet_scene_node_release(&x,LIST,got+16));uint32_t release_args[8]={LIST,got+16,0,0,0,0,0,0};
   ORIG(0x801808ecu,release_args,NULL);SAME("unmarked node release",UNMARKED_LO);++unmarked;
  }
  if(!traces(ns,os))return 8;
  /* Raw quotient/truncation helper: node token is arithmetic only and may be
     outside RAM. Index storage stays bounded; no huge pool is allocated. */
  for(unsigned k=0;k<2;++k){
   uint32_t widthcap=(current+k)%2?256:255,stride=(current+k)%3?3:17,data=0x81230000u;
   static const uint32_t quotients[]={65535,65536,65537};uint32_t address=data+quotients[(current+k)%3]*stride;
   if((current+k)%4==0)address=data-1u;
   for(unsigned j=0;j<64;++j)at(ram,RAWPOOL)[j]=(uint8_t)((rnd()>>17)|1u);
   wr(at(ram,RAWPOOL),widthcap);wr(at(ram,RAWPOOL+4),stride);wr(at(ram,RAWPOOL+8),0);
   wr(at(ram,RAWPOOL+16),RAWPOOL+32);wr(at(ram,RAWPOOL+20),data);
   memcpy(at(oracle,RAWPOOL),at(ram,RAWPOOL),64);synchronize_stack();
   HOST(nflstreet_scene_pool_release(x.ram,RAWPOOL,address));uint32_t raw_args[8]={RAWPOOL,address,0,0,0,0,0,0};
   ORIG(0x80181334u,raw_args,NULL);SAME("raw unsigned quotient",RAW_LO);
   HOST(nflstreet_pool_pop(x.ram,RAWPOOL,&got));raw_args[1]=0;ORIG(0x801810a4u,raw_args,&want);RETURN(got,want,"truncated index pop");SAME("truncated index pop",RAW_LO);
   ++raws;if(widthcap==256)++wide;
  }
  if(current&&current%16==0){
   reset_services(ns,os);ns->composition=os->composition=1;ns->scene_size=os->scene_size=48;ns->list_size=os->list_size=list_size(4,4);
   ns->responses[0]=os->responses[0]=REGISTRY;ns->responses[1]=os->responses[1]=ACTOR_POOL;ns->responses[2]=os->responses[2]=SCENE;ns->responses[3]=os->responses[3]=LIST;
   at(ram,FLAG)[0]=at(oracle,FLAG)[0]=0;wr(at(ram,ALIGNMENT),4);wr(at(oracle,ALIGNMENT),4);
   for(unsigned i=0;i<256;++i){at(ram,REGISTRY)[i]=at(oracle,REGISTRY)[i]=(uint8_t)((rnd()>>17)|1u);at(ram,ACTOR_POOL)[i]=at(oracle,ACTOR_POOL)[i]=(uint8_t)((rnd()>>17)|1u);}
   nflstreet_registry_init_services is={ns,heap,get_error};nflstreet_registry_init_context ix={{ram,BYTES,BASE},COUNT,ROOT,FLAG,&is};uint32_t init_args[8]={2,0,0,0,0,0,0,0};
   HOST(nflstreet_registry_open(&ix,2,&got));ORIG(0x80195368u,init_args,&want);RETURN(got,want,"composed init");
   nflstreet_registered_services rs={{ns,heap,error},get_error,construct};nflstreet_registered_context rx={{ram,BYTES,BASE},ROOT,4,2,&rs};
   uint32_t ctor=(current/16)%2?CTOR:0,reg_args[8]={0,32,3,ctor,0,0,0,0};
   HOST(nflstreet_registered_register(&rx,0,32,3,ctor,0,&got));ORIG(0x80195400u,reg_args,&want);RETURN(got,want,"composed register");
   nflstreet_scene_owner_context sx={{ram,BYTES,BASE},TYPE_TABLE,4,2,&ps};uint32_t scene_args[8]={1,4,0,0,0,1,0,0xffffffffu};
   HOST(nflstreet_scene_owner_create(&sx,1,4,0,0,0,1,0,0xffffffffu,&got));ORIG(0x801954e4u,scene_args,&want);RETURN(got,want,"composed scene");SAME("composed scene",SETUP_LO);
   nflstreet_factory_record scratch[2];uint32_t actors[3];
   for(unsigned k=0;k<3;++k){uint32_t input=(0xffu<<16)|(k%2?0xffffu:0x8000u),both_args[8]={SCENE,0,(uint8_t)rnd(),input,0,0,0,0};
    HOST(nflstreet_scene_create_insert(&x,&rx,scratch,2,SCENE,0,(uint8_t)both_args[2],input,&got));actors[k]=got;
    ORIG(0x801957a4u,both_args,&want);RETURN(got,want,"composed factory insert");SAME("composed factory insert",SETUP_LO);}
   uint8_t actor_allocator[28];memcpy(actor_allocator,at(ram,ACTOR_POOL),28);
   uint32_t duplicate_args[8]={SCENE,actors[0],0,0,0,0,0,0};HOST(nflstreet_scene_insert(&x,SCENE,actors[0],&got));ORIG(0x8019573cu,duplicate_args,&want);RETURN(got,want,"composed duplicate");
   for(unsigned k=0;k<3;++k){uint32_t reorder_args[8]={SCENE,actors[k],k?0xffffu:0,0,0,0,0,0};
    HOST(nflstreet_scene_reorder(&x,SCENE,actors[k],reorder_args[2]));ORIG(0x801958e8u,reorder_args,NULL);SAME("composed reorder void",SETUP_LO);}
   while(rd(at(ram,LIST+4))){uint32_t first=rd(at(ram,LIST+24)),object=rd(at(ram,first+16)),remove_args[8]={SCENE,object,0,0,0,0,0,0};
    HOST(nflstreet_scene_remove(&x,SCENE,object));ORIG(0x8019585cu,remove_args,NULL);SAME("composed remove void",SETUP_LO);
    if(memcmp(actor_allocator,at(ram,ACTOR_POOL),28)){fprintf(stderr,"Scene removal freed actor storage\n");return 9;}++actor_unchanged;}
   HOST(nflstreet_registered_create(&rx,scratch,2,0,0,0,&got));uint32_t actor_args[8]={0,0,0,0,0,0,0,0};
   ORIG(0x801956a4u,actor_args,&want);RETURN(got,want,"actor pool stays exhausted");if(got)return 9;SAME("actor pool stays exhausted",SETUP_LO);
   if(!traces(ns,os))return 8;++compositions;
  }
  digest=hash(digest,ram,SETUP_LO-BASE);digest=hash(digest,at(ram,HI),BYTES-(HI-BASE));
 }
 reset_services(ns,os);int rejected=invalid((nflstreet_scene_insert_context){{ram,BYTES,BASE},TYPE_TABLE,&(nflstreet_pool_services){ns,heap,error},get_error,NULL});
 if(rejected!=8||!removals||!reorders||!head||!middle||!tail||!single||!wraps||!duplicates||!reused||!unmarked||!raws||!wide||!absent||!empty||!compositions||!actor_unchanged)return 9;
 printf("Original scene removal: %u cases (1 controlled saved-input setup + %u edited); remove/reorder/wrap %u/%u/%u; head/middle/tail/single %u/%u/%u/%u; duplicate/reuse/unmarked %u/%u/%u; raw-release/wide %u/%u; absent/empty search %u/%u; compositions/actor-allocator-preserved %u/%u; 8 host guards; void entries have no guest return assertions; child returns, ordered services and all RAM match phase stacks; digest %08x.\n",
 edited+1,edited,removals,reorders,wraps,head,middle,tail,single,duplicates,reused,unmarked,raws,wide,absent,empty,compositions,actor_unchanged,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
