/* Default comparator and all registry/pool/scene/list/node/factory routines
   execute in the untouched DLL. Only explicit external services return early. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "scene_insert.h"
#include "scene_owner.h"
#include "registry_init.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define SETUP_LO (STACK-0xa0u)
#define INSERT_LO (STACK-0x50u)
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
static int32_t compare(void *u,uint32_t fn,uint32_t left,uint32_t right){service *s=u;int32_t result;
 uint32_t l=rd(at(s->ram,left)),r=rd(at(s->ram,right));uint8_t *lp=at(s->ram,l+16),*rp=at(s->ram,r+16);
 unsigned a=(unsigned)lp[0]<<8|lp[1],b=(unsigned)rp[0]<<8|rp[1];
 switch(s->mode){case 0:result=INT32_MIN;break;case 1:result=INT32_MAX;break;case 2:result=0;break;
 default:result=a<b?INT32_MIN:a>b?INT32_MAX:0;break;}
 ++s->comparisons;note(s,fn,left,right,0,(uint32_t)result);return result;}
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
  if(c.pc==CUSTOM){c.gpr[3]=(uint32_t)compare(s,c.pc,c.gpr[3],c.gpr[4]);c.pc=c.lr;continue;}
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
static int invalid(nflstreet_scene_insert_context x){
 uint8_t *before=malloc(BYTES);if(!before)return 0;memcpy(before,ram,BYTES);unsigned count=0;uint32_t out;
#define REJECT(e) do{out=123;if((e)||out){fprintf(stderr,"Host guard failed line %d\n",__LINE__);free(before);return 0;}++count;}while(0)
 REJECT(nflstreet_scene_insert(&x,SCENE+1,OBJECTS,&out));
 REJECT(nflstreet_scene_insert(&x,SCENE,BASE+BYTES,&out));
 nflstreet_scene_insert_context bad=x;bad.type_table=BASE+BYTES-4;
 REJECT(nflstreet_scene_insert(&bad,SCENE,OBJECTS,&out));bad=x;bad.pool=NULL;
 REJECT(nflstreet_scene_insert(&bad,SCENE,OBJECTS,&out));bad=x;bad.get_error=NULL;
 REJECT(nflstreet_scene_insert(&bad,SCENE,OBJECTS,&out));
 REJECT(nflstreet_scene_create_insert(&x,NULL,NULL,0,SCENE,0,0,0,&out));
 int32_t order=123;if(nflstreet_scene_default_compare(x.ram,OBJECTS+1,OBJECTS,&order)||order){free(before);return 0;}++count;
 if(memcmp(before,ram,BYTES)){free(before);return 0;}free(before);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<40||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||
 module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);
 if(!saved||!ram||!oracle||!ns||!os)return 3;
 uint32_t registry=rd(at(saved,ROOT));if(registry<BASE||registry>BASE+BYTES-20)return 3;
 uint32_t pool=rd(at(saved,registry+16));if(pool<BASE||pool>BASE+BYTES-24)return 3;
 uint32_t saved_object=rd(at(saved,pool+20));if(saved_object<BASE||saved_object>BASE+BYTES-32)return 3;
 unsigned inserted=0,exhausted=0,wrapped=0,custom=0,defaults=0,no_compare=0,ties=0,head=0,middle=0,tail=0,composed=0,returned_on_failure=0;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);reset_services(ns,os);
  nflstreet_pool_services ps={ns,heap,error};
  nflstreet_scene_insert_context x={{ram,BYTES,BASE},TYPE_TABLE,&ps,get_error,compare};
  uint32_t a=4,cap=0x150,count=30,callback=0,args[8]={1,cap,0,0,0,1,0,0xffffffffu};
  if(current){
   for(unsigned i=0;i<LIST_BYTES+32;++i)at(ram,LIST-4)[i]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned i=0;i<512;++i){at(ram,SCENE)[i]=(uint8_t)((rnd()>>17)|1u);at(ram,OBJECTS)[i]=(uint8_t)((rnd()>>17)|1u);}
   static const uint32_t caps[]={0,1,2,3,4,8,14,255,256,257};cap=caps[(current-1)%10];a=4u<<((current-1)%5);count=1+(current%4);
   callback=current%3==0?CUSTOM:0;ns->mode=os->mode=(current/3)%4;
   wr(at(ram,ALIGNMENT),a);wr(at(ram,COUNT),count);args[1]=cap;args[3]=callback;
   static const uint16_t keys[]={0,0xffff,0x7fff,0x8000,1,0xfffe,2,0x8001};
   for(unsigned i=0;i<16;++i){uint16_t key;
    switch((current/10)%4){case 0:key=keys[i%8];break;case 1:key=(uint16_t)i;break;case 2:key=(uint16_t)(16-i);break;default:key=0x8000;break;}
    half(at(ram,OBJECTS+32*i+16),key);at(ram,OBJECTS+32*i)[2]=(uint8_t)(255u-i);}
  }
  ns->scene_size=os->scene_size=40+count*4;ns->list_size=os->list_size=list_size(cap,a);
  ns->responses[0]=os->responses[0]=SCENE;ns->responses[1]=os->responses[1]=LIST;
  ns->get_response=os->get_response=current%3==0?0:current%3==1?0x80000000u:0x1234u;
  memcpy(oracle,ram,BYTES);
  nflstreet_scene_owner_context owner={{ram,BYTES,BASE},TYPE_TABLE,a,count,&ps};uint32_t got,want;
  HOST(nflstreet_scene_owner_create(&owner,1,cap,0,callback,0,1,0,0xffffffffu,&got));
  ORIG(0x801954e4u,args,&want);RETURN(got,want,"scene setup");SAME("scene setup",SETUP_LO);
  /* Synchronize only the declared setup stack before narrower insertion phases. */
  memcpy(at(ram,SETUP_LO),at(oracle,SETUP_LO),HI-SETUP_LO);
  if(current&&current%11==0){wr(at(ram,LIST+16),0);wr(at(oracle,LIST+16),0);callback=0xffffffffu;}
  unsigned attempts=current?(cap<8?cap+2:10):1;
  for(unsigned k=0;k<attempts;++k){
   uint32_t object=current?OBJECTS+32u*(k%4==3?0:k%16):saved_object;
   unsigned before_count=rd(at(ram,LIST+4));uint8_t before_byte=at(ram,object)[2];
   uint32_t insert_args[8]={SCENE,object,0,0,0,0,0,0};
   HOST(nflstreet_scene_insert(&x,SCENE,object,&got));ORIG(0x8019573cu,insert_args,&want);RETURN(got,want,"insert");SAME("insert",INSERT_LO);
   if(rd(at(ram,LIST+4))!=before_count){
    ++inserted;if(before_byte==255)++wrapped;uint32_t node_pool=rd(at(ram,LIST+20));
    uint32_t base=rd(at(ram,node_pool+20))+(cap-1u-before_count)*rd(at(ram,node_pool+4));
    uint32_t prev=rd(at(ram,base+8)),next=rd(at(ram,base+12));
    if(!prev)++head;else if(!next)++tail;else ++middle;
    if(!callback)++defaults;else if(callback==CUSTOM)++custom;else ++no_compare;
    if(next){uint32_t other=rd(at(ram,next+16));uint8_t *left=at(ram,object+16),*right=at(ram,other+16);
     if(left[0]==right[0]&&left[1]==right[1])++ties;}
   }else ++exhausted;
   if(!traces(ns,os))return 8;
  }
  if(current){
   /* Direct default-comparator returns exercise both u16 boundaries and equal
      keys independently of any insertion branch. No comparator hook is used. */
   for(unsigned k=0;k<3;++k){uint16_t left=k==0?0:k==1?0xffff:0x8000,right=k==0?0xffff:k==1?0:0x8000;
    half(at(ram,OBJECTS+16),left);half(at(oracle,OBJECTS+16),left);half(at(ram,OBJECTS+48),right);half(at(oracle,OBJECTS+48),right);
    wr(at(ram,OBJECTS+0x300),OBJECTS);wr(at(oracle,OBJECTS+0x300),OBJECTS);wr(at(ram,OBJECTS+0x304),OBJECTS+32);wr(at(oracle,OBJECTS+0x304),OBJECTS+32);
    int32_t order;HOST(nflstreet_scene_default_compare(x.ram,OBJECTS+0x300,OBJECTS+0x304,&order));uint32_t cmp_args[8]={OBJECTS+0x300,OBJECTS+0x304,0,0,0,0,0,0};
    ORIG(0x80194f04u,cmp_args,&want);RETURN((uint32_t)order,want,"default comparator");SAME("default comparator",INSERT_LO);}
  }
  if(current&&current%16==0){
   /* Explicit fresh-table composition; original component calls remain unhooked. */
   reset_services(ns,os);ns->composition=os->composition=1;ns->scene_size=os->scene_size=48;uint32_t slots=(current/16)%3;
   ns->list_size=os->list_size=list_size(slots,4);ns->get_response=os->get_response=(current/16)%2?0:0x80000000u;
   ns->responses[0]=os->responses[0]=REGISTRY;ns->responses[1]=os->responses[1]=ACTOR_POOL;
   ns->responses[2]=os->responses[2]=SCENE;ns->responses[3]=os->responses[3]=LIST;
   at(ram,FLAG)[0]=at(oracle,FLAG)[0]=0;wr(at(ram,ALIGNMENT),4);wr(at(oracle,ALIGNMENT),4);
   for(unsigned i=0;i<256;++i){at(ram,REGISTRY)[i]=at(oracle,REGISTRY)[i]=(uint8_t)((rnd()>>17)|1u);at(ram,ACTOR_POOL)[i]=at(oracle,ACTOR_POOL)[i]=(uint8_t)((rnd()>>17)|1u);}
   nflstreet_registry_init_services is={ns,heap,get_error};nflstreet_registry_init_context ix={{ram,BYTES,BASE},COUNT,ROOT,FLAG,&is};uint32_t init_args[8]={2,0,0,0,0,0,0,0};
   HOST(nflstreet_registry_open(&ix,2,&got));ORIG(0x80195368u,init_args,&want);RETURN(got,want,"composed init");
   nflstreet_registered_services rs={{ns,heap,error},get_error,construct};nflstreet_registered_context rx={{ram,BYTES,BASE},ROOT,4,2,&rs};
   uint32_t ctor=(current/16)%2?CTOR:0,reg_args[8]={0,32,3,ctor,0,0,0,0};
   HOST(nflstreet_registered_register(&rx,0,32,3,ctor,0,&got));ORIG(0x80195400u,reg_args,&want);RETURN(got,want,"composed register");
   nflstreet_scene_owner_context sx={{ram,BYTES,BASE},TYPE_TABLE,4,2,&ps};uint32_t scene_args[8]={1,slots,0,0,0,1,0,0xffffffffu};
   HOST(nflstreet_scene_owner_create(&sx,1,slots,0,0,0,1,0,0xffffffffu,&got));ORIG(0x801954e4u,scene_args,&want);RETURN(got,want,"composed scene");SAME("composed scene",SETUP_LO);
   nflstreet_factory_record scratch[2];
   for(unsigned k=0;k<4;++k){uint32_t input=(0xffu<<16)|(k%2?0xffffu:0),both_args[8]={k==3?0:SCENE,0,(uint8_t)rnd(),input,0,0,0,0};
    unsigned before_count=rd(at(ram,LIST+4));
    HOST(nflstreet_scene_create_insert(&x,&rx,scratch,2,both_args[0],0,(uint8_t)both_args[2],input,&got));
    ORIG(0x801957a4u,both_args,&want);RETURN(got,want,"factory insert");SAME("factory insert",SETUP_LO);
    if(got&&rd(at(ram,LIST+4))==before_count)++returned_on_failure;
   }
   if(!traces(ns,os))return 8;++composed;
  }
  digest=hash(digest,ram,SETUP_LO-BASE);digest=hash(digest,at(ram,HI),BYTES-(HI-BASE));
 }
 reset_services(ns,os);int rejected=invalid((nflstreet_scene_insert_context){{ram,BYTES,BASE},TYPE_TABLE,
 &(nflstreet_pool_services){ns,heap,error},get_error,compare});
 if(rejected!=7||!inserted||!exhausted||!wrapped||!custom||!defaults||!no_compare||!ties||!head||!middle||!tail||!composed||!returned_on_failure)return 9;
 printf("Original scene insertion: %u cases (1 saved-input setup + %u edited); inserted/exhausted/wrapped %u/%u/%u; default/custom/none %u/%u/%u; head/middle/tail/ties %u/%u/%u/%u; composed/factory-return-on-failure %u/%u; 7 host guards; real returns, ordered services and all RAM match outside setup [80f46f60,80f47008) or insertion [80f46fb0,80f47008); digest %08x.\n",
 edited+1,edited,inserted,exhausted,wrapped,defaults,custom,no_compare,head,middle,tail,ties,composed,returned_on_failure,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
