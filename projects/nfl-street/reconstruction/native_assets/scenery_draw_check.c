/* Native scenery draw (801EC8E4) and the EARS venue loops (80165DC4 main
   field, 8015B670 field passes) on a saved context. The saved RAM is fixture
   input; venue objects are those the original scene traversal draws in the
   saved match (main field 8094D31C, passes 80A9753C, located by running the
   original traversal). Controlled cases edit a fresh copy: instance flags,
   depth, ambient, group selection, shader override, bounding box, enable and
   fog state, pass records and pass instance presence. The native render
   state starts from the saved globals. */
#include "scenery_draw.h"
#define CONTROLLED 1200u
#define SCENE_CASES 301u
#define RING_CASES 201u
#define LINE_CASES 201u
#define MARK_CASES 201u
#define MATRIX_CASES 201u
#define RING_MATRIX_CASES 201u
#include "check_fixture.h"
#define MAIN_FIELD VENUE_MAIN_FIELD
#define FIELD_PASSES VENUE_FIELD_PASSES
/* kind 0: both venue loops as saved; 1: loops with edited pass records and
   instances; 2: one instance through 801EC8E4 with edited state; 3: sky
   (801655D8) and actor shadows (801643F4), first as saved, then edited;
   4: the three player-select rings (800E3F7C), as saved, then edited;
   5: the four field lines (800DA074), as saved, then edited;
   6: the ground marks (800D75EC), as saved, then edited; 7: both venue
   loops recomputing their instance matrices from the viewport's stack, as
   saved, then with edited stack tops, object positions and layer offset;
   8: the rings recomputing their matrices, as saved, then edited. */
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
 if(index>=1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES+MARK_CASES+MATRIX_CASES){c->kind=8;
  if(index==1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES+MARK_CASES+MATRIX_CASES)return;
  for(unsigned i=0;i<3;++i){uint8_t *o=ram_at(ram,venue_rings[i],0xfc);static const uint32_t types[4]={5,7,7,3};
   if(rnd()%3==0)wr32(o+0x28,types[rnd()%4]);if(rnd()%6==0)wr32(o+0x20,rd32(o+0x20)^2);
   if(rnd()%2)for(unsigned k=0;k<3;++k)wr32(o+4+4*k,fbits(rndf(-40.0f,40.0f)));if(rnd()%2)wr32(o+0x14,rnd()<<8^rnd());
   if(rnd()%2)wr32(o+0x18,fbits(rndf(0.2f,2.0f)));if(rnd()%2)wr32(o+0x1c,fbits(rndf(0.2f,3.0f)));}
  for(unsigned k=0;k<2;++k)if(rnd()%4==0)ram_at(ram,R13-0x71dcu+k,1)[0]=(uint8_t)(rnd()%3);
  if(rnd()%3==0)ram_at(ram,R13-0x4ee8u,1)[0]=(uint8_t)(rnd()%2?0:3);
  uint32_t stack=ram32(ram,R13-0x65f0u)+12u*ram_at(ram,R13-0x4ee8u,1)[0],top=ram32(ram,stack+8)+0x40u*ram32(ram,stack+4);
  for(unsigned i=0;i<12;++i)if(rnd()%2)wr32(ram_at(ram,top+4*i,4),fbits(i%4==3?rndf(-60.0f,60.0f):rndf(-1.5f,1.5f)));
  return;}
 if(index>=1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES+MARK_CASES){c->kind=7;
  if(index==1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES+MARK_CASES)return;
  if(rnd()%3==0)ram_at(ram,R13-0x4ee8u,1)[0]=(uint8_t)(rnd()%2?0:3);
  uint32_t stack=ram32(ram,R13-0x65f0u)+12u*ram_at(ram,R13-0x4ee8u,1)[0],top=ram32(ram,stack+8)+0x40u*ram32(ram,stack+4);
  for(unsigned i=0;i<12;++i)if(rnd()%2)wr32(ram_at(ram,top+4*i,4),fbits(i%4==3?rndf(-60.0f,60.0f):rndf(-1.5f,1.5f)));
  if(rnd()%3==0)wr32(ram_at(ram,stack+4,4),rnd()%3);
  if(rnd()%3==0)for(unsigned k=0;k<3;++k)wr32(ram_at(ram,MAIN_FIELD+4+4*k,4),fbits(rndf(-20.0f,20.0f)));
  if(rnd()%3==0)for(unsigned k=0;k<3;++k)wr32(ram_at(ram,FIELD_PASSES+4+4*k,4),fbits(rndf(-20.0f,20.0f)));
  if(rnd()%3==0)for(unsigned k=0;k<3;++k)wr32(ram_at(ram,0x8028a090u+4*k,4),fbits(rndf(-5.0f,5.0f)));
  for(unsigned l=0;l<5;++l)if(rnd()%5==0)ram_at(ram,0x8028a004u+0x1cu*l+0x19,1)[0]^=1;
  return;}
 if(index>=1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES){c->kind=6;if(index==1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES)return;
  uint32_t base=ram32(ram,0x802aa7ccu);
  for(unsigned i=0;i<130;++i){uint8_t *m=ram_at(ram,base+0x74u*i,0x74);
   if(rnd()%10==0)m[0x70]=(uint8_t)(m[0x70]?0:(rd32(ram_at(ram,base,0x74)+0x60)?1:0));
   if(m[0x70]&&!rd32(m+0x60))memcpy(m+0x60,ram_at(ram,base+0x60,4),4);
   if(m[0x70]){memcpy(m+0x68,"\0\0\0\2",4);
    if(rnd()%5==0)for(unsigned k=0;k<3;++k)wr32(m+4*k,fbits(rndf(-40.0f,40.0f)));
    if(rnd()%5==0)for(unsigned k=0;k<3;++k)wr32(m+0xc+4*k,fbits(rndf(0.2f,3.0f)));
    if(rnd()%5==0)wr32(m+0x30,rnd()<<8^rnd());}}
  if(rnd()%4==0)wr32(ram_at(ram,R13-0x5b60u,4),rnd()%130);
  if(rnd()%12==0)ram_at(ram,R13-0x7280u,1)[0]=0;
  if(rnd()%3==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);
  if(rnd()%3==0)ram_at(ram,R13-0x4ee8u,1)[0]=(uint8_t)(rnd()%2?0:3);
  uint32_t stack=ram32(ram,R13-0x65f0u)+12u*ram_at(ram,R13-0x4ee8u,1)[0],top=ram32(ram,stack+8)+0x40u*ram32(ram,stack+4);
  if(rnd()%3==0)for(unsigned i=0;i<12;++i)if(rnd()%2)wr32(ram_at(ram,top+4*i,4),fbits(rndf(-30.0f,30.0f)));
  return;}
 if(index>=1+CONTROLLED+SCENE_CASES+RING_CASES){c->kind=5;if(index==1+CONTROLLED+SCENE_CASES+RING_CASES)return;
  for(unsigned i=0;i<4;++i){uint8_t *o=ram_at(ram,venue_lines[i],0x124);
   if(rnd()%3==0)for(unsigned k=0;k<4;++k)wr32(o+0xd4+4*k,fbits(rndf(-0.2f,1.2f)));
   if(rnd()%3==0)wr32(o+0xe0,fbits(rnd()%5==0?0.0f:rndf(-0.3f,1.0f)));if(rnd()%3==0)wr32(o+0xe4,fbits(rnd()%5==0?0.0f:rndf(-0.4f,0.4f)));
   if(rnd()%3==0)for(unsigned k=0;k<3;++k)wr32(o+4+4*k,fbits(rndf(-60.0f,60.0f)));if(rnd()%8==0)wr32(o+0x11c,0);}
  if(rnd()%4==0)wr32(ram_at(ram,R13-0x7260u,4),fbits(rndf(-0.5f,1.5f)));
  uint32_t gs=ram32(ram,R13-0x7078u);if(rnd()%5==0)wr32(ram_at(ram,gs+0x30,4),rnd()%2);if(rnd()%4==0)wr32(ram_at(ram,gs+0x90,4),fbits(rndf(1.0f,40.0f)));
  if(rnd()%8==0)ram_at(ram,ram32(ram,R13-0x7b88u)+0x182,1)[0]=1;
  if(rnd()%8==0)wr32(ram_at(ram,ram32(ram,R13-0x7f40u)+0xb64,4),rd32(ram_at(ram,ram32(ram,R13-0x7f40u)+0xb64,4))^(0x80u<<(rnd()%4*4)));
  if(rnd()%8==0)ram_at(ram,R13-0x7258u,1)[0]=(uint8_t)(rnd()%3);if(rnd()%8==0)wr32(ram_at(ram,0x80308560u,4),rnd()%3-1u);
  if(rnd()%8==0)wr32(ram_at(ram,ram32(ram,R13-0x7cf8u),4),rnd()%2?7u:rnd()%10);
  uint32_t stack=ram32(ram,R13-0x65e8u),top=ram32(ram,stack+8)+0x40u*ram32(ram,stack+4);
  if(rnd()%3==0)for(unsigned i=0;i<12;++i)if(rnd()%2)wr32(ram_at(ram,top+4*i,4),fbits(rndf(-30.0f,30.0f)));
  if(rnd()%4==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);
  return;}
 if(index>=1+CONTROLLED+SCENE_CASES){c->kind=4;if(index==1+CONTROLLED+SCENE_CASES)return;
  for(unsigned i=0;i<3;++i){uint8_t *o=ram_at(ram,venue_rings[i],0xfc);static const uint32_t types[4]={5,7,7,3};
   if(rnd()%3==0)wr32(o+0x28,types[rnd()%4]);if(rnd()%4==0)wr32(o+0x30,rnd()%4);if(rnd()%6==0)wr32(o+0x20,rd32(o+0x20)^2);
   if(rnd()%8==0)wr32(o+0xf8,0);if(rnd()%3==0)edit_instance(ram,venue_rings[i]+0x3c);}
  for(unsigned k=0;k<2;++k)if(rnd()%4==0)ram_at(ram,R13-0x71dcu+k,1)[0]=(uint8_t)(rnd()%3);
  if(rnd()%4==0)for(unsigned k=0;k<16;++k)wr32(ram_at(ram,0x8027bed4u+4*k,4),fbits(rndf(-0.2f,1.4f)));
  return;}
 if(index>=1+CONTROLLED){c->kind=3;if(index==1+CONTROLLED)return;edit_scene(ram);return;}
 c->kind=2;c->instance=venue_instance(rnd()%20);c->lights=rnd()%256;edit_instance(ram,c->instance);
 if(rnd()%5==0)boundary_box(ram,c->instance);
 if(rnd()%8==0)wr32(ram_at(ram,R13-0x6230u,4),0);
 if(rnd()%3==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);
 if(rnd()%6==0)wr32(ram_at(ram,R13-0x6218u,4),0);
 if(rnd()%4==0)wr32(ram_at(ram,R13-0x562cu,4),rnd()%8);
 if(rnd()%4==0)wr32(ram_at(ram,R13-0x622cu,4),fbits(rndf(0.0f,0.01f)));}
static int run_native(uint8_t *ram,const scenery_case *c,trace *t,nflstreet_render_state *s){
 nflstreet_player_draw_context x;nflstreet_scenery_context sc=scenery_context(ram,&x,t);t->count=0;saved_state(ram,s);
 if(c->kind==2)return nflstreet_scenery_draw(&sc,c->instance,c->lights,s);
 if(c->kind==3)return nflstreet_sky_draw(&sc,s)&&nflstreet_shadows_draw(&sc,s);
 if(c->kind==8){nflstreet_matrix_stacks k=venue_stacks(ram);sc.stacks=&k;
  for(unsigned i=0;i<3;++i)if(!nflstreet_ring_draw(&sc,venue_rings[i],s))return 0;return 1;}
 if(c->kind==7){nflstreet_matrix_stacks k=venue_stacks(ram);sc.stacks=&k;
  return nflstreet_field_passes_draw(&sc,FIELD_PASSES,s)&&nflstreet_field_draw(&sc,MAIN_FIELD,s);}
 if(c->kind==6){nflstreet_marks_context mc=marks_context(ram,&sc);return nflstreet_marks_draw(&mc,s);}
 if(c->kind==5){for(unsigned i=0;i<4;++i)if(!nflstreet_line_draw(&sc,venue_lines[i],s))return 0;return 1;}
 if(c->kind==4){for(unsigned i=0;i<3;++i)if(!nflstreet_ring_draw(&sc,venue_rings[i],s))return 0;return 1;}
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
 totals live={0},loops={0},single={0},scene_live={0},scene={0},ring_live={0},rings={0},line_live={0},lines={0},mark_live={0},marks={0},matrix_live={0},matrices={0},ring_matrix_live={0},ring_matrices={0};uint32_t h=2166136261u;unsigned culled=0;
 for(unsigned index=0;index<1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES+MARK_CASES+MATRIX_CASES+RING_MATRIX_CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);scenery_case c;prepare(ram,index,&c);memcpy(before,ram,RAM_BYTES);
  nflstreet_render_state s;if(!run_native(ram,&c,&t,&s)){fprintf(stderr,"native scenery rejected case %u after %u calls\n",index,t.count);
   if(t.count){const nflstreet_gx_call *g=t.calls+t.count-1;fprintf(stderr,"  last op %d args %08x %08x %08x\n",(int)g->op,g->arg[0],g->arg[1],g->arg[2]);}
   for(unsigned i=0;i<20;++i){uint32_t in=venue_instance(i);fprintf(stderr,"  inst %08x flags %08x grp %08x 78 %08x b8 %08x\n",in,ram32(before,in),ram32(before,in+4),ram32(before,in+0x78),ram32(before,in+0xb8));}
   return 5;}
#ifdef NFLSTREET_SCENERY_ORACLE
  if(!original_case(before,&c,&t,ram,&s)){fprintf(stderr,"original scenery mismatch case %u (kind %u instance %08x)\n",index,c.kind,c.instance);return 7;}
#endif
  if(c.kind==2&&ram32(before,R13-0x6230u)&&ram32(before,c.instance+0x54)&&!t.count)++culled;
  count(&t,c.kind==0?&live:c.kind==1?&loops:c.kind==2?&single:
   c.kind==8?(index==1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES+MARK_CASES+MATRIX_CASES?&ring_matrix_live:&ring_matrices):
   c.kind==7?(index==1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES+MARK_CASES?&matrix_live:&matrices):
   c.kind==6?(index==1+CONTROLLED+SCENE_CASES+RING_CASES+LINE_CASES?&mark_live:&marks):c.kind==5?(index==1+CONTROLLED+SCENE_CASES+RING_CASES?&line_live:&lines):
   c.kind==4?(index==1+CONTROLLED+SCENE_CASES?&ring_live:&rings):
   index==1+CONTROLLED?&scene_live:&scene);
  h=fnv(h,t.calls,t.count*sizeof *t.calls);h=fnv(h,&s,sizeof s);}
 printf("Scenery draw: EARS venue as saved (%u GX calls, %u display lists, %u instances drawn, %u depth ranges, %u fog calls); "
  "%u edited venue loops (%u GX calls, %u display lists, %u instances drawn); %u single instances (%u GX calls, %u display lists, "
  "%u drawn, %u frustum-culled, %u fog calls); sky and shadows as saved (%u GX calls, %u primitives, %u pipe writes) and %u edited "
  "(%u GX calls, %u primitives, %u pipe writes); rings as saved (%u GX calls, %u display lists) and %u edited (%u GX calls, "
  "%u display lists); lines as saved (%u GX calls, %u pipe writes) and %u edited (%u GX calls, %u primitives, %u pipe writes); "
  "ground marks as saved (%u GX calls, %u marks) and %u edited (%u GX calls, %u marks); recomputed venue matrices as saved "
  "(%u GX calls, %u instances) and %u edited (%u GX calls, %u instances); recomputed rings as saved (%u GX calls) and %u edited "
  "(%u GX calls, %u display lists); digest %08x.\n",live.calls,live.lists,live.instances_drawn,live.depth_ranges,live.fog_calls,
  loops.cases,loops.calls,loops.lists,loops.instances_drawn,single.cases,single.calls,single.lists,single.instances_drawn,culled,single.fog_calls,
  scene_live.calls,scene_live.primitives,scene_live.writes,scene.cases,scene.calls,scene.primitives,scene.writes,
  ring_live.calls,ring_live.lists,rings.cases,rings.calls,rings.lists,line_live.calls,line_live.writes,lines.cases,lines.calls,
  lines.primitives,lines.writes,mark_live.calls,mark_live.primitives,marks.cases,marks.calls,marks.primitives,
  matrix_live.calls,matrix_live.instances_drawn,matrices.cases,matrices.calls,matrices.instances_drawn,ring_matrix_live.calls,
  ring_matrices.cases,ring_matrices.calls,ring_matrices.lists,h);
#ifdef NFLSTREET_SCENERY_ORACLE
 printf("Original scenery draw: %u runs of 801EC8E4, 80165DC4, 8015B670, 801655D8, 801643F4, 800E3F7C, 800DA074 and 800D75EC (with and without the venue matrix recompute) match every GX call (%u calls), the render state and all RAM outside the oracle stack.\n",
  oracle_runs,oracle_calls);
#endif
 free(t.calls);free(before);free(ram);free(pristine);return 0;}
