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
#include "view_frustum.h"
#include "camera_projection.h"
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
static unsigned current;static float input_floats[6],original_float;static unsigned input_count;
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
 for(unsigned i=0;i<input_count;++i)c.fpr[i+1]=(double)input_floats[i];
 for(unsigned step=0;c.pc;++step){
  if(step>=1000000){fprintf(stderr,"Oracle timeout %08x\n",c.pc);return 0;}
  if(c.pc==0x8018b7e4u){c.gpr[3]=heap(s,c.gpr[3],c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  if(c.pc==0x801af57cu){error(s,c.gpr[3]);c.gpr[3]=0xdeadf00du;c.pc=c.lr;continue;}
  if(c.pc==0x801af630u){c.gpr[3]=get_error(s);c.pc=c.lr;continue;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"Oracle exception %u at %08x\n",c.exception,c.pc);return 0;}}
 original_float=(float)c.fpr[1];if(result)*result=c.gpr[3];return 1;}
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
#define SLOTS 0x81300000u
static uint32_t fb(float f){uint32_t b;memcpy(&b,&f,4);return b;}
static float fl(const uint8_t *p){uint32_t u=rd(p);float f;memcpy(&f,&u,4);return f;}
static void reset(service *n,service *o){memset(n,0,sizeof *n);memset(o,0,sizeof *o);n->ram=ram;o->ram=oracle;
 n->list_address=o->list_address=LIST;n->list_size=o->list_size=LIST_BYTES;n->scene_size=o->scene_size=SCENE_BYTES;n->responses[0]=o->responses[0]=LIST;input_count=0;}
static uint8_t *draw_span(void *u,uint32_t token,uint32_t n){uint8_t *r=u;if(token<BASE||(uint64_t)token+n>BASE+BYTES)return NULL;return at(r,token);}
static int guards(nflstreet_view_frustum_context x){
 uint8_t *before=malloc(BYTES);if(!before)return 0;memcpy(before,ram,BYTES);unsigned count=0;float result;
#define REJECT(e) do{if(e){free(before);return 0;}++count;}while(0)
 nflstreet_view_frustum_context b=x;b.ram.bytes=NULL;REJECT(nflstreet_view_tangent(&b,0,&result));
 b=x;b.angle_constants=BASE+BYTES-4;REJECT(nflstreet_view_tangent(&b,0,&result));
 REJECT(nflstreet_view_tangent(&x,0x80000000u,&result));
 REJECT(nflstreet_view_frustum_set(&x,LIST+1,45,1,1,10));
 REJECT(nflstreet_view_frustum_set(&x,BASE+BYTES-4,45,1,1,10));
 REJECT(nflstreet_view_frustum_set(&x,LIST,360,1,1,10));
 REJECT(nflstreet_view_scale_set(&x,BASE+BYTES-4,1));
 REJECT(nflstreet_view_binding_set(x.ram,LIST+1,0,0));
 if(memcmp(before,ram,BYTES)){free(before);return 0;}free(before);return count;}
int main(int argc,char **argv){
 if(argc!=4)return 2;unsigned edited=(unsigned)strtoul(argv[3],NULL,10);if(edited<40||edited>10000)return 2;
 HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->abi_version!=MODERNGEKKO_MODULE_ABI_VERSION||module->cpu_abi_version!=MODERNGEKKO_CPU_ABI_VERSION||module->cpu_state_size!=sizeof(CPUState)||strcmp(module->game_id,"GNNE69"))return 3;
 uint8_t *saved=load(argv[1]);ram=malloc(BYTES);oracle=malloc(BYTES);service *ns=calloc(1,sizeof *ns),*os=calloc(1,sizeof *os);if(!saved||!ram||!oracle||!ns||!os)return 3;
 nflstreet_pool_ram memory={ram,BYTES,BASE};nflstreet_pool_services ps={ns,heap,error};
 nflstreet_view_nodes_context vx={{memory,TABLE,&ps,NULL,NULL},VIEWS,CALLBACKS,0x8039e07c,0x80263cc8,get_error};
 nflstreet_view_frustum_context fx={memory,0x803a9a40,0x802617b8,0x80263ce8};
 nflstreet_player_draw_context draw={0};draw.span=draw_span;draw.memory=ram;nflstreet_view_context projection={0};projection.x=&draw;projection.r13=0x803a9a40;projection.slots=SLOTS;
 unsigned fresh=0,setters=0,scales=0,bindings=0,tangents=0,reduced=0,negative=0,projected=0,defaults=0,nans=0;
 for(current=0;current<=edited;++current){
  memcpy(ram,saved,BYTES);reset(ns,os);uint32_t got,want,node=0x806a1ca8u;
  if(current){
   for(unsigned j=0;j<LIST_BYTES+8;++j)at(ram,LIST-4)[j]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned j=0;j<0x200;++j)at(ram,SLOTS)[j]=(uint8_t)((rnd()>>17)|1u);
   for(unsigned j=0;j<32;++j)at(ram,OUTPUT)[j]=(uint8_t)((rnd()>>17)|1u);
   wr(at(ram,ALIGNMENT),4u<<((current/8)%4));nflstreet_scene_owner_context owner={memory,TABLE,rd(at(ram,ALIGNMENT)),30,&ps};
   uint32_t args[8]={1,0,1,0xf8,0x801def40,0,0,0};memcpy(oracle,ram,BYTES);stack_lo=STACK-112u;
   HOST(nflstreet_scene_list_create(&owner,1,0,1,0xf8,0x801def40,0,&got));ORIG(0x801805e0u,args,&want);RETURN(got,want,"fresh list");SAME("fresh list");if(!traces(ns,os))return 8;
   wr(at(ram,VIEWS),got);wr(at(oracle,VIEWS),got);wr(at(ram,OUTPUT),0x00040002);wr(at(ram,OUTPUT+4),0);wr(at(ram,OUTPUT+8),0);wr(at(ram,OUTPUT+16),0x028001e0);memcpy(at(oracle,OUTPUT),at(ram,OUTPUT),20);
   reset(ns,os);args[0]=OUTPUT;HOST(nflstreet_view_node_create(&vx,OUTPUT,&got));ORIG(0x801df14cu,args,&want);RETURN(got,want,"fresh view");SAME("fresh view");if(!traces(ns,os))return 8;node=got;++fresh;
  }else memcpy(oracle,ram,BYTES);
  for(unsigned k=0;k<(current?4u:1u);++k){
   reset(ns,os);static const uint32_t focused[]={0,1,0xffffffffu,0x100000,0x1fffff,0x200000,0x200001,0x400000,0x400001,0x3fffff,0x5fffff,0xffa00001u};
   uint32_t angle;if(!current)angle=0x100000;else if(k==0)angle=focused[(current-1)%12];else {int32_t raw=(int32_t)(rnd()%12582909u)-6291454;memcpy(&angle,&raw,4);}
   float value;HOST(nflstreet_view_tangent(&fx,angle,&value));uint32_t args[8]={angle,0,0,0,0,0,0,0};stack_lo=STACK-104u;ORIG(0x80188c80u,args,NULL);RETURN(fb(value),fb(original_float),"tangent float return");SAME("tangent");if(!traces(ns,os))return 8;
   ++tangents;int32_t signed_angle;memcpy(&signed_angle,&angle,4);if(signed_angle<0)++negative;if(signed_angle>0x1fffff||signed_angle<-(int32_t)0x1fffff)++reduced;digest=hash(digest,&value,4);
  }
  reset(ns,os);static const float fovs[]={45,0,1,15,60,77.2f,89.999f,90,100,179.99f,180,200,260,-269,-90,-45};
  float fov=!current||current%16==0?45:fovs[(current-1)%16],aspect=!current?1.333333373f:(current%2?1.333333373f:0.75f),near_z=!current?0.1f:current%13==0?0:(float)(1+current%17)/100.0f,far_z=!current?750.0f:100.0f+(float)current;
  if(current){wr(at(ram,node+0x1c),rnd());wr(at(oracle,node+0x1c),rd(at(ram,node+0x1c)));}
  input_floats[0]=fov;input_floats[1]=aspect;input_floats[2]=near_z;input_floats[3]=far_z;input_count=4;uint32_t args[8]={node,0,0,0,0,0,0,0};stack_lo=STACK-144u;
  HOST(nflstreet_view_frustum_set(&fx,node,fov,aspect,near_z,far_z));ORIG(0x801df60cu,args,NULL);SAME("frustum setter");if(!traces(ns,os))return 8;++setters;if(fov==45)++defaults;
  reset(ns,os);float scale=!current?4:fov==0&&current%3==2?0:(float)((int)(current%9)-3)/2.0f;input_floats[0]=scale;input_count=1;stack_lo=STACK-88u;
  HOST(nflstreet_view_scale_set(&fx,node,scale));ORIG(0x801df6b4u,args,NULL);SAME("scale setter");if(!traces(ns,os))return 8;++scales;if((rd(at(ram,node+0xf0))&0x7fffffffu)>0x7f800000u)++nans;
  reset(ns,os);uint32_t binding=rnd();uint8_t flag=(uint8_t)rnd();uint32_t bindargs[8]={node,binding,flag,0,0,0,0,0};stack_lo=HI;
  HOST(nflstreet_view_binding_set(memory,node,binding,flag));ORIG(0x801df5f8u,bindargs,NULL);SAME("binding setter");if(!traces(ns,os))return 8;++bindings;
  if(fov!=0&&near_z!=0){reset(ns,os);float viewport=4.0f;HOST(nflstreet_camera_projection(&projection,node,viewport));
   input_floats[0]=fl(at(ram,node+4));input_floats[1]=fl(at(ram,node+8));input_floats[2]=fl(at(ram,node+0x14));input_floats[3]=fl(at(ram,node+0x18));input_floats[4]=viewport;input_count=5;uint32_t outargs[8]={SLOTS+0x40,0,0,0,0,0,0,0};stack_lo=HI;
   ORIG(0x801df8c4u,outargs,NULL);/* Native writes both slots; compare after both original leaves. */
   input_floats[0]=fl(at(ram,node+0xd8));input_floats[1]=fl(at(ram,node+0xdc));input_floats[2]=fl(at(ram,node+0xe4));input_floats[3]=fl(at(ram,node+0xe0));input_floats[4]=fl(at(ram,node+0xe8));input_floats[5]=fl(at(ram,node+0xec));input_count=6;outargs[0]=SLOTS+0x80;stack_lo=STACK-8u;ORIG(0x801df938u,outargs,NULL);SAME("fresh projection");if(!traces(ns,os))return 8;++projected;}
  digest=hash(digest,at(ram,node),0xf8);digest=hash(digest,at(ram,SLOTS),0x200);
 }
 if(guards(fx)!=8||!fresh||!setters||!scales||!bindings||!tangents||!reduced||!negative||!projected||!defaults||!nans)return 9;
 printf("Original view frustum: %u cases (1 saved + %u edited); fresh/set/scale/bind %u/%u/%u/%u; tangent/reduced/negative/project/default/nan %u/%u/%u/%u/%u/%u; 8 host guards; float returns, ordered services and all RAM outside phase stacks match; digest %08x.\n",edited+1,edited,fresh,setters,scales,bindings,tangents,reduced,negative,projected,defaults,nans,digest);
 free(saved);free(ram);free(oracle);free(ns);free(os);FreeLibrary(dll);return 0;}
