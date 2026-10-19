/* Original list owner/allocation/publish/default-comparator/pool execute unhooked.
   Heap and error alone return early with controlled, identically traced responses. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "scene_owner.h"
#include "scene_insert.h"
#include "list_nodes.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
static uint32_t stack_lo;
#define LO stack_lo
#define HI (STACK+8u)
#define TABLE 0x80389e88u
#define ALIGNMENT (0x803a9a40u-0x66a8u)
#define COUNT (0x803a9a40u-0x4f18u)
#define SCENE 0x81000004u
#define LIST 0x81100004u
#define OUTPUT 0x81200000u
#define DIRECT 0x81300004u
#define SCENE_BYTES 8192u
#define LIST_BYTES 0x30000u
typedef struct {
 uint32_t pc,a,b,c,result;
 uint8_t scene[SCENE_BYTES],list[LIST_BYTES];
} event;
typedef struct {
 uint8_t *ram;event e[8];unsigned count,heaps,bad;
 uint32_t responses[2],scene_size,list_size,list_address;
} service;
static uint8_t *oracle,*ram;
static const ModernGekkoModuleDesc *module;
static unsigned current;
static uint32_t seed=0xbd31303u,digest=2166136261u;
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(uint8_t *r,uint32_t a){return r+(a-BASE);}
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static void note(service *s,uint32_t pc,uint32_t a,uint32_t b,uint32_t c,uint32_t result){
 if(s->count>=8||s->scene_size>SCENE_BYTES||s->list_size>LIST_BYTES){s->bad=1;return;}
 event *e=&s->e[s->count++];memset(e,0,sizeof *e);e->pc=pc;e->a=a;e->b=b;e->c=c;e->result=result;
 if(s->scene_size)memcpy(e->scene,at(s->ram,SCENE),s->scene_size);
 if(s->list_size)memcpy(e->list,at(s->ram,s->list_address),s->list_size);}
static uint32_t heap(void *u,uint32_t a,uint32_t b,uint32_t c){service *s=u;
 if(s->heaps>=2){s->bad=1;return 0;}uint32_t result=s->responses[s->heaps++];note(s,0x8018b7e4u,a,b,c,result);return result;}
static void error(void *u,uint32_t code){note(u,0x801af57cu,code,0,0,0);}
static int original(uint32_t pc,const uint32_t args[8],service *s,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=pc;
 for(unsigned i=0;i<8;++i)c.gpr[3+i]=args[i];
 for(unsigned step=0;c.pc;++step){
  if(step>=1000000){fprintf(stderr,"Oracle timeout %08x\n",c.pc);return 0;}
  if(c.pc==0x8018b7e4u){c.gpr[3]=heap(s,c.gpr[3],c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  if(c.pc==0x801af57cu){error(s,c.gpr[3]);c.gpr[3]=0xdeadf00du;c.pc=c.lr;continue;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"Oracle exception %u at %08x\n",c.exception,c.pc);return 0;}}
 if(result)*result=c.gpr[3];return 1;}
static int same(const char *phase){
 if(!memcmp(ram,oracle,LO-BASE)&&!memcmp(at(ram,HI),at(oracle,HI),BYTES-(HI-BASE))){
 memcpy(at(ram,LO),at(oracle,LO),HI-LO);return 1;}
 for(uint32_t i=0;i<BYTES;++i)if((i<LO-BASE||i>=HI-BASE)&&ram[i]!=oracle[i]){
  fprintf(stderr,"MISMATCH RAM case %u phase %s at %08x: original %02x native %02x\n",current,phase,BASE+i,oracle[i],ram[i]);return 0;}
 return 0;}
static int returns(uint32_t got,uint32_t want,const char *phase){
 if(got==want)return 1;fprintf(stderr,"MISMATCH RETURN case %u phase %s original %08x native %08x\n",current,phase,want,got);return 0;}
static int traces(service *n,service *o){
 if(n->bad||o->bad||n->count!=o->count||memcmp(n->e,o->e,n->count*sizeof(event))){
  fprintf(stderr,"MISMATCH TRACE case %u native %u original %u\n",current,n->count,o->count);return 0;}
 for(unsigned i=0;i<n->count;++i){event *e=&n->e[i];digest=hash(digest,e,5*sizeof(uint32_t));
  digest=hash(digest,e->scene,n->scene_size);digest=hash(digest,e->list,n->list_size);}
 return 1;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *r=malloc(BYTES);
 if(!r||fread(r,1,BYTES,f)!=BYTES||fgetc(f)!=EOF){free(r);r=NULL;}fclose(f);return r;}
#define HOST(x) do{if(!(x)){fprintf(stderr,"Native rejected case %u line %d\n",current,__LINE__);return 5;}}while(0)
#define ORIG(pc,args,out) do{if(!original(pc,args,os,out))return 4;}while(0)
#define RETURN(g,w,p) do{if(!returns(g,w,p))return 7;}while(0)
#define SAME(p) do{if(!same(p))return 7;}while(0)
static int compare(void *u,uint32_t function,uint32_t left,uint32_t right,int32_t *result){
 if(function!=0x80194f04u)return 0;return nflstreet_scene_default_compare(*(nflstreet_pool_ram *)u,left,right,result);}
static void reset(service *n,service *o){
 memset(n,0,sizeof *n);memset(o,0,sizeof *o);n->ram=ram;o->ram=oracle;
 n->list_address=o->list_address=LIST;n->list_size=o->list_size=LIST_BYTES;n->scene_size=o->scene_size=SCENE_BYTES;
 n->responses[0]=o->responses[0]=LIST;}
static int guards(nflstreet_list_nodes_context x){
 uint8_t *copy=malloc(BYTES);if(!copy)return 0;unsigned count=0;uint32_t out;
 wr(at(ram,LIST),0);wr(at(ram,LIST+16),0);memcpy(copy,ram,BYTES);
#define REJECT(e) do{out=123;if((e)||out){free(copy);return 0;}++count;}while(0)
 nflstreet_list_nodes_context b=x;b.services=NULL;REJECT(nflstreet_list_node_allocate(&b,LIST,&out));
 b=x;b.type_table=BASE+BYTES-4;REJECT(nflstreet_list_node_allocate(&b,LIST,&out));
 REJECT(nflstreet_list_node_allocate(&x,LIST+1,&out));
 REJECT(nflstreet_list_node_publish(&x,LIST,0,&out));
 REJECT(nflstreet_list_node_publish(&x,LIST,BASE+BYTES+16,&out));
 REJECT(nflstreet_list_node_publish(&x,LIST,OUTPUT+1,&out));
 if(memcmp(copy,ram,BYTES)){free(copy);return 0;}
 wr(at(ram,LIST),1);memcpy(copy,ram,BYTES);REJECT(nflstreet_list_node_allocate(&x,LIST,&out));
 if(memcmp(copy,ram,BYTES)){free(copy);return 0;}
 wr(at(ram,LIST),0);wr(at(ram,LIST+16),0x12345678);x.compare=NULL;memcpy(copy,ram,BYTES);
 REJECT(nflstreet_list_node_publish(&x,LIST,OUTPUT,&out));
 if(memcmp(copy,ram,BYTES)){free(copy);return 0;}free(copy);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<40||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);
 if(!saved||!ram||!oracle||!ns||!os)return 3;
 unsigned allocs=0,exhausted=0,publishes=0,prepend=0,append=0,middle=0,ties=0,wraps=0,failed=0,startup=0;
 nflstreet_pool_ram memory={ram,BYTES,BASE};nflstreet_pool_services ps={ns,heap,error};
 nflstreet_list_nodes_context x={memory,TABLE,&ps,&memory,compare};
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);reset(ns,os);uint32_t got,want,args[8]={0};
  if(!current){uint32_t list=rd(at(ram,0x803a9a40u-0x638cu));args[0]=list;memcpy(oracle,ram,BYTES);stack_lo=STACK-40u;
   HOST(nflstreet_list_node_allocate(&x,list,&got));ORIG(0x80180700u,args,&want);RETURN(got,want,"saved allocate");SAME("saved allocate");if(!traces(ns,os))return 8;continue;}
  for(unsigned j=0;j<LIST_BYTES+8;++j)at(ram,LIST-4)[j]=(uint8_t)((rnd()>>17)|1u);
  for(unsigned j=0;j<SCENE_BYTES;++j)at(ram,SCENE)[j]=(uint8_t)((rnd()>>17)|1u);
  static const uint32_t caps[]={0,1,2,3,8,14,255,256,257};uint32_t cap=caps[(current-1)%9],a=4u<<((current/9)%4);
  uint32_t element=current%2?0xf8u:16u,callback=current%5?0x80194f04u:0;
  wr(at(ram,ALIGNMENT),a);nflstreet_scene_owner_context owner={memory,TABLE,a,30,&ps};
  if(current%17==0)ns->responses[0]=os->responses[0]=0;
  memcpy(oracle,ram,BYTES);stack_lo=STACK-112u;
  uint32_t setup[8]={1,0,cap,element,callback,0,0,0};
  HOST(nflstreet_scene_list_create(&owner,1,0,cap,element,callback,0,&got));ORIG(0x801805e0u,setup,&want);RETURN(got,want,"create");SAME("create");if(!traces(ns,os))return 8;
  if(!got){++failed;continue;}if(element==0xf8u)++startup;
  unsigned attempts=cap<8?cap+2:8;
  for(unsigned k=0;k<attempts;++k){
   reset(ns,os);uint32_t alloc[8]={LIST,0,0,0,0,0,0,0};stack_lo=STACK-40u;
   HOST(nflstreet_list_node_allocate(&x,LIST,&got));ORIG(0x80180700u,alloc,&want);RETURN(got,want,"allocate");SAME("allocate");if(!traces(ns,os))return 8;++allocs;
   if(!got){++exhausted;continue;}
   uint32_t object=SCENE+64u*k;static const uint16_t keys[]={0,0xffff,0x7fff,0x8000,1,0xfffe,2,0x8001};
   uint16_t key;switch((current/9)%4){case 0:key=keys[k];break;case 1:key=(uint16_t)k;break;case 2:key=(uint16_t)(8-k);break;default:key=0x8000;break;}
   at(ram,object+16)[0]=(uint8_t)(key>>8);at(ram,object+16)[1]=(uint8_t)key;
   wr(at(ram,got),object);memcpy(at(oracle,object),at(ram,object),64);wr(at(oracle,got),object);
   /* Caller writes payload only. Header/padding/pool state stay under comparison. */
   uint32_t head=rd(at(ram,LIST+24)),tail=rd(at(ram,LIST+28)),oldcount=rd(at(ram,LIST+4));
   if(k==0&&current%7==0){oldcount=0xffffffffu;wr(at(ram,LIST+4),oldcount);wr(at(oracle,LIST+4),oldcount);}
   uint32_t pub[8]={LIST,got,0,0,0,0,0,0},payload=got;reset(ns,os);stack_lo=STACK-64u;
   HOST(nflstreet_list_node_publish(&x,LIST,payload,&got));ORIG(0x80180784u,pub,&want);RETURN(got,want,"publish");SAME("publish");if(!traces(ns,os))return 8;++publishes;
   if(rd(at(ram,LIST+4))<oldcount)++wraps;
   uint32_t raw=payload-16;
   if(rd(at(ram,LIST+24))==raw)++prepend;else if(rd(at(ram,LIST+28))==raw)++append;else ++middle;
   if(callback&&head){uint32_t object0=rd(at(ram,head+16));if(at(ram,object0+16)[0]==(uint8_t)(key>>8)&&at(ram,object0+16)[1]==(uint8_t)key)++ties;}
   if(tail&&tail==raw)return 9;
   digest=hash(digest,at(ram,LIST),LIST_BYTES);
  }
 }
 if(guards(x)!=8||!allocs||!exhausted||!publishes||!prepend||!append||!middle||!ties||!wraps||!failed||!startup)return 9;
 printf("Original list nodes: %u cases (1 saved + %u edited); allocate/exhaust/publish %u/%u/%u; prepend/append/middle/tie %u/%u/%u/%u; wrap/heap-zero/view-size %u/%u/%u; 8 host guards; returns, ordered services and all RAM outside phase stacks match; digest %08x.\n",edited+1,edited,allocs,exhausted,publishes,prepend,append,middle,ties,wraps,failed,startup,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
