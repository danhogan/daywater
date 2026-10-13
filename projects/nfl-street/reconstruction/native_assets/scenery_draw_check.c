/* Native scenery draw (801EC8E4) and the EARS venue loops (80165DC4 main
   field, 8015B670 field passes) on a saved context. The saved RAM is fixture
   input; venue objects are those the original scene traversal draws in the
   saved match (main field 8094D31C, passes 80A9753C, located by running the
   original traversal). Controlled cases edit a fresh copy: instance flags,
   depth, ambient, group selection, shader override, bounding box, enable and
   fog state, pass records and pass instance presence. The native render
   state starts from the saved globals. */
#include "scenery_draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define CONTROLLED 1200u
#define SCENE_CASES 301u
#define CALLS_MAX 32768u
#define STACK_LO 0x80f38000u
#define STACK_HI 0x80f47000u
#define STATE 0x8039e560u
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *ram_at(uint8_t *ram,uint32_t a,uint32_t n){return a>=RAM_BASE&&(uint64_t)a-RAM_BASE+n<=RAM_BYTES?ram+(a-RAM_BASE):NULL;}
static uint32_t ram32(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,4);return p?rd32(p):0;}
static float ramf(uint8_t *ram,uint32_t a){uint32_t v=ram32(ram,a);float f;memcpy(&f,&v,4);return f;}
#include "venue_fixture.h"
#define MAIN_FIELD VENUE_MAIN_FIELD
#define FIELD_PASSES VENUE_FIELD_PASSES
static uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint8_t *load_ram(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *b=malloc(RAM_BYTES);
 if(!b||fread(b,1,RAM_BYTES,f)!=RAM_BYTES){free(b);fclose(f);return NULL;}fclose(f);return b;}
static uint8_t *span(void *m,uint32_t token,uint32_t size){
 if((uint64_t)token+size>STACK_LO&&token<STACK_HI)return NULL;return ram_at(m,token,size);}
typedef struct {nflstreet_gx_call *calls;unsigned count;} trace;
static int record(void *t,const nflstreet_gx_call *c){trace *r=t;if(r->count>=CALLS_MAX)return 0;r->calls[r->count++]=*c;return 1;}
static uint32_t seed=0x13579bdu;
static uint32_t rnd(void){seed=seed*1103515245u+12345u;return seed>>8;}
static float rndf(float lo,float hi){return lo+(hi-lo)*(float)(rnd()&0xffffu)/65535.0f;}
static uint32_t fbits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
/* kind 0: both venue loops as saved; 1: loops with edited pass records and
   instances; 2: one instance through 801EC8E4 with edited state; 3: sky
   (801655D8) and actor shadows (801643F4), first as saved, then edited. */
typedef struct {unsigned kind;uint32_t instance,lights;} scenery_case;
static const uint32_t flag_bits[13]={1,2,8,0x200,0x4000,0x8000,0x10000,0x20000,0x40000,0x80000,0x100000,0x200000,0x400000};
static uint32_t venue_instance(unsigned i){
 static const uint16_t first[5]={0x258,0xe0,0x19c,0x24,0x314};
 if(i<5)return MAIN_FIELD+first[i];if(i<15)return MAIN_FIELD+0x3d0u+0xbcu*(i-5);return FIELD_PASSES+0x2cu+0xbcu*(i-15);}
static void edit_instance(uint8_t *ram,uint32_t inst){
 uint8_t *in=ram_at(ram,inst,0xbc);uint32_t flags=rd32(in);
 for(unsigned k=0;k<13;++k)if(rnd()%3==0)flags^=flag_bits[k];
 wr32(in,flags);if(rnd()%3==0)wr32(in+0x48,fbits(rndf(0.0f,1.0f)));if(rnd()%3==0)wr32(in+0x4c,rnd()<<8^rnd());
 if(rnd()%6==0)wr32(in+0x78,0);if(rnd()%4==0)wr32(in+4,rnd()%5==0?0xffffffffu:rnd()%20);
 if(rnd()%6==0){uint32_t sh[3]={ram32(ram,R13-0x5638u),ram32(ram,R13-0x5634u),ram32(ram,R13-0x5684u)};wr32(in+0xb8,sh[rnd()%3]);}
 if(rnd()%3==0){float c[3],e[3];for(unsigned k=0;k<3;++k){c[k]=rndf(-200.0f,200.0f);e[k]=rndf(0.1f,300.0f);}
  for(unsigned k=0;k<3;++k){wr32(in+0x98+4*k,fbits(c[k]-e[k]));wr32(in+0xa8+4*k,fbits(c[k]+e[k]));}}}
/* A box outside the left (x) or bottom (y) clip plane except for the
   corners of its high face, which lie exactly on it: identity model-view,
   z_low = P0*x_high (or P5*y_high), so those corners project to x = -w. */
static void boundary_box(uint8_t *ram,uint32_t inst){
 uint8_t *in=ram_at(ram,inst,0xbc);const float p0=ramf(ram,0x80331f40u),p5=ramf(ram,0x80331f40u+0x14);
 for(unsigned i=0;i<12;++i)wr32(in+8+4*i,fbits(i%5==0?1.0f:0.0f));
 float lo[3],hi[3];unsigned axis=rnd()%2;
 for(unsigned k=0;k<2;++k){lo[k]=rndf(-20.0f,20.0f);hi[k]=lo[k]+rndf(0.5f,10.0f);}
 hi[axis]=rndf(-40.0f,-1.0f);lo[axis]=hi[axis]-rndf(0.5f,10.0f);
 lo[2]=axis?p5*hi[1]:p0*hi[0];hi[2]=lo[2]-lo[2]*rndf(0.0f,0.9f);
 for(unsigned k=0;k<3;++k){wr32(in+0x98+4*k,fbits(lo[k]));wr32(in+0xa8+4*k,fbits(hi[k]));}
 wr32(in,rd32(in)|0x4000u);}
/* Sky colors (including out-of-range values), dome counts, matrix-stack
   depth and top, actor shadow flags/colors/corners, the actor count, the
   shadow texture and the fog byte. */
static void edit_scene(uint8_t *ram){
 for(unsigned i=0;i<5;++i)for(unsigned k=0;k<3;++k)if(rnd()%2)wr32(ram_at(ram,0x8038668cu+16*i+4*k,4),fbits(rndf(-0.5f,1.5f)));
 uint8_t *counts=ram_at(ram,0x8038758eu,6);if(rnd()%4==0){counts[2]=0;counts[3]=(uint8_t)(rnd()%33);}if(rnd()%4==0){counts[4]=0;counts[5]=(uint8_t)(rnd()%9);}
 uint32_t stack=ram32(ram,R13-0x65e8u);if(rnd()%3==0)wr32(ram_at(ram,stack+4,4),rnd()%3);
 uint32_t top=ram32(ram,stack+8)+0x40u*ram32(ram,stack+4);for(unsigned i=0;i<12;++i)if(rnd()%2)wr32(ram_at(ram,top+4*i,4),fbits(rndf(-50.0f,50.0f)));
 uint32_t list=ram32(ram,R13-0x7bbcu);
 for(unsigned a=0;a<14;++a){uint8_t *ac=ram_at(ram,ram32(ram,list+4*a),0x1340);if(rnd()%3==0)ac[0x133d]=(uint8_t)(rnd()%2);
  if(rnd()%3==0)for(unsigned k=0;k<4;++k)wr32(ac+0x109c+4*k,fbits(rndf(-0.2f,1.2f)));
  if(rnd()%3==0)for(unsigned k=0;k<20;++k)wr32(ac+0x10ac+4*k,fbits(rndf(-100.0f,100.0f)));}
 if(rnd()%5==0)ram_at(ram,R13-0x7bb8u,2)[1]=(uint8_t)(rnd()%15);
 if(rnd()%6==0)wr32(ram_at(ram,R13-0x58e4u,4),0);
 if(rnd()%3==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);}
static void prepare(uint8_t *ram,unsigned index,scenery_case *c){
 memset(c,0,sizeof *c);c->lights=1;
 if(index<1)return;
 if(index<1+CONTROLLED/4){c->kind=1;
  for(unsigned l=0;l<5;++l){uint8_t *rec=ram_at(ram,0x8028a004u+0x1cu*l,0x1c);
   if(rnd()%4==0)rec[0x19]=(uint8_t)(rnd()%2);if(rnd()%3==0)wr32(rec+0xc,fbits(rnd()%2?0.0f:rndf(-0.5f,0.5f)));if(rnd()%3==0)wr32(rec+0x14,fbits(rndf(0.0f,1.0f)));
   if(rnd()%8==0)wr32(ram_at(ram,FIELD_PASSES+0x3d8u+4u*l,4),0);}
  if(rnd()%3==0)wr32(ram_at(ram,R13-0x58f4u,4),fbits(rndf(0.0f,1.0f)));
  if(rnd()%3==0)wr32(ram_at(ram,0x8028a09cu,4),fbits(rndf(0.0f,1.0f)));
  for(unsigned i=0;i<20;++i)if(rnd()%5==0)edit_instance(ram,venue_instance(i));
  if(rnd()%4==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);
  return;}
 if(index>=1+CONTROLLED){c->kind=3;if(index==1+CONTROLLED)return;edit_scene(ram);return;}
 c->kind=2;c->instance=venue_instance(rnd()%20);c->lights=rnd()%256;edit_instance(ram,c->instance);
 if(rnd()%5==0)boundary_box(ram,c->instance);
 if(rnd()%8==0)wr32(ram_at(ram,R13-0x6230u,4),0);
 if(rnd()%3==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);
 if(rnd()%6==0)wr32(ram_at(ram,R13-0x6218u,4),0);
 if(rnd()%4==0)wr32(ram_at(ram,R13-0x562cu,4),rnd()%8);
 if(rnd()%4==0)wr32(ram_at(ram,R13-0x622cu,4),fbits(rndf(0.0f,0.01f)));}
static nflstreet_scenery_context scenery_context(uint8_t *ram,nflstreet_player_draw_context *x,trace *t){
 memset(x,0,sizeof *x);x->span=span;x->memory=ram;x->emit=record;x->sink=t;
 x->default_texture=ram32(ram,R13-0x640cu);x->null_shader=ram32(ram,R13-0x5684u);
 x->setups[0]=0x801c9d0cu;x->setups[1]=0x801c9dacu;x->setups[2]=0x801c9dfcu;x->setups[3]=0x801c9d5cu;
 return venue_context(ram,x);}
/* Native render state from the saved globals (8039E560.., r13-6220/-5630, 80293A54..). */
static void saved_state(uint8_t *ram,nflstreet_render_state *s){
 uint32_t *w[]={&s->chunks,&s->palette,&s->model_10,&s->groups,&s->materials,&s->instance,&s->shader,&s->blend,&s->overrides};
 for(unsigned i=0;i<9;++i)*w[i]=ram32(ram,STATE+4*i);
 for(unsigned i=0;i<6;++i)s->viewport[i]=ramf(ram,STATE+0x24+4*i);
 s->effects=ram32(ram,STATE+0x3c);s->format=ram32(ram,STATE+0x60);s->model_04=ram32(ram,STATE+0x64);s->model_24=ram32(ram,STATE+0x68);
 s->model_28=ram32(ram,STATE+0x6c);s->model_30=ram32(ram,STATE+0x70);s->model_20=ram32(ram,STATE+0x88);s->override=ram32(ram,STATE+0x8c);
 s->lod=ram32(ram,STATE+0x90);s->light_set=ram32(ram,R13-0x6220u);s->light_mask=ram32(ram,R13-0x5630u);
 for(unsigned i=0;i<4;++i)s->matrix_cache[i]=ram32(ram,0x80293a54u+4*i);}
static int run_native(uint8_t *ram,const scenery_case *c,trace *t,nflstreet_render_state *s){
 nflstreet_player_draw_context x;nflstreet_scenery_context sc=scenery_context(ram,&x,t);t->count=0;saved_state(ram,s);
 if(c->kind==2)return nflstreet_scenery_draw(&sc,c->instance,c->lights,s);
 if(c->kind==3)return nflstreet_sky_draw(&sc,s)&&nflstreet_shadows_draw(&sc,s);
 return nflstreet_field_passes_draw(&sc,FIELD_PASSES,s)&&nflstreet_field_draw(&sc,MAIN_FIELD,s);}
#ifdef NFLSTREET_SCENERY_ORACLE
static unsigned oracle_runs,oracle_calls;
static int initialize_oracle(const char *dll);
static int original_case(const uint8_t *before,const scenery_case *c,const trace *t,uint8_t *after,const nflstreet_render_state *s);
#endif
typedef struct {unsigned cases,calls,lists,culled_cases,fog_calls,depth_ranges,instances_drawn,primitives,writes;} totals;
static void count(const trace *t,totals *n){++n->cases;n->calls+=t->count;
 for(unsigned i=0;i<t->count;++i){nflstreet_gx_op op=t->calls[i].op;n->lists+=op==NFLSTREET_GX_CALL_DISPLAY_LIST;n->fog_calls+=op==NFLSTREET_GX_SET_FOG;
  n->depth_ranges+=op==NFLSTREET_GX_SET_VIEWPORT;n->instances_drawn+=op==NFLSTREET_GX_LOAD_POS_MTX;n->primitives+=op==NFLSTREET_GX_BEGIN;
  n->writes+=op==NFLSTREET_GX_WRITE;}}
int main(int argc,char **argv){
#ifdef NFLSTREET_SCENERY_ORACLE
 if(argc!=3||!initialize_oracle(argv[2]))return 2;
#else
 if(argc!=2)return 2;
#endif
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);trace t={malloc(CALLS_MAX*sizeof(nflstreet_gx_call)),0};
 if(!pristine||!ram||!before||!t.calls)return 3;
 if(!venue_present(pristine))return 4;
 totals live={0},loops={0},single={0},scene_live={0},scene={0};uint32_t h=2166136261u;unsigned culled=0;
 for(unsigned index=0;index<1+CONTROLLED+SCENE_CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);scenery_case c;prepare(ram,index,&c);memcpy(before,ram,RAM_BYTES);
  nflstreet_render_state s;if(!run_native(ram,&c,&t,&s)){fprintf(stderr,"native scenery rejected case %u after %u calls\n",index,t.count);
   if(t.count){const nflstreet_gx_call *g=t.calls+t.count-1;fprintf(stderr,"  last op %d args %08x %08x %08x\n",(int)g->op,g->arg[0],g->arg[1],g->arg[2]);}
   for(unsigned i=0;i<20;++i){uint32_t in=venue_instance(i);fprintf(stderr,"  inst %08x flags %08x grp %08x 78 %08x b8 %08x\n",in,ram32(before,in),ram32(before,in+4),ram32(before,in+0x78),ram32(before,in+0xb8));}
   return 5;}
#ifdef NFLSTREET_SCENERY_ORACLE
  if(!original_case(before,&c,&t,ram,&s)){fprintf(stderr,"original scenery mismatch case %u (kind %u instance %08x)\n",index,c.kind,c.instance);return 7;}
#endif
  if(c.kind==2&&ram32(before,R13-0x6230u)&&ram32(before,c.instance+0x54)&&!t.count)++culled;
  count(&t,c.kind==0?&live:c.kind==1?&loops:c.kind==2?&single:index==1+CONTROLLED?&scene_live:&scene);
  h=fnv(h,t.calls,t.count*sizeof *t.calls);h=fnv(h,&s,sizeof s);}
 printf("Scenery draw: EARS venue as saved (%u GX calls, %u display lists, %u instances drawn, %u depth ranges, %u fog calls); "
  "%u edited venue loops (%u GX calls, %u display lists, %u instances drawn); %u single instances (%u GX calls, %u display lists, "
  "%u drawn, %u frustum-culled, %u fog calls); sky and shadows as saved (%u GX calls, %u primitives, %u pipe writes) and %u edited "
  "(%u GX calls, %u primitives, %u pipe writes); digest %08x.\n",live.calls,live.lists,live.instances_drawn,live.depth_ranges,live.fog_calls,
  loops.cases,loops.calls,loops.lists,loops.instances_drawn,single.cases,single.calls,single.lists,single.instances_drawn,culled,single.fog_calls,
  scene_live.calls,scene_live.primitives,scene_live.writes,scene.cases,scene.calls,scene.primitives,scene.writes,h);
#ifdef NFLSTREET_SCENERY_ORACLE
 printf("Original scenery draw: %u runs of 801EC8E4, 80165DC4, 8015B670, 801655D8 and 801643F4 match every GX call (%u calls), the render state and all RAM outside the oracle stack.\n",
  oracle_runs,oracle_calls);
#endif
 free(t.calls);free(before);free(ram);free(pristine);return 0;}
