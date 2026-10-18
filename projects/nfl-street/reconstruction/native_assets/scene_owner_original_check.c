/* Original wrapper/scene/list/layout/init/pool/memset execute unhooked.
   Heap and error alone return early with controlled, identically traced responses. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "scene_owner.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define LO (STACK-0xa8u)
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
static uint32_t align(uint32_t v,uint32_t a){return(v-1u+a)&(0u-a);}
static uint32_t fixture_layout(uint32_t cap,uint32_t elem,uint32_t a,uint32_t header,uint32_t extra){
 return align(header,a)+align(24u+cap*(cap<=255u?1u:2u),a)+cap*(align(elem,a)+extra);}
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
 if(!memcmp(ram,oracle,LO-BASE)&&!memcmp(at(ram,HI),at(oracle,HI),BYTES-(HI-BASE)))return 1;
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
static int invalid(nflstreet_scene_owner_context x){
 uint8_t *before=malloc(BYTES);if(!before)return 0;memcpy(before,ram,BYTES);unsigned count=0;uint32_t out=123;
#define REJECT(e) do{out=123;if((e)||out){fprintf(stderr,"Host guard failed line %d\n",__LINE__);free(before);return 0;}++count;}while(0)
 REJECT(nflstreet_scene_owner_create(&x,1,1,1,0,0,1,0,0,&out));
 REJECT(nflstreet_scene_list_create(&x,1,1,1,4,0,0,&out));
 nflstreet_scene_owner_context bad=x;bad.alignment=3;
 REJECT(nflstreet_scene_owner_create(&bad,1,1,0,0,0,1,0,0,&out));
 bad=x;bad.registry_count=0x80000000u;
 REJECT(nflstreet_scene_owner_create(&bad,1,1,0,0,0,0x80000000u,0,0,&out));
 bad=x;bad.type_table=BASE+BYTES-4;
 REJECT(nflstreet_scene_list_create(&bad,1,0,1,4,0,0,&out));
 bad=x;bad.services=NULL;
 REJECT(nflstreet_scene_owner_create(&bad,1,1,0,0,0,1,0,0,&out));
 REJECT(nflstreet_scene_list_create(&x,1,0,0xffffffffu,0xffffffffu,0,0,&out));
 REJECT(nflstreet_scene_list_create(&x,1,0,1,4,0,DIRECT+1,&out));
 if(memcmp(before,ram,BYTES)){free(before);return 0;}free(before);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<32||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||
 module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);
 if(!saved||!ram||!oracle||!ns||!os)return 3;
 static const uint32_t table[12]={0x80180c20u,0x80180c30u,0x80180d64u,0x80180c6cu,0x80180d8cu,0x80180e0cu,
 0x80180e6cu,0x80180f18u,0x80180facu,32,16,0x01000000u};
 for(unsigned i=0;i<12;++i)if(rd(at(saved,TABLE+4*i))!=table[i])return 3;
 static const uint8_t init[]={0x38,0,0,0,0x90,3,0,0x1c,0x90,3,0,0x18,0x4e,0x80,0,0x20};
 if(memcmp(at(saved,0x80180c20u),init,16)||rd(at(saved,ALIGNMENT))!=4||rd(at(saved,COUNT))!=30)return 3;
 unsigned scenes=0,scene_fail=0,list_fail=0,default_cb=0,custom_cb=0,zero_region=0,nonzero_region=0,wide=0,supplied=0;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);memset(ns,0,sizeof *ns);memset(os,0,sizeof *os);ns->ram=ram;os->ram=oracle;
  nflstreet_pool_services services={ns,heap,error};
  uint32_t args[8]={1,0x150,0,0,0,1,0,0xffffffffu};uint32_t a=4,count=30,header=32,extra=16;
  if(current){
   for(unsigned j=0;j<SCENE_BYTES+32;++j)at(ram,SCENE-4)[j]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned j=0;j<LIST_BYTES+32;++j){at(ram,LIST-4)[j]=(uint8_t)((rnd()>>17)|1u);at(ram,DIRECT-4)[j]=(uint8_t)((rnd()>>17)|1u);}
   static const uint32_t caps[]={0,1,2,14,254,255,256,257,336,511};
   static const uint32_t counts[]={0,1,2,14,30,255,256};
   args[0]=rnd();args[1]=caps[(current-1)%10];args[3]=current%2?0:0x80194f04u+(current%3)*4u;
   args[4]=rnd();args[5]=(current/7)%5;args[6]=rnd();args[7]=rnd();a=1u<<((current-1)%7);count=counts[(current-1)%7];
   /* Controlled descriptor edits exercise the original table-sourced layout;
      init identity stays type0. These do not assert new list types. */
   static const uint32_t hs[]={32,36,48,64},es[]={0,4,16,32};
   header=hs[(current/11)%4];extra=es[(current/13)%4];
   wr(at(ram,ALIGNMENT),a);wr(at(ram,COUNT),count);wr(at(ram,TABLE+0x24),header);wr(at(ram,TABLE+0x28),extra);
  }
  nflstreet_scene_owner_context x={{ram,BYTES,BASE},TABLE,a,count,&services};
  ns->responses[0]=os->responses[0]=current&&current%5==1?0:SCENE;
  ns->responses[1]=os->responses[1]=current&&current%5==2?0:LIST;
  ns->scene_size=os->scene_size=40+args[5]*count*4u;
  ns->list_size=os->list_size=fixture_layout(args[1],4,a,header,extra);ns->list_address=os->list_address=LIST;
  memcpy(oracle,ram,BYTES);uint32_t got,want;
  HOST(nflstreet_scene_owner_create(&x,args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],&got));
  ORIG(current?0x801954e4u:0x80004830u,args,&want);
  /* 80004830 is void: compare its writes/services, not its incidental r3. */
  if(current)RETURN(got,want,"scene");SAME("scene");if(!traces(ns,os))return 8;
  if(!got)++scene_fail;else{
   ++scenes;if(!rd(at(ram,got)))++list_fail;if(!args[3])++default_cb;else ++custom_cb;
   if(args[5]*count)++nonzero_region;else ++zero_region;if(args[1]>=256)++wide;}
  /* Direct list path also covers a supplied block and varied element sizes.
     New fixture phase uses unchanged RAM plus explicit controlled responses. */
  memset(ns,0,sizeof *ns);memset(os,0,sizeof *os);ns->ram=ram;os->ram=oracle;
  uint32_t element=current?rnd()%93u:4u,provided=current%2?DIRECT:0;
  ns->responses[0]=os->responses[0]=DIRECT;
  ns->list_size=os->list_size=fixture_layout(args[1],element,a,header,extra);ns->list_address=os->list_address=DIRECT;
  wr(at(ram,OUTPUT),element);wr(at(oracle,OUTPUT),element);
  uint32_t stride,total;HOST(nflstreet_scene_list_layout(&x,0,args[1],element,&stride,&total));wr(at(ram,OUTPUT),stride);uint32_t layout_args[8]={0,args[1],OUTPUT,0,0,0,0,0};
  ORIG(0x80180388u,layout_args,&want);RETURN(total,want,"layout");SAME("layout");
  HOST(nflstreet_scene_list_create(&x,args[0],0,args[1],element,args[3],provided,&got));
  uint32_t list_args[8]={args[0],0,args[1],element,args[3],provided,0,0};
  ORIG(0x801805e0u,list_args,&want);RETURN(got,want,"list");SAME("list");if(!traces(ns,os))return 8;if(provided)++supplied;
  digest=hash(digest,ram,LO-BASE);digest=hash(digest,at(ram,HI),BYTES-(HI-BASE));
 }
 nflstreet_scene_owner_context guards={{ram,BYTES,BASE},TABLE,4,30,&(nflstreet_pool_services){ns,heap,error}};
 int rejected=invalid(guards);
 if(rejected!=8||!scenes||!scene_fail||!list_fail||!default_cb||!custom_cb||!zero_region||!nonzero_region||!wide||!supplied)return 9;
 printf("Original scene owner: %u cases (1 saved wrapper + %u edited); scenes/scene-failure/list-failure %u/%u/%u; default/custom %u/%u; zero/nonzero region %u/%u; wide pools %u; supplied lists %u; 8 host guards; returns, ordered services and all RAM outside [80f46f58,80f47008) match; digest %08x.\n",
 edited+1,edited,scenes,scene_fail,list_fail,default_cb,custom_cb,zero_region,nonzero_region,wide,supplied,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
