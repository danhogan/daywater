/* Integration test: original storage/stack routines execute in untouched DLL.
   Only heap/error/geterror services return early with controlled responses. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "moderngekko/module_abi.h"
#include "matrix_storage.h"
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define LO (STACK-24u)
#define HI (STACK+8u)
#define FLAG (0x803a9a40u-0x65f4u)
#define ROOT (0x803a9a40u-0x65f0u)
#define COUNT (0x803a9a40u-0x65ecu)
#define CURRENT (0x803a9a40u-0x65e8u)
#define INDEX (0x803a9a40u-0x4ee8u)
#define ZERO 0x80261800u
#define ONE 0x80261804u
#define TABLE 0x81000004u
#define A 0x81100004u
#define B 0x81200004u
#define C 0x81300004u
#define D 0x81400004u
#define TABLE_BYTES 8192u
#define SLOT_BYTES 4096u
static const uint32_t buffers[4]={A,B,C,D};
typedef struct {
 uint32_t pc,a,b,c,result,root,count,current;
 uint8_t flag[8],table[TABLE_BYTES],slots[4][SLOT_BYTES];
} event;
typedef struct {
 uint8_t *ram;event events[32];unsigned count,heaps,bad;
 uint32_t response[16],get_response;
} service;
static uint8_t *ram,*oracle;
static const ModernGekkoModuleDesc *module;
static uint32_t seed=0xd2738a1u,digest=2166136261u;
static unsigned current;
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *at(uint8_t *r,uint32_t a){return r+(a-BASE);}
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static void note(service *s,uint32_t pc,uint32_t a,uint32_t b,uint32_t c,uint32_t result){
 if(s->count>=32){s->bad=1;return;}event *e=&s->events[s->count++];memset(e,0,sizeof *e);
 e->pc=pc;e->a=a;e->b=b;e->c=c;e->result=result;e->root=rd(at(s->ram,ROOT));e->count=rd(at(s->ram,COUNT));e->current=rd(at(s->ram,CURRENT));
 memcpy(e->flag,at(s->ram,FLAG-3),8);memcpy(e->table,at(s->ram,TABLE),TABLE_BYTES);
 for(unsigned i=0;i<4;++i)memcpy(e->slots[i],at(s->ram,buffers[i]),SLOT_BYTES);}
static uint32_t heap(void *u,uint32_t owner,uint32_t size,uint32_t flags){service *s=u;
 if(s->heaps>=16){s->bad=1;return 0;}uint32_t response=s->response[s->heaps++];note(s,0x8018b7e4u,owner,size,flags,response);return response;}
static uint32_t get_error(void *u){service *s=u;note(s,0x801af630u,0,0,0,s->get_response);return s->get_response;}
static void error(void *u,uint32_t code){note(u,0x801af57cu,code,0,0,0);}
static void response(service *n,service *o,uint32_t token){n->response[n->heaps]=o->response[o->heaps]=token;}
static uint8_t *draw_span(void *r,uint32_t a,uint32_t n){if(a<BASE||(uint64_t)(a-BASE)+n>BYTES)return NULL;return at(r,a);}
static int original(uint32_t pc,uint32_t a,uint32_t b,service *s,uint32_t *result){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=pc;c.gpr[3]=a;c.gpr[4]=b;
 for(unsigned step=0;c.pc;++step){
  if(step>=1000000){fprintf(stderr,"Oracle timeout %08x\n",c.pc);return 0;}
  if(c.pc==0x8018b7e4u){c.gpr[3]=heap(s,c.gpr[3],c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  if(c.pc==0x801af630u){c.gpr[3]=get_error(s);c.pc=c.lr;continue;}
  if(c.pc==0x801af57cu){error(s,c.gpr[3]);c.gpr[3]=0xdeadf00du;c.pc=c.lr;continue;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"Oracle exception %u at %08x\n",c.exception,c.pc);return 0;}}
 if(result)*result=c.gpr[3];return 1;}
static int same(const char *phase){
 if(!memcmp(ram,oracle,LO-BASE)&&!memcmp(at(ram,HI),at(oracle,HI),BYTES-(HI-BASE)))return 1;
 for(uint32_t i=0;i<BYTES;++i)if((i<LO-BASE||i>=HI-BASE)&&ram[i]!=oracle[i]){
  fprintf(stderr,"MISMATCH RAM case %u phase %s at %08x: original %02x native %02x\n",current,phase,BASE+i,oracle[i],ram[i]);return 0;}return 0;}
static int returns(uint32_t got,uint32_t want,const char *phase){
 if(got==want)return 1;fprintf(stderr,"MISMATCH RETURN case %u phase %s original %08x native %08x\n",current,phase,want,got);return 0;}
static int traces(service *n,service *o){
 if(n->bad||o->bad||n->count!=o->count||memcmp(n->events,o->events,n->count*sizeof(event))){
  fprintf(stderr,"MISMATCH TRACE case %u native %u original %u\n",current,n->count,o->count);return 0;}
 digest=hash(digest,n->events,n->count*sizeof(event));return 1;}
static uint8_t *load(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *r=malloc(BYTES);
 if(!r||fread(r,1,BYTES,f)!=BYTES||fgetc(f)!=EOF){free(r);r=NULL;}fclose(f);return r;}
#define HOST(x) do{if(!(x)){fprintf(stderr,"Native rejected case %u line %d\n",current,__LINE__);return 5;}}while(0)
#define ORIG(pc,a,b,out) do{if(!original(pc,a,b,os,out))return 4;}while(0)
#define RETURN(g,w,p) do{if(!returns(g,w,p))return 7;}while(0)
#define SAME(p) do{if(!same(p))return 7;}while(0)
#define TRACE() do{if(!traces(ns,os))return 8;}while(0)
static int invalid(nflstreet_matrix_storage_context x){
 at(ram,FLAG)[0]=0;wr(at(ram,ROOT),TABLE);wr(at(ram,COUNT),12);
 uint8_t *before=malloc(BYTES);if(!before)return 0;memcpy(before,ram,BYTES);uint32_t out=123;unsigned count=0;
#define REJECT(expr) do{out=123;if((expr)||out){fprintf(stderr,"Host guard failed line %d\n",__LINE__);free(before);return 0;}++count;}while(0)
 REJECT(nflstreet_matrix_storage_open(&x,0xffffffffu,&out));
 REJECT(nflstreet_matrix_storage_allocate(&x,12,4,&out));
 REJECT(nflstreet_matrix_storage_allocate(&x,0,0x4000000u,&out));
 nflstreet_matrix_storage_context bad=x;bad.flag_slot=BASE+BYTES;
 REJECT(nflstreet_matrix_storage_open(&bad,4,&out));bad=x;bad.table_slot=ROOT+1;
 REJECT(nflstreet_matrix_storage_open(&bad,4,&out));bad=x;bad.count_slot=BASE+BYTES-2;
 REJECT(nflstreet_matrix_storage_entry(&bad,0,&out));bad=x;bad.services=NULL;
 REJECT(nflstreet_matrix_storage_open(&bad,4,&out));
 if(memcmp(before,ram,BYTES)){free(before);return 0;}free(before);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<40||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);
 if(!saved||!ram||!oracle||!ns||!os||!at(saved,FLAG)[0]||rd(at(saved,ZERO))||rd(at(saved,ONE))!=0x3f800000u)return 3;
 unsigned opened=0,blocked=0,open_failed=0,allocated=0,alloc_failed=0,depth_zero=0,sequences=0,replaced=0,pops=0,underflows=0;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);memset(ns,0,sizeof *ns);memset(os,0,sizeof *os);ns->ram=ram;os->ram=oracle;
  nflstreet_matrix_storage_services services={ns,heap,get_error,error};
  nflstreet_matrix_storage_context x={{ram,BYTES,BASE},FLAG,ROOT,COUNT,CURRENT,ZERO,ONE,&services};uint32_t got,want;
  if(!current){memcpy(oracle,ram,BYTES);HOST(nflstreet_matrix_storage_open(&x,4,&got));ORIG(0x8018908cu,4,0,&want);RETURN(got,want,"saved initialized");SAME("saved initialized");++blocked;TRACE();}
  else{
   for(unsigned j=0;j<TABLE_BYTES+16;++j)at(ram,TABLE-4)[j]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned k=0;k<4;++k)for(unsigned j=0;j<SLOT_BYTES+16;++j)at(ram,buffers[k]-4)[j]=(uint8_t)((rnd()>>17)|1u);
   unsigned composed=current%16==0;
   at(ram,FLAG)[0]=current%5==0&&!composed?(uint8_t)(current%255+1):0;
   wr(at(ram,ROOT),0x80abcde0u);wr(at(ram,COUNT),0x31415926u);wr(at(ram,CURRENT),0xdeadbeecu);
   static const uint32_t sizes[]={0,1,4,8,14,30,255,256,511};
   uint32_t requested=composed?4:sizes[(current-1)%9];if(at(ram,FLAG)[0])requested=0xffffffffu;
   static const uint32_t errors[]={0,0x80000000u,0xffffffffu,0x1234u};ns->get_response=os->get_response=errors[(current/7)%4];
   response(ns,os,current%5==1&&!composed?0:TABLE);memcpy(oracle,ram,BYTES);
   HOST(nflstreet_matrix_storage_open(&x,requested,&got));ORIG(0x8018908cu,requested,0,&want);RETURN(got,want,"open");SAME("open");TRACE();
   if(requested==0xffffffffu){++blocked;continue;}
   if(!at(ram,FLAG)[0]){++open_failed;response(ns,os,TABLE);
    HOST(nflstreet_matrix_storage_open(&x,requested,&got));ORIG(0x8018908cu,requested,0,&want);RETURN(got,want,"retry open");SAME("retry open");TRACE();}
   ++opened;
   uint32_t total=rd(at(ram,COUNT)),indices[4]={0,total-1,total,0xffffffffu};
   for(unsigned j=0;j<4;++j){HOST(nflstreet_matrix_storage_entry(&x,indices[j],&got));ORIG(0x80189068u,indices[j],0,&want);RETURN(got,want,"entry lookup");SAME("entry lookup");}
   uint32_t first=composed?3:4+current%((total<255?total:255)-4);
   static const uint32_t depths[]={0,1,2,3,4,12,16,31,64};uint32_t depths_case[3]={composed?12:depths[(current-1)%9],12,12},stack_ids[3]={first,1,2};
   for(unsigned j=0;j<3;++j){response(ns,os,j==0&&current%7==0&&!composed?0:buffers[j]);
    HOST(nflstreet_matrix_storage_allocate(&x,stack_ids[j],depths_case[j],&got));ORIG(0x801891e8u,stack_ids[j],depths_case[j],&want);RETURN(got,want,"allocate");SAME("allocate");TRACE();
    if(!ns->response[ns->heaps-1]){++alloc_failed;response(ns,os,buffers[j]);
     HOST(nflstreet_matrix_storage_allocate(&x,stack_ids[j],depths_case[j],&got));ORIG(0x801891e8u,stack_ids[j],depths_case[j],&want);RETURN(got,want,"retry allocate");SAME("retry allocate");TRACE();}
    ++allocated;if(!depths_case[j])++depth_zero;}
   nflstreet_player_draw_context dx;memset(&dx,0,sizeof dx);dx.span=draw_span;dx.memory=ram;
   nflstreet_render_state state;memset(&state,0,sizeof state);nflstreet_draw draw={&dx,&state};
   nflstreet_matrix_stacks view={0xdeadbeefu,0,0,INDEX,0xcafebabeu,{2,3}};
   HOST(nflstreet_matrix_storage_bind(&x,&view));if(view.index!=INDEX||view.sine!=0xcafebabeu||view.angle_scale[0]!=2||view.angle_scale[1]!=3)return 9;
   at(ram,INDEX)[0]=at(oracle,INDEX)[0]=1;HOST(nflstreet_matrix_select(&draw,&view));ORIG(0x801893c8u,1,0,NULL);SAME("consumer select");
   HOST(nflstreet_matrix_top(&draw,&view,&got));ORIG(0x8018939cu,0,0,&want);RETURN(got,want,"consumer top");SAME("consumer top");
   HOST(nflstreet_matrix_reset(&draw,&view));ORIG(0x801893ecu,0,0,NULL);SAME("consumer reset");
   for(unsigned j=0;j<16;++j){uint32_t bits=rnd();wr(at(ram,B+4*j),bits);wr(at(oracle,B+4*j),bits);}
   HOST(nflstreet_matrix_push(&draw,&view));ORIG(0x8018941cu,0,0,NULL);SAME("consumer push copy");
   HOST(nflstreet_matrix_push_identity(&draw,&view));ORIG(0x80189460u,0,0,NULL);SAME("consumer push identity");
   uint8_t dirty[64];memcpy(dirty,at(ram,B+128),64);
   HOST(nflstreet_matrix_pop(&draw,&view));ORIG(0x8018949cu,0,0,NULL);SAME("consumer pop dirty slot");if(memcmp(dirty,at(ram,B+128),64))return 9;++pops;
   HOST(nflstreet_matrix_reset(&draw,&view));ORIG(0x801893ecu,0,0,NULL);SAME("consumer reset depth");
   HOST(nflstreet_matrix_pop(&draw,&view));ORIG(0x8018949cu,0,0,NULL);SAME("consumer pop underflow");++underflows;
   HOST(nflstreet_matrix_reset(&draw,&view));ORIG(0x801893ecu,0,0,NULL);SAME("consumer restore");
   response(ns,os,D);HOST(nflstreet_matrix_storage_allocate(&x,first,2,&got));ORIG(0x801891e8u,first,2,&want);RETURN(got,want,"replace without free");SAME("replace without free");TRACE();++replaced;
   at(ram,INDEX)[0]=at(oracle,INDEX)[0]=(uint8_t)(total<256?total:255);HOST(nflstreet_matrix_select(&draw,&view));ORIG(0x801893c8u,at(ram,INDEX)[0],0,NULL);SAME("consumer selection boundary");
   if(composed)++sequences;
  }
  digest=hash(digest,ram,LO-BASE);digest=hash(digest,at(ram,HI),BYTES-(HI-BASE));
 }
 int guards=invalid((nflstreet_matrix_storage_context){{ram,BYTES,BASE},FLAG,ROOT,COUNT,CURRENT,ZERO,ONE,
 &(nflstreet_matrix_storage_services){ns,heap,get_error,error}});
 if(guards!=7||!opened||!blocked||!open_failed||!allocated||!alloc_failed||!depth_zero||!sequences||!replaced||!pops||!underflows)return 9;
 printf("Original matrix storage: %u cases (1 saved + %u edited); table open/blocked/fail %u/%u/%u; stack allocations/fail/zero-depth %u/%u/%u; startup-sequences/replacements/pop-dirty/underflow %u/%u/%u/%u; 7 host guards; returns, ordered services and all RAM outside [80f46fe8,80f47008) match; digest %08x.\n",edited+1,edited,opened,blocked,open_failed,allocated,alloc_failed,depth_zero,sequences,replaced,pops,underflows,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
