/* Native player draw (80161E10) for the captured actors of a saved context
   and deterministic controlled variants of their state. The saved RAM is
   fixture input only: actors, models, materials, shaders and light sets are
   read in place as original-layout objects, tokens are their addresses.
   Controlled variants edit a fresh copy per case: LOD-0 heads with and
   without the compiled head model, eye angles, light-color interpolation and
   fades, light sets, material arrays, synthetic material instances and
   parameter shaders, stage textures and palettes, chunk flags, formats and
   material indices, vertex array overrides and group selectors. */
#include "player_draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define ACTOR_LIST (R13-0x7bbcu)
#define PLAYERS 14u
#define CONTROLLED 840u
#define CALLS_MAX 16384u
/* Zero in all three contexts: synthetic objects, then the oracle stack. */
#define SCRATCH 0x80f04000u
#define STACK_LO 0x80f38000u
#define STACK_HI 0x80f47000u
#define PARAM_SHADER SCRATCH
#define PARAM_RECORDS (SCRATCH+0x100u)
#define INSTANCES (SCRATCH+0x1000u)
#define ENTRIES (SCRATCH+0x2000u)
#define FLAGGED_PALETTE (SCRATCH+0x3000u)
#define LIGHT_TARGET (SCRATCH+0x3100u)
#define OVERRIDES (SCRATCH+0x4000u)
#define PLAYER_RECORDS 0x802aaa60u
#define LIGHT_SETS 0x8034fe9cu
#define SHARED_MODEL 0x803102b4u
#define VIEWPORT 0x8039e584u
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t rd16(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static void wr32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void wr16(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static uint8_t *ram_at(uint8_t *ram,uint32_t a,uint32_t n){return a>=RAM_BASE&&(uint64_t)a-RAM_BASE+n<=RAM_BYTES?ram+(a-RAM_BASE):NULL;}
static uint32_t ram32(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,4);return p?rd32(p):0;}
static uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint8_t *load_ram(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *b=malloc(RAM_BYTES);
 if(!b||fread(b,1,RAM_BYTES,f)!=RAM_BYTES){free(b);fclose(f);return NULL;}fclose(f);return b;}
/* Native memory namespace: the case's RAM copy, never the oracle stack. */
static uint8_t *span(void *m,uint32_t token,uint32_t size){
 if((uint64_t)token+size>STACK_LO&&token<STACK_HI)return NULL;return ram_at(m,token,size);}
typedef struct {nflstreet_gx_call *calls;unsigned count;} trace;
static int record(void *t,const nflstreet_gx_call *c){trace *r=t;if(r->count>=CALLS_MAX)return 0;r->calls[r->count++]=*c;return 1;}
typedef struct {int compiled;unsigned requests;} face_plan;
static int face(void *p,uint32_t actor,int *compiled){(void)actor;face_plan *f=p;++f->requests;*compiled=f->compiled;return 1;}
enum {F_LOD0=1,F_FACE=2,F_EYES=4,F_LIGHT=8,F_FADE=16,F_LIGHTSET=32,F_MATERIALS=64,F_TEXTURES=128,F_CHUNKS=256,F_OVERRIDES=512,
 F_REFRESH=1024,F_RECORD=2048,F_DIM=4096};
typedef struct {uint32_t actor,features;int compiled;} draw_case;
static uint32_t seed=0x2468aceu;
static uint32_t rnd(void){seed=seed*1103515245u+12345u;return seed>>8;}
static float rndf(float lo,float hi){return lo+(hi-lo)*(float)(rnd()&0xffffu)/65535.0f;}
static uint32_t fbits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
/* A random word that is a normal (or zero) single: float fields pass through
   lfs/stfs, which this comparison does not exercise for NaN or denormals. */
static uint32_t normal_word(void){uint32_t v=rnd()<<8^rnd();uint32_t e=v>>23&0xffu;return e==0xffu||!e?v&0x807fffffu:v;}
static uint32_t live_actor(uint8_t *ram,unsigned i){return ram32(ram,ram32(ram,ACTOR_LIST)+4u*i);}
static const uint32_t instances_at[12]={0x4ac,0x59c,0x68c,0x77c,0x86c,0x95c,0xa4c,0xb3c,0xc2c,0xd1c,0xe0c,0xefc};
/* Sum of chunk counts of the selected group variants (801EB12C bound). */
static unsigned selected_chunks(uint8_t *ram,uint32_t groups,const uint8_t *sel,unsigned count){
 unsigned n=0;for(unsigned g=0;g<count;++g){if(sel[g]==0xff)continue;
  const uint8_t *gr=ram_at(ram,ram32(ram,groups+4)+0x14u*g,0x14);if(!gr||sel[g]>=gr[0x12])continue;
  const uint8_t *v=ram_at(ram,ram32(ram,groups+8)+0x14u*(rd16(gr+0x10)+sel[g]),0x14);if(v)n+=rd16(v+0x12);}
 return n;}
static void edit_chunks(uint8_t *ram,uint32_t model){
 uint32_t table=ram32(ram,model+8);if(!table)return;uint32_t chunks=ram32(ram,table),count=ram32(ram,table+4),arrays=ram32(ram,table+8);
 for(unsigned k=0;k<count;++k){uint8_t *c=ram_at(ram,chunks+0x18u*k,0x18);if(!c||rnd()%3)continue;
  if(rnd()%2)c[0x11]^=(uint8_t)(rnd()%2?1:8);if(rnd()%2)c[0x12]=(uint8_t)(rnd()%4);
  if(rnd()%3==0)wr16(c+6,(uint32_t)(int32_t)((int)(rnd()%52)-3));
  if(c[0x10]&&rnd()%2){uint8_t *r=ram_at(ram,arrays+0xcu*(rd16(c+0xe)+rnd()%c[0x10]),0xc);if(r){r[6]=9;r[9]=(uint8_t)(rnd()%2?3:rnd()%5);}}}}
/* Apply the case's features to the fresh RAM copy. */
static void prepare(uint8_t *ram,unsigned index,draw_case *c){
 memset(c,0,sizeof *c);
 if(index<PLAYERS){c->actor=live_actor(ram,index);return;}
 c->actor=live_actor(ram,rnd()%PLAYERS);c->features=rnd()&0x1fffu;if(index%7==0)c->features|=F_LOD0|F_EYES;
 uint32_t a=c->actor;uint8_t *actor=ram_at(ram,a,0x1a90),*st=ram_at(ram,rd32(actor+0x4a8),0x34);
 uint32_t one=ram32(ram,R13-0x5638u),two=ram32(ram,R13-0x5634u),none=ram32(ram,R13-0x5684u);
 uint32_t list=rd32(actor+0xff0),first=ram32(ram,list),texture=0,pal=0;
 /* A live texture object and palette object of this actor's instances. */
 for(unsigned i=0;i<45&&!pal;++i){uint32_t st0=ram32(ram,ram32(ram,list+4*i)+0x10);texture=ram32(ram,st0);pal=ram32(ram,st0+4);}
 if(c->features&F_LOD0){wr32(st+8,0);wr32(st+0x10,a+0x420u+0x44u*(rnd()%2));
  uint32_t f=rd32(st+4)&~8u;if(c->features&F_FACE){f|=8;c->compiled=(int)(rnd()%2);}wr32(st+4,f);}
 if(c->features&F_EYES){wr32(actor+0x14,rd32(actor+0x14)|0x80u);uint8_t *e=ram_at(ram,rd32(actor+0x5c)+0xa8,10);
  for(unsigned i=0;i<5&&e;++i)wr16(e+2*i,rnd()%4?rnd()%0x1200u-0x900u:rnd());}
 if(c->features&F_DIM){wr32(actor+0x14,rd32(actor+0x14)^8u);wr32(actor+0x1084,fbits(rndf(-60.0f,60.0f)));}
 if(c->features&F_LIGHT){uint8_t *t=ram_at(ram,LIGHT_TARGET,0x18);
  for(unsigned i=0;i<6;++i)wr32(actor+0x1a64+4*i,fbits(rndf(0.0f,2.0f)));
  for(unsigned i=0;i<3;++i)wr32(t+0xc+4*i,rnd()%3?fbits(rndf(0.0f,2.0f)):rd32(actor+0x1a64+4*i));
  wr32(actor+0x3ec,rnd()%4?LIGHT_TARGET:0);wr32(actor+0x1a84,rnd()%20);}
 /* Exact tints (1, 3, 5 times 255, 85, 51) reach the 255 clamp boundary. */
 static const float exact[4]={1.0f,3.0f,5.0f,2.0f};
 if(c->features&F_FADE){wr32(actor+0x1a88,rnd()%2);wr32(actor+0x1a8c,rnd()%40);actor[0x1a4c]=(uint8_t)(rnd()%3);
  for(unsigned i=0;i<3;++i)wr32(actor+0x1a50+4*i,fbits(rnd()%2?exact[rnd()%4]:rndf(0.0f,2.0f)));}
 if(c->features&F_LIGHTSET){uint8_t *s=ram_at(ram,LIGHT_SETS+0x108u,0x108);wr32(s,rnd()%5);
  static const uint8_t channel[4]={0xff,0x55,0x33,0x80};
  for(unsigned i=4;i<0x108;++i)s[i]=(uint8_t)rnd();
  for(unsigned k=0;k<4;++k)for(unsigned b=0;b<3;++b)if(rnd()%2)s[8+0x40*k+0xc+b]=channel[rnd()%4];}
 if(c->features&F_MATERIALS){uint8_t *arr=ram_at(ram,a+0xfec,8);if(rnd()%2)wr32(arr,rnd()%46);
  uint8_t *shader=ram_at(ram,PARAM_SHADER,0x20),*recs=ram_at(ram,PARAM_RECORDS,0x14*6);
  memcpy(shader,ram_at(ram,one,0x20),0x20);wr32(shader+0x10,1+rnd()%2);wr32(shader+0x14,rnd()%7);wr32(shader+0x18,PARAM_RECORDS);
  static const uint8_t types[6]={2,4,5,6,0,7};
  for(unsigned i=0;i<6;++i){uint8_t *r=recs+0x14*i;r[0x10]=types[rnd()%6];r[0x11]=(uint8_t)(rnd()%5);wr16(r+0x12,rnd()%8);}
  const uint32_t shaders[4]={none,one,two,PARAM_SHADER};
  for(unsigned i=0;i<16;++i){uint8_t *in=ram_at(ram,INSTANCES+0x40u*i,0x40),*en=ram_at(ram,ENTRIES+0x20u*i,0x18);
   wr32(in+4,ram32(ram,first+4));wr32(in+8,shaders[rnd()%4]);wr32(in+0xc,2);wr32(in+0x10,ENTRIES+0x20u*i);
   for(unsigned k=0;k<8;++k)wr32(in+0x14+4*k,rnd()%2?fbits(rndf(-4.0f,4.0f)):normal_word());
   for(unsigned k=0;k<2;++k){wr32(en+12*k,rnd()%4?texture:0);uint32_t p=rnd()%3;wr32(en+12*k+4,p==0?0:p==1?pal:FLAGGED_PALETTE);wr32(en+12*k+8,0xffffffffu);}}
  for(unsigned i=0;i<45;++i){uint8_t *e=ram_at(ram,list+4*i,4);uint32_t r=rnd()%8;if(r==0)wr32(e,0);else if(r<3)wr32(e,INSTANCES+0x40u*(rnd()%16));}}
 if(c->features&F_TEXTURES){memcpy(ram_at(ram,FLAGGED_PALETTE,0x28),ram_at(ram,pal,0x28),0x28);wr32(ram_at(ram,FLAGGED_PALETTE,4),1);
  for(unsigned i=0;i<45;++i){uint32_t in=ram32(ram,list+4*i);if(in<RAM_BASE||in>=SCRATCH)continue;
   uint8_t *en=ram_at(ram,ram32(ram,in+0x10),12);if(!en)continue;uint32_t r=rnd()%8;
   if(r==0)wr32(en,0);else if(r==1)wr32(en+4,0);else if(r==2)wr32(en+4,FLAGGED_PALETTE);}}
 else{memcpy(ram_at(ram,FLAGGED_PALETTE,0x28),ram_at(ram,pal,0x28),0x28);wr32(ram_at(ram,FLAGGED_PALETTE,4),1);}
 for(unsigned i=0;i<12;++i){uint8_t *in=ram_at(ram,a+instances_at[i],0xf0);uint32_t model=rd32(in+0xc);if(!model)continue;
  if((c->features&F_CHUNKS)&&rnd()%2)edit_chunks(ram,model);
  if((c->features&F_OVERRIDES)&&rnd()%2){uint8_t *v=ram_at(ram,model,0x34),*o=ram_at(ram,OVERRIDES,0x1000);
   for(unsigned k=0;k<0x400;++k)wr32(o+4*k,rnd()%2?0:RAM_BASE+(rnd()%RAM_BYTES&~31u));wr32(v+0x1c,OVERRIDES);}
  if((c->features&F_REFRESH)&&rnd()%2){uint32_t groups=ram32(ram,model+0x14);if(!groups)continue;
   unsigned count=ram32(ram,groups);if(count>64)count=64;uint8_t sel[64];memset(sel,0xff,sizeof sel);
   for(unsigned k=0;k<count;++k)if(rnd()%3==0)sel[k]=(uint8_t)(rnd()%3);
   while(selected_chunks(ram,groups,sel,count)>64){for(unsigned k=0;k<count;++k)if(sel[k]!=0xff){sel[k]=0xff;break;}}
   memcpy(in+0x1c,sel,count);wr32(in+(rnd()%2?0x14:0x18),1+rnd()%3);wr32(in+4,rnd());wr32(in+8,rnd());}}
 if(c->features&F_RECORD)ram_at(ram,PLAYER_RECORDS+0x68u*actor[0x1363]+0x65u,1)[0]=(uint8_t)(rnd()%2);
}
static nflstreet_player_draw_context draw_context(uint8_t *ram,face_plan *f,trace *t){
 nflstreet_player_draw_context x;memset(&x,0,sizeof x);
 x.span=span;x.memory=ram;x.emit=record;x.sink=t;x.face=face;x.face_context=f;
 x.shared_model=SHARED_MODEL;x.player_records=PLAYER_RECORDS;x.light_sets=LIGHT_SETS;
 x.default_texture=ram32(ram,R13-0x640cu);x.null_shader=ram32(ram,R13-0x5684u);
 x.setups[0]=0x801c9d0cu;x.setups[1]=0x801c9dacu;x.setups[2]=0x801c9dfcu;
 uint32_t d0=ram32(ram,R13-0x6850u),d1=ram32(ram,R13-0x684cu);memcpy(&x.dim[0],&d0,4);memcpy(&x.dim[1],&d1,4);
 return x;}
static void initial_state(uint8_t *ram,nflstreet_render_state *s){
 memset(s,0,sizeof *s);for(unsigned i=0;i<4;++i){uint32_t v=ram32(ram,VIEWPORT+4*i);memcpy(&s->viewport[i],&v,4);}}
typedef struct {unsigned cases,calls,lists,lights,scaled,two_pass,tluts,default_loads,params,cached,lod0,compiled,refreshed,headwear,
 restarts,interpolating,fades,fade_ends,tints,eye_clamps;} totals;
static float word_float(const uint8_t *p){uint32_t v=rd32(p);float f;memcpy(&f,&v,4);return f;}
/* Light-color and eye branches taken, from the state before the draw. */
static void branches(const uint8_t *before,uint32_t actor,totals *n){
 const uint8_t *a=before+(actor-RAM_BASE);uint32_t target=rd32(a+0x3ec);
 if(target){const uint8_t *t=before+(target+0xc-RAM_BASE);int restart=0;
  for(unsigned i=0;i<3;++i)if(!(word_float(t+4*i)==word_float(a+0x1a64+4*i)))restart=1;
  n->restarts+=restart;n->interpolating+=(restart?0u:rd32(a+0x1a84))<=14u;}
 if(rd32(a+0x1a88)){if(rd32(a+0x1a8c)>0x1du)++n->fade_ends;else ++n->fades;}else n->tints+=a[0x1a4c]==1;
 const uint8_t *st=before+(rd32(a+0x4a8)-RAM_BASE);
 if(!rd32(st+8)&&(rd32(a+0x14)&0x80u)){const uint8_t *e=before+(rd32(a+0x5c)+0xa8-RAM_BASE);
  static const unsigned used[4]={0,1,3,4};
  for(unsigned i=0;i<4;++i){float deg=(float)(int32_t)((uint32_t)(int32_t)(int16_t)rd16(e+2*used[i])<<8)*2.1457672119140625e-05f;
   n->eye_clamps+=deg>(i<2?45.619f:47.028f)||deg<(i<2?-45.619f:-47.028f);}}}
/* Counts from the trace; a chunk whose format id is current skips
   GXClearVtxDesc, so cached formats = display lists - clears. Eye texture
   matrices of LOD-0 heads are not shader parameters. */
static void count(const uint8_t *before,const uint8_t *after,const draw_case *c,const trace *t,uint32_t default_texture,totals *n){
 ++n->cases;n->calls+=t->count;unsigned clears=0,lists=0,scaled=0;
 const uint8_t *set=before+(LIGHT_SETS+0x108u-RAM_BASE);
 for(unsigned i=0;i<t->count;++i){const nflstreet_gx_call *g=t->calls+i;
  switch(g->op){
  case NFLSTREET_GX_CALL_DISPLAY_LIST:++lists;break;
  case NFLSTREET_GX_CLEAR_VTX_DESC:++clears;break;
  case NFLSTREET_GX_LOAD_LIGHT:{++n->lights;unsigned k=0;while(k<4&&g->arg[0]!=1u<<k)++k;
   if(k<4&&g->data[3]!=rd32(set+8+0x40*k+0xc))scaled=1;break;}
  case NFLSTREET_GX_SET_NUM_TEV_STAGES:n->two_pass+=g->arg[0]==3;break;
  case NFLSTREET_GX_LOAD_TLUT:++n->tluts;break;
  case NFLSTREET_GX_LOAD_TEX_OBJ:n->default_loads+=g->arg[0]==default_texture;break;
  case NFLSTREET_GX_SET_TEV_COLOR:case NFLSTREET_GX_SET_CHAN_AMB_COLOR:case NFLSTREET_GX_SET_CHAN_MAT_COLOR:case NFLSTREET_GX_LOAD_TEX_MTX:
   ++n->params;break;
  default:break;}}
 /* Fixed per draw: one ambient and one material color; two eye matrices per LOD-0 head. */
 n->params-=2u+((c->features&F_LOD0)?2u:0u);
 n->lists+=lists;n->cached+=lists-clears;n->scaled+=scaled;
 n->lod0+=(c->features&F_LOD0)!=0;n->compiled+=c->compiled;
 for(unsigned i=0;i<12;++i){const uint8_t *x=before+(c->actor-RAM_BASE)+instances_at[i],*y=after+(c->actor-RAM_BASE)+instances_at[i];
  n->refreshed+=rd32(x+0xc)&&(rd32(x+0x14)|rd32(x+0x18))&&!(rd32(y+0x14)|rd32(y+0x18));}
 n->headwear+=after[PLAYER_RECORDS-RAM_BASE+0x68u*after[c->actor-RAM_BASE+0x1363]+0x65u]==0;branches(before,c->actor,n);}
#ifdef NFLSTREET_DRAW_ORACLE
static unsigned oracle_runs,oracle_calls;
static int original_case(const uint8_t *before,const draw_case *c,const trace *t,uint8_t *after,const nflstreet_render_state *s);
static int initialize_oracle(const char *dll);
#endif
/* Deliberate rejections: more than 8 vertex arrays, 5 lights, a type-3
   shader parameter, a chunk past the table. Output must be 0. */
static int rejections(const uint8_t *pristine,uint8_t *ram,trace *t,unsigned *passed){
 for(unsigned k=0;k<4;++k){memcpy(ram,pristine,RAM_BYTES);uint32_t a=live_actor(ram,k);
  uint8_t *in=ram_at(ram,a+0x4ac,0xf0);uint32_t model=rd32(in+0xc),table=ram32(ram,model+8),chunks=ram32(ram,table);
  uint8_t *c=ram_at(ram,chunks+0x18u*rd16(in+0x60),0x18);
  if(k==0)c[0x10]=9;
  else if(k==1)wr32(ram_at(ram,LIGHT_SETS+0x108u,4),5);
  else if(k==2){uint32_t one=ram32(ram,R13-0x5638u);uint8_t *sh=ram_at(ram,PARAM_SHADER,0x20),*r=ram_at(ram,PARAM_RECORDS,0x14);
   memcpy(sh,ram_at(ram,one,0x20),0x20);wr32(sh+0x14,1);wr32(sh+0x18,PARAM_RECORDS);r[0x10]=3;
   uint32_t first=ram32(ram,ram32(ram,a+0xff0));wr32(ram_at(ram,first+8,4),PARAM_SHADER);wr16(c+6,0);}
  else wr16(in+0x60,ram32(ram,table+4));
  face_plan f={0};t->count=0;nflstreet_player_draw_context x=draw_context(ram,&f,t);nflstreet_render_state s;initial_state(ram,&s);
  if(nflstreet_player_draw(&x,a,&s))return 0;++*passed;}
 return 1;}
int main(int argc,char **argv){
#ifdef NFLSTREET_DRAW_ORACLE
 if(argc!=3||!initialize_oracle(argv[2]))return 2;
#else
 if(argc!=2)return 2;
#endif
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);trace t={malloc(CALLS_MAX*sizeof(nflstreet_gx_call)),0};
 if(!pristine||!ram||!before||!t.calls)return 3;
 for(unsigned i=0;i<PLAYERS;++i){uint32_t a=live_actor(pristine,i);if(!ram_at(pristine,a,0x1a90)||pristine[a-RAM_BASE+0x1363]>=PLAYERS)return 4;}
 totals live={0},controlled={0};uint32_t h=2166136261u;unsigned rejected=0;
 for(unsigned index=0;index<PLAYERS+CONTROLLED;++index){
  memcpy(ram,pristine,RAM_BYTES);draw_case c;prepare(ram,index,&c);memcpy(before,ram,RAM_BYTES);
  face_plan f={c.compiled,0};t.count=0;nflstreet_player_draw_context x=draw_context(ram,&f,&t);nflstreet_render_state s;initial_state(ram,&s);
  if(!nflstreet_player_draw(&x,c.actor,&s)){fprintf(stderr,"native draw rejected case %u (actor %08x features %04x)\n",index,c.actor,c.features);return 5;}
  if((c.features&F_LOD0)&&(c.features&F_FACE)?f.requests!=1:f.requests!=0)return 6;
#ifdef NFLSTREET_DRAW_ORACLE
  if(!original_case(before,&c,&t,ram,&s)){fprintf(stderr,"original draw mismatch case %u (actor %08x features %04x)\n",index,c.actor,c.features);return 7;}
#endif
  count(before,ram,&c,&t,x.default_texture,index<PLAYERS?&live:&controlled);
  h=fnv(h,t.calls,t.count*sizeof *t.calls);h=fnv(h,&s,sizeof s);h=fnv(h,ram+(c.actor-RAM_BASE),0x1a90);}
 if(!rejections(pristine,ram,&t,&rejected))return 8;
 printf("Player draw: %u live actors (%u GX calls, %u display lists, %u lights, %u TwoPass, %u TLUT loads, %u default textures, "
  "%u cached formats, %u headwear); %u controlled draws (%u GX calls, %u display lists, %u lights (%u scaled), %u TwoPass, "
  "%u TLUT loads, %u default textures, %u shader parameters, %u cached formats, %u LOD-0 heads (%u compiled), %u refreshed "
  "instances, %u headwear; light color: %u restarts, %u interpolating, %u fading, %u fade ends, %u tints; %u clamped eye angles); "
  "%u rejections; digest %08x.\n",
  live.cases,live.calls,live.lists,live.lights,live.two_pass,live.tluts,live.default_loads,live.cached,live.headwear,
  controlled.cases,controlled.calls,controlled.lists,controlled.lights,controlled.scaled,controlled.two_pass,controlled.tluts,
  controlled.default_loads,controlled.params,controlled.cached,controlled.lod0,controlled.compiled,controlled.refreshed,controlled.headwear,
  controlled.restarts,controlled.interpolating,controlled.fades,controlled.fade_ends,controlled.tints,controlled.eye_clamps,rejected,h);
#ifdef NFLSTREET_DRAW_ORACLE
 printf("Original player draw: %u 80161E10 runs match every GX call (%u calls), the render state and all RAM outside the oracle stack.\n",
  oracle_runs,oracle_calls);
#endif
 free(t.calls);free(before);free(ram);free(pristine);return 0;}
