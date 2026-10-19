/* Original type0 producer/view bootstrap/list/matrix/scene execute unhooked.
   Heap/error/geterror alone return early with controlled, identically traced responses. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "scene_owner.h"
#include "view_system.h"

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
 uint8_t scene[SCENE_BYTES],list[LIST_BYTES],globals[128];
} event;
typedef struct {
 uint8_t *ram;event e[16];unsigned count,heaps,bad;
 uint32_t get_response;uint32_t responses[8],scene_size,list_size,list_address;
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
 if(s->count>=16||s->scene_size>SCENE_BYTES||s->list_size>LIST_BYTES){s->bad=1;return;}
 event *e=&s->e[s->count++];memset(e,0,sizeof *e);e->pc=pc;e->a=a;e->b=b;e->c=c;e->result=result;
 if(s->scene_size)memcpy(e->scene,at(s->ram,SCENE),s->scene_size);
 if(s->list_size)memcpy(e->list,at(s->ram,s->list_address),s->list_size);
 memcpy(e->globals,at(s->ram,0x803a9a40u-0x6394u),32);memcpy(e->globals+32,at(s->ram,0x803a9a40u-0x4ef0u),16);
 memcpy(e->globals+48,at(s->ram,0x803a9a40u-0x65f4u),16);memcpy(e->globals+64,at(s->ram,TABLE),64);}
static uint32_t heap(void *u,uint32_t a,uint32_t b,uint32_t c){service *s=u;
 if(s->heaps>=8){s->bad=1;return 0;}uint32_t result=s->responses[s->heaps++];note(s,0x8018b7e4u,a,b,c,result);return result;}
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
  digest=hash(digest,e->scene,n->scene_size);digest=hash(digest,e->list,n->list_size);digest=hash(digest,e->globals,sizeof e->globals);}
 return 1;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *r=malloc(BYTES);
 if(!r||fread(r,1,BYTES,f)!=BYTES||fgetc(f)!=EOF){free(r);r=NULL;}fclose(f);return r;}
#define HOST(x) do{if(!(x)){fprintf(stderr,"Native rejected case %u line %d\n",current,__LINE__);return 5;}}while(0)
#define ORIG(pc,args,out) do{if(!original(pc,args,os,out))return 4;}while(0)
#define RETURN(g,w,p) do{if(!returns(g,w,p))return 7;}while(0)
#define SAME(p) do{if(!same(p))return 7;}while(0)
#define R13 0x803a9a40u
#define VIEWS (R13-0x638cu)
#define CALLBACKS (R13-0x6388u)
#define INIT (R13-0x6390u)
#define ACTIVE (R13-0x4eecu)
#define INDEX (R13-0x4ee8u)
#define VIEWSCENE (R13-0x6380u)
#define MTABLE (LIST+0x2a000u)
#define VIEWPORT (LIST+0x28000u)
static void reset(service *n,service *o,uint32_t response){
 memset(n,0,sizeof *n);memset(o,0,sizeof *o);n->ram=ram;o->ram=oracle;n->get_response=o->get_response=response;
 n->list_address=o->list_address=LIST;n->list_size=o->list_size=LIST_BYTES;n->scene_size=o->scene_size=SCENE_BYTES;}
static void replies(service *n,service *o,unsigned mode){
 const uint32_t addresses[]={LIST,LIST+0x18000u,VIEWPORT,LIST+0x1e000u,LIST+0x1f000u};
 for(unsigned i=0;i<5;++i)n->responses[i]=o->responses[i]=addresses[i];
 if(mode>=1&&mode<=5)n->responses[mode-1]=o->responses[mode-1]=0;}
static int guards(nflstreet_view_system_context x){
 uint8_t *copy=malloc(BYTES);if(!copy)return 0;at(ram,INIT)[0]=0;memcpy(copy,ram,BYTES);uint32_t out;unsigned count=0;
#define REJECT(e) do{out=123;if((e)||out){free(copy);return 0;}++count;}while(0)
 nflstreet_view_system_context b=x;b.owner.services=NULL;REJECT(nflstreet_view_system_open(&b,OUTPUT,&out));
 b=x;b.r13++;REJECT(nflstreet_view_system_open(&b,OUTPUT,&out));
 b=x;b.r13=0;REJECT(nflstreet_view_system_open(&b,OUTPUT,&out));
 b=x;b.matrix=NULL;REJECT(nflstreet_view_system_open(&b,OUTPUT,&out));
 b=x;b.get_error=NULL;REJECT(nflstreet_view_system_open(&b,OUTPUT,&out));
 REJECT(nflstreet_view_system_open(&x,OUTPUT+1,&out));
 REJECT(nflstreet_view_system_open(&x,BASE+BYTES-4,&out));
 if(nflstreet_view_system_type0(x.owner.ram,TABLE+1,4)){free(copy);return 0;}++count;
 if(memcmp(copy,ram,BYTES)){free(copy);return 0;}free(copy);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<40||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);if(!saved||!ram||!oracle||!ns||!os)return 3;
 nflstreet_pool_ram memory={ram,BYTES,BASE};nflstreet_pool_services ps={ns,heap,error};nflstreet_matrix_storage_services ms={ns,heap,get_error,error};
 nflstreet_matrix_storage_context matrix={memory,R13-0x65f4u,R13-0x65f0u,R13-0x65ecu,R13-0x65e8u,0x80261800,0x80261804,&ms};
 nflstreet_view_system_context x={{memory,TABLE,4,30,&ps},&matrix,R13,get_error};
 unsigned producers=0,opens=0,blocked=0,failures[5]={0},error_zero=0,scene_zero=0,defaults=0,repeat=0,depth_zero=0;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);uint32_t response=current%3==0?0:current%3==1?0x80000000u:0x12345678u;reset(ns,os,response);
  uint32_t got,want,args[8]={0},descriptor=current?OUTPUT:0x80288ad8u;
  if(current){
   for(unsigned j=0;j<LIST_BYTES+8;++j)at(ram,LIST-4)[j]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned j=0;j<72;++j)at(ram,TABLE-4)[j]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned j=0;j<32;++j)at(ram,OUTPUT-4)[j]=(uint8_t)((rnd()>>17)|1u);
   static const uint32_t raw[]={0,1,2,3,4,16,32,64,0xffffffffu,0x80000000u};uint32_t alignment=raw[(current-1)%10];wr(at(ram,ALIGNMENT),alignment);memcpy(oracle,ram,BYTES);stack_lo=STACK-88u;
   HOST(nflstreet_view_system_type0(memory,TABLE,alignment));ORIG(0x80180fb4u,args,NULL);SAME("raw type0 registration");++producers;
   alignment=current%16==0?4:4u<<((current/8)%4);x.owner.alignment=alignment;x.owner.registry_count=current%16==0?30:current%4==0?0:current%4==1?1:current%4==2?7:30;
   wr(at(ram,ALIGNMENT),alignment);wr(at(oracle,ALIGNMENT),alignment);wr(at(ram,COUNT),x.owner.registry_count);wr(at(oracle,COUNT),x.owner.registry_count);
   HOST(nflstreet_view_system_type0(memory,TABLE,alignment));ORIG(0x80180fb4u,args,NULL);SAME("usable fresh type0 registration");++producers;
   at(ram,matrix.flag_slot)[0]=0;at(oracle,matrix.flag_slot)[0]=0;
   reset(ns,os,response);ns->responses[0]=os->responses[0]=MTABLE;args[0]=4;stack_lo=STACK-24u;
   HOST(nflstreet_matrix_storage_open(&matrix,4,&got));ORIG(0x8018908cu,args,&want);RETURN(got,want,"matrix table");SAME("matrix table");if(!traces(ns,os))return 8;
   for(unsigned k=0;k<3;++k){static const uint32_t indices[]={3,1,2};reset(ns,os,response);ns->responses[0]=os->responses[0]=LIST+0x29000u+0x400u*k;args[0]=indices[k];args[1]=12;stack_lo=STACK-16u;
    HOST(nflstreet_matrix_storage_allocate(&matrix,args[0],12,&got));ORIG(0x801891e8u,args,&want);RETURN(got,want,"startup matrix slots");SAME("startup matrix slots");if(!traces(ns,os))return 8;}
  }else memcpy(oracle,ram,BYTES);
  unsigned mode=current?(current-1)%8:6,standard=current&&current%16==0;if(standard)mode=0;
  if(current){static const uint32_t views[]={0,1,5,14,255,256,257},callbacks[]={0,1,6,14,255,256};static const uint8_t depths[]={0,1,2,3,12,16,32};
   uint32_t view=standard?5:views[(current-1)%7],callback=standard?6:callbacks[(current-1)%6],capacity=standard?40:mode==7?0:mode==5?0xffff:mode==4?0x8001:1+current%255;
   uint8_t index=standard?0:(uint8_t)(current%12),depth=standard?12:depths[(current-1)%7];
   wr(at(ram,OUTPUT),view);wr(at(ram,OUTPUT+4),callback);at(ram,OUTPUT+8)[0]=(uint8_t)(capacity>>8);at(ram,OUTPUT+9)[0]=(uint8_t)capacity;at(ram,OUTPUT+10)[0]=index;at(ram,OUTPUT+11)[0]=depth;
   at(ram,INIT)[0]=mode==6?(uint8_t)(1+current%255):0;wr(at(ram,VIEWS),0xfeed0000u+current);wr(at(ram,CALLBACKS),0xbeef0000u+current);wr(at(ram,VIEWSCENE),0xcafe0000u+current);wr(at(ram,ACTIVE),0xdead0000u+current);at(ram,INDEX)[0]=0xa7;
   memcpy(oracle,ram,BYTES);if(standard)++defaults;
  }
  reset(ns,os,response);replies(ns,os,mode);args[0]=descriptor;stack_lo=STACK-184u;
  HOST(nflstreet_view_system_open(&x,descriptor,&got));ORIG(0x801defd8u,args,&want);RETURN(got,want,"view system bootstrap");SAME("view system bootstrap");if(!traces(ns,os))return 8;
  ++opens;if(mode==6)++blocked;else{if(mode>=1&&mode<=5)++failures[mode-1];if(!response&&mode>=1&&mode<=3)++error_zero;if(mode==4&&rd(at(ram,VIEWSCENE))==0)++scene_zero;if(at(ram,OUTPUT+11)[0]==0&&mode!=1&&mode!=2)++depth_zero;}
  /* An attempted failure is still marked initialized. Repeated calls cannot
     silently allocate/retry, and do not even read this invalid descriptor. */
  reset(ns,os,response);uint32_t again[8]={0xfffffffcu,0,0,0,0,0,0,0};stack_lo=STACK-16u;
  HOST(nflstreet_view_system_open(&x,again[0],&got));ORIG(0x801defd8u,again,&want);RETURN(got,want,"repeated init");SAME("repeated init");if(!traces(ns,os))return 8;++repeat;
  digest=hash(digest,at(ram,LIST),LIST_BYTES);digest=hash(digest,at(ram,TABLE),64);digest=hash(digest,at(ram,INIT),32);digest=hash(digest,at(ram,ACTIVE),8);
 }
 if(guards(x)!=8||!producers||!opens||!blocked||!error_zero||!scene_zero||!defaults||!repeat||!depth_zero)return 9;
 for(unsigned i=0;i<5;++i)if(!failures[i])return 9;
 printf("Original view system: %u cases (1 saved + %u edited); producer/open/blocked %u/%u/%u; heap-fail stages %u/%u/%u/%u/%u; error-zero/scene-zero/default/repeat/depth-zero %u/%u/%u/%u/%u; 8 host guards; returns, ordered services and all RAM outside phase stacks match; digest %08x.\n",edited+1,edited,producers,opens,blocked,failures[0],failures[1],failures[2],failures[3],failures[4],error_zero,scene_zero,defaults,repeat,depth_zero,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
