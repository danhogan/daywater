/* Original view/callback/list/pool execute unhooked.
   Heap and error alone return early with controlled, identically traced responses. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "scene_owner.h"
#include "view_nodes.h"
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
 uint32_t get_response;uint32_t responses[2],scene_size,list_size,list_address;
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
static uint32_t get_error(void *u){service *s=u;note(s,0x801af630u,0,0,0,s->get_response);return s->get_response;}
static int original(uint32_t pc,const uint32_t args[8],service *s,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=pc;
 for(unsigned i=0;i<8;++i)c.gpr[3+i]=args[i];
 for(unsigned step=0;c.pc;++step){
  if(step>=1000000){fprintf(stderr,"Oracle timeout %08x\n",c.pc);return 0;}
  if(c.pc==0x8018b7e4u){c.gpr[3]=heap(s,c.gpr[3],c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  if(c.pc==0x801af57cu){error(s,c.gpr[3]);c.gpr[3]=0xdeadf00du;c.pc=c.lr;continue;}
  if(c.pc==0x801af630u){c.gpr[3]=get_error(s);c.pc=c.lr;continue;}
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
#define VIEWS (0x803a9a40u-0x638cu)
#define CALLBACKS (0x803a9a40u-0x6388u)
#define DISPLAY 0x8039e07cu
#define CONSTANTS 0x80263cc8u
static void reset(service *n,service *o){
 uint32_t reply=n->get_response;memset(n,0,sizeof *n);memset(o,0,sizeof *o);n->ram=ram;o->ram=oracle;
 n->get_response=o->get_response=reply;n->list_address=o->list_address=LIST;n->list_size=o->list_size=LIST_BYTES;n->scene_size=o->scene_size=SCENE_BYTES;}
static void half(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static int guards(nflstreet_view_nodes_context x){
 uint8_t *copy=malloc(BYTES);if(!copy)return 0;unsigned count=0;uint32_t out;memcpy(copy,ram,BYTES);
#define REJECT(e) do{out=123;if((e)||out){free(copy);return 0;}++count;}while(0)
 nflstreet_view_nodes_context b=x;b.lists.services=NULL;REJECT(nflstreet_view_node_create(&b,OUTPUT,&out));
 b=x;b.constants=BASE+BYTES-4;REJECT(nflstreet_view_node_create(&b,OUTPUT,&out));
 b=x;b.display_word=DISPLAY+1;REJECT(nflstreet_view_node_create(&b,OUTPUT,&out));
 b=x;b.view_list_slot=BASE+BYTES;REJECT(nflstreet_view_node_create(&b,OUTPUT,&out));
 REJECT(nflstreet_view_node_create(&x,OUTPUT+1,&out));
 b=x;b.get_error=NULL;REJECT(nflstreet_view_callback_register(&b,1,2,3,4,&out));
 if(memcmp(copy,ram,BYTES)){free(copy);return 0;}
 at(ram,OUTPUT)[3]=51;memcpy(copy,ram,BYTES);REJECT(nflstreet_view_node_create(&x,OUTPUT,&out));
 int32_t result=123;if(nflstreet_view_node_compare(&x.lists.ram,0x1234,OUTPUT,OUTPUT+4,&result)||result){free(copy);return 0;}++count;
 if(memcmp(copy,ram,BYTES)){free(copy);return 0;}free(copy);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<40||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);if(!saved||!ram||!oracle||!ns||!os)return 3;
 uint32_t saved_callbacks=rd(at(saved,CALLBACKS));if(saved_callbacks<BASE||saved_callbacks>BASE+BYTES-32||rd(at(saved,saved_callbacks+16))!=0x801dee3cu)return 3;
 unsigned views=0,view_zero=0,callbacks=0,callback_zero=0,error_zero=0,large_count=0,comparisons=0,wrapped=0,heap_zero=0;
 nflstreet_pool_services ps={ns,heap,error};nflstreet_pool_ram memory={ram,BYTES,BASE};
 nflstreet_view_nodes_context x={{memory,TABLE,&ps,NULL,NULL},VIEWS,CALLBACKS,DISPLAY,CONSTANTS,get_error};
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);ns->get_response=current%3==0?0:current%3==1?0x80000000u:0x12345678u;reset(ns,os);
  uint32_t got,want;
  if(!current){uint32_t args[8]={0x1234,0xdeadbeef,0x80000000,7,0,0,0,0};memcpy(oracle,ram,BYTES);stack_lo=STACK-96u;
   HOST(nflstreet_view_callback_register(&x,args[0],args[1],args[2],args[3],&got));ORIG(0x801df39cu,args,&want);RETURN(got,want,"saved callback");SAME("saved callback");if(!traces(ns,os))return 8;continue;}
  for(unsigned j=0;j<LIST_BYTES+8;++j)at(ram,LIST-4)[j]=(uint8_t)((rnd()>>17)|1u);
  for(unsigned j=0;j<SCENE_BYTES+8;++j)at(ram,SCENE-4)[j]=(uint8_t)((rnd()>>17)|1u);
  for(unsigned j=0;j<1024;++j)at(ram,OUTPUT)[j]=(uint8_t)((rnd()>>17)|1u);
  uint32_t alignment=4u<<((current/9)%4);wr(at(ram,ALIGNMENT),alignment);
  nflstreet_scene_owner_context owner={memory,TABLE,alignment,30,&ps};
  static const uint32_t vc[]={0,1,2,3,8,14,255,256,257},cc[]={0,1,2,3,8,14};
  uint32_t capacities[2]={vc[(current-1)%9],cc[(current-1)%6]},lists[2]={0,0};
  for(unsigned j=0;j<2;++j){reset(ns,os);uint32_t address=j?SCENE:LIST;
   ns->responses[0]=os->responses[0]=(current%(j?19:17)==0)?0:address;memcpy(oracle,ram,BYTES);stack_lo=STACK-112u;
   uint32_t args[8]={1,0,capacities[j],j?16:0xf8,j?0x801dee3c:0x801def40,0,0,0};
   HOST(nflstreet_scene_list_create(&owner,args[0],args[1],args[2],args[3],args[4],0,&got));ORIG(0x801805e0u,args,&want);RETURN(got,want,"fresh lists");SAME("fresh lists");if(!traces(ns,os))return 8;
   lists[j]=got;if(!got)++heap_zero;wr(at(ram,j?CALLBACKS:VIEWS),got);wr(at(oracle,j?CALLBACKS:VIEWS),got);}
  static const uint32_t screens[]={0,1,480,640,0x7fffffffu,0x80000000u,0xffffffffu,0x1000001u};
  static const uint16_t dimensions[]={0,1,479,480,65535,255,640,32768};
  static const uint8_t counts[]={0,1,2,3,4,5,16,49,50};
  wr(at(ram,DISPLAY),screens[(current-1)%8]);wr(at(oracle,DISPLAY),screens[(current-1)%8]);
  if(lists[0])for(unsigned k=0;k<4;++k){
   reset(ns,os);uint16_t key=(uint16_t)(k==0?4:k==1?9:k==2?0:4);half(at(ram,OUTPUT),key);
   at(ram,OUTPUT)[3]=counts[(current+k)%9];half(at(ram,OUTPUT+16),dimensions[(current+k)%8]);half(at(ram,OUTPUT+18),dimensions[(current+k+3)%8]);
   memcpy(at(oracle,OUTPUT),at(ram,OUTPUT),208);stack_lo=STACK-112u;uint32_t args[8]={OUTPUT,0,0,0,0,0,0,0};
   HOST(nflstreet_view_node_create(&x,OUTPUT,&got));ORIG(0x801df14cu,args,&want);RETURN(got,want,"view create");SAME("view create");if(!traces(ns,os))return 8;
   if(got){++views;if(at(ram,OUTPUT)[3]>3)++large_count;}else ++view_zero;}
  if(lists[1])for(unsigned k=0;k<5;++k){
   reset(ns,os);static const uint32_t keys[]={0,0xffffffffu,0x80000000u,0x7fffffffu,0};
   uint32_t args[8]={rnd(),rnd(),rnd(),keys[k],0,0,0,0};stack_lo=STACK-96u;
   HOST(nflstreet_view_callback_register(&x,args[0],args[1],args[2],args[3],&got));ORIG(0x801df39cu,args,&want);RETURN(got,want,"callback register");SAME("callback register");if(!traces(ns,os))return 8;
   /* Pool errors distinguish successful return0 from exhaustion/error0. */
   if(ns->count&&ns->e[0].a==0x30003){++callback_zero;if(!got)++error_zero;}else ++callbacks;}
  for(unsigned k=0;k<4;++k){
   static const uint32_t values[]={0,1,0x7fffffffu,0x80000000u,0xffffffffu,0x80000001u,0x1000001u,0xfffffffeu};
   uint32_t left=values[(current+k)%8],right=values[(current+3*k+1)%8],function=k%2?0x801dee3cu:0x801def40u;
   wr(at(ram,OUTPUT+512),left);wr(at(ram,OUTPUT+516),right);wr(at(oracle,OUTPUT+512),left);wr(at(oracle,OUTPUT+516),right);
   int32_t order;HOST(nflstreet_view_node_compare(&memory,function,OUTPUT+512,OUTPUT+516,&order));memcpy(&got,&order,4);
   uint32_t args[8]={OUTPUT+512,OUTPUT+516,0,0,0,0,0,0};stack_lo=HI;ORIG(function,args,&want);RETURN(got,want,"raw comparator");SAME("raw comparator");++comparisons;
   if(((left^right)&0x80000000u)&&((left^got)&0x80000000u))++wrapped;}
  digest=hash(digest,at(ram,LIST),LIST_BYTES);digest=hash(digest,at(ram,SCENE),SCENE_BYTES);
 }
 if(guards(x)!=8||!views||!view_zero||!callbacks||!callback_zero||!error_zero||!large_count||!comparisons||!wrapped||!heap_zero)return 9;
 printf("Original view nodes: %u cases (1 saved + %u edited); view/empty/callback/exhaust/error-zero %u/%u/%u/%u/%u; large-copy/compare/wrap/heap-zero %u/%u/%u/%u; 8 host guards; returns, ordered services and all RAM outside phase stacks match; digest %08x.\n",edited+1,edited,views,view_zero,callbacks,callback_zero,error_zero,large_count,comparisons,wrapped,heap_zero,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
