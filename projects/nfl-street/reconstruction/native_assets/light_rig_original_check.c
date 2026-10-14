/* Native per-frame light update (801ED3A0) against the original on a saved
   context. Each case prepares a fresh copy of the saved RAM and runs the
   update natively and then, from the same RAM, the original 801ED3A0 in the
   preserved module. Every routine executes in the original, the memory-only
   GX SDK light initializers (GXInitLightAttn/Spot/DistAttn/Pos/Dir/
   SpecularDir/Color, 80204DB4..80205158) and cosf included; any other GX
   SDK entry fails the case (gx_oracle.h records none). All RAM outside the
   oracle stack must be equal, with the native render state written back
   (r13-6220 cleared).
   Case 0 is the saved frame. The others edit the view on the stack top, the
   stack selection, the active rig (the nine venue rigs 801ED408 installs,
   or none), light types, world directions, colors (beyond 255, negative
   and beyond 2^31), callbacks (the three the rigs use), the sun direction,
   set counts, entries and color scales, the rig color scale, shadow
   counts, types, lights, angles, scales and colors, and the shadow limit.
   Frame-order cases run the original field camera node's handler and
   viewport setup (801DF458 -> 8017D16C, 801DFAB0, GX SDK entries returning
   at once) before 801ED3A0, against the native field camera view
   (8017D16C), its copy into stack slot 3 (as scene_frame.c) and the
   native update, with edited camera positions and angles; the rig's
   lights, the shadow and light-set area (8034FD8C..803504CC), slot 3 and
   the stack top are compared. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "hud_fixture.h"
#include "gx_oracle.h"
#include "light_rig_fixture.h"
#include <math.h>
#define CASES 401u
#define FRAME_CASES 201u
#define LIGHT_AREA 0x8034fd8cu
#define LIGHT_AREA_END (0x8034fe9cu+6u*0x108u)
#define SLOT3 (0x80331ec0u+0xc0u)
static const uint32_t rigs[9]={0x8028a48cu,0x8028b764u,0x8028ae5cu,0x8028a9d8u,0x8028c06cu,0x8028bbe8u,0x8028b2e0u,0x8028c4f0u,0x8028c974u};
static const uint32_t callbacks[4]={0,0x8015b858u,0x8015b8a4u,0x8015b8c4u};
static int frame_order;
/* The light initializers execute; in frame-order runs every other GX SDK
   entry returns at once (801DFAB0's viewport, projection and scissor). */
static int service(CPUState *c,void *p){(void)p;const uint32_t at=c->pc;
 if(at>=0x80204db4u&&at<0x80205158u){c->downcount=-1000000000;gx_module->dispatch(c,c->pc);
  if(c->exception){fprintf(stderr,"exception at %08x\n",c->pc);return -1;}return 1;}
 if(frame_order&&at>=0x80200000u&&at<0x80210000u){c->pc=c->lr;return 1;}
 return 0;}
static float direction(void){float v=rndf(0.05f,1.0f);return rnd()%2?v:-v;}
static float color(void){switch(rnd()%10){case 0:return rndf(-300.0f,0.0f);case 1:return rndf(3.0e9f,4.0e9f);case 2:return rndf(255.0f,600.0f);default:return rndf(0.0f,255.0f);}}
static uint32_t stack_top(uint8_t *ram){uint32_t s=ram32(ram,R13-0x65e8u);return ram32(ram,s+8)+0x40u*ram32(ram,s+4);}
static void select_stack(uint8_t *ram){nflstreet_matrix_stacks k=venue_stacks(ram);wr32(ram_at(ram,R13-0x65e8u,4),k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0]);}
static void edit_rig(uint8_t *ram){
 if(rnd()%3==0)wr32(ram_at(ram,0x8034fd88u,4),rigs[rnd()%9]);
 if(rnd()%40==0){wr32(ram_at(ram,0x8034fd88u,4),0);return;}
 const uint32_t rig=ram32(ram,0x8034fd88u),lights=ram32(ram,rig+4),count=ram32(ram,rig);
 uint32_t shadow_count=ram32(ram,rig+0x10);if(rnd()%3==0)wr32(ram_at(ram,rig+0x10,4),shadow_count=rnd()%5);
 /* the lights shadow type 1 reads keep a direction off the ground plane */
 for(uint32_t i=0;i<count;++i){uint8_t *l=ram_at(ram,lights+0x30*i,0x30);
  if(rnd()%4==0)wr32(l,rnd()%6);
  if(rnd()%2)for(unsigned k=0;k<3;++k)wr32(l+4+4*k,fbits(direction()*rndf(0.2f,3.0f)));
  if(rnd()%2)for(unsigned k=0;k<4;++k)wr32(l+0x10+4*k,fbits(rnd()%3?rndf(0.0f,2.0f):rndf(-1.0f,4.0f)));
  if(rnd()%3==0)wr32(l+0x20,callbacks[rnd()%4]);
  if(rnd()%4==0)for(unsigned k=0;k<3;++k)wr32(l+0x24+4*k,rnd());} /* stale view directions */
 if(rnd()%2){uint32_t n=ram32(ram,rig+8);if(rnd()%3==0)wr32(ram_at(ram,rig+8,4),n=rnd()%7);
  const uint32_t sets=ram32(ram,rig+12);
  for(uint32_t i=0;i<n;++i)for(unsigned e=0;e<5;++e)if(rnd()%3==0){uint8_t *p=ram_at(ram,sets+100*i+0x14*e,0x14);
   if(rnd()%2)wr32(p,rnd()%count);for(unsigned k=0;k<4;++k)if(rnd()%2)wr32(p+4+4*k,fbits(color()));}}
 if(rnd()%3==0)for(unsigned k=0;k<4;++k)wr32(ram_at(ram,rig+0x94+4*k,4),fbits(rndf(0.0f,2.0f)));
 for(uint32_t i=0;i<shadow_count;++i){uint8_t *r=ram_at(ram,rig+0x14+0x20*i,0x20);
  if(rnd()%2)wr32(r,rnd()%4);if(rnd()%2)wr32(r+0x14,rnd()%count);
  if(rnd()%2)for(unsigned k=0;k<4;++k)wr32(r+4+4*k,fbits(rnd()%5?rndf(0.0f,2.0f):rndf(-1.0f,3.0e7f)));
  if(rnd()%2){wr32(r+0x18,fbits(rndf(-400.0f,400.0f)));wr32(r+0x1c,fbits(rndf(-5.0f,5.0f)));}
  if(rd32(r)==1){const uint32_t la=lights+0x30*rd32(r+0x14);uint8_t *l=ram_at(ram,la,0x30);
   if(rd32(l+0x20)==0x8015b8c4u)wr32(l+0x20,0x8015b858u); /* 8015B8C4 leaves z 0 */
   if(!rd32(l+0x20)&&fabsf(ramf(ram,la+12))<0.05f)wr32(l+12,fbits(direction()));}}
 if(rnd()%3==0)wr32(ram_at(ram,R13-0x6228u,4),fbits(rndf(0.05f,4.0f)));}
/* The sun direction stays off the ground plane for the lights shadows read. */
static void edit_sun(uint8_t *ram){if(rnd()%3==0)for(unsigned k=0;k<3;++k)wr32(ram_at(ram,0x802a3030u+4*k,4),fbits(direction()*rndf(0.3f,2.0f)));}
static void prepare(uint8_t *ram,unsigned index){
 select_stack(ram);if(!index)return;
 if(rnd()%4==0)ram_at(ram,R13-0x4ee8u,1)[0]=(uint8_t)(rnd()%4);
 if(rnd()%3==0)wr32(ram_at(ram,R13-0x65e8u,4),venue_stacks(ram).table+12u*(rnd()%4)); /* a stale selection */
 nflstreet_matrix_stacks k=venue_stacks(ram);const uint32_t s=k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0],top=ram32(ram,s+8)+0x40u*ram32(ram,s+4);
 if(rnd()%4)for(unsigned i=0;i<12;++i)if(rnd()%3)wr32(ram_at(ram,top+4*i,4),fbits(i%4==3?rndf(-60.0f,60.0f):rndf(-1.5f,1.5f)));
 edit_sun(ram);edit_rig(ram);}
typedef struct {unsigned rigs,none,lights[6],callbacks,set_lights,ambient,specular,shadow_types[4];} totals;
static void count(uint8_t *ram,totals *t){
 const uint32_t rig=ram32(ram,0x8034fd88u);if(!rig){++t->none;return;}++t->rigs;
 const uint32_t lights=ram32(ram,rig+4);
 for(uint32_t i=0;i<ram32(ram,rig);++i){const uint32_t type=ram32(ram,lights+0x30*i);++t->lights[type<6?type:5];t->callbacks+=ram32(ram,lights+0x30*i+0x20)?1u:0u;}
 for(uint32_t i=0;i<ram32(ram,rig+8);++i){const uint32_t set=0x8034fe9cu+0x108u*i;t->set_lights+=ram32(ram,set);
  for(uint32_t k=0;k<ram32(ram,set);++k)t->specular+=ramf(ram,set+8+0x40*k+0x1c)==25.0f?1u:0u;
  t->ambient+=ram32(ram,set+4)?1u:0u;}
 for(uint32_t i=0;i<ram32(ram,rig+0x10);++i)++t->shadow_types[ram32(ram,rig+0x14+0x20*i)&3];}
static int native_update(uint8_t *ram,nflstreet_render_state *s){
 nflstreet_player_draw_context x;memset(&x,0,sizeof x);x.span=span;x.memory=ram;
 const nflstreet_light_rig_context c=light_rig_context(ram,&x);return nflstreet_light_rig_update(&c,s);}
static int same_area(uint8_t *native,uint32_t from,uint32_t to,const char *what){
 for(uint32_t a=from;a<to;++a)if(oracle_ram[a-RAM_BASE]!=native[a-RAM_BASE]){
  fprintf(stderr,"%s differs at %08x: original %02x native %02x\n",what,a,oracle_ram[a-RAM_BASE],native[a-RAM_BASE]);return 0;}
 return 1;}
/* Native: the field camera view as scene_frame.c computes it (no GX calls
   expected), then the update. */
static int no_gx(void *sink,const nflstreet_gx_call *c){(void)sink;(void)c;return 0;}
static int native_frame(uint8_t *ram,nflstreet_render_state *s){
 nflstreet_player_draw_context x;memset(&x,0,sizeof x);x.span=span;x.memory=ram;x.emit=no_gx;
 const nflstreet_scenery_context v=venue_context(ram,&x);const nflstreet_hud_context h=hud_context(ram,&v);nflstreet_draw d={&x,s};uint32_t top;
 if(!nflstreet_camera_view(&h,hud_cameras[0],s)||!nflstreet_matrix_top(&d,&h.stacks,&top))return 0;
 memmove(ram_at(ram,SLOT3,0x40),ram_at(ram,top,0x40),0x40);
 const nflstreet_light_rig_context c=light_rig_context(ram,&x);return nflstreet_light_rig_update(&c,s);}
static void prepare_frame(uint8_t *ram,unsigned index){
 select_stack(ram);if(!index)return;
 uint8_t *v=ram_at(ram,ram32(ram,hud_cameras[0]+0x40),0x20);
 for(unsigned k=0;k<3;++k){if(rnd()%2)wr32(v+4+4*k,fbits(rndf(-200.0f,200.0f)));if(rnd()%2)wr32(v+0x14+4*k,rnd()&0xffffffu);}
 edit_sun(ram);if(rnd()%2)edit_rig(ram);}
int main(int argc,char **argv){
 if(argc!=3||!gx_oracle_open(argv[2]))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);if(!pristine||!ram||!before)return 3;
 static nflstreet_gx_call none[1];const trace empty={none,0};const uint32_t no_args[6]={0};unsigned n;
 totals t;memset(&t,0,sizeof t);uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);prepare(ram,index);memcpy(before,ram,RAM_BYTES);
  nflstreet_render_state s;saved_state(ram,&s);
  if(!native_update(ram,&s)){fprintf(stderr,"native update rejected case %u\n",index);return 5;}
  memcpy(oracle_ram,before,RAM_BYTES);
  if(!gx_oracle_run(0x801ed3a0u,no_args,&empty,service,NULL,&n)){fprintf(stderr,"original case %u\n",index);return 6;}
  if(!gx_oracle_same_ram(ram,&s)){fprintf(stderr,"original light update mismatch case %u\n",index);return 7;}
  count(ram,&t);digest=fnv(digest,ram,RAM_BYTES);}
 frame_order=1;unsigned frames=0;int reproduced=0;
 for(unsigned index=0;index<FRAME_CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);prepare_frame(ram,index);memcpy(before,ram,RAM_BYTES);
  nflstreet_render_state s;saved_state(ram,&s);
  if(!native_frame(ram,&s)){fprintf(stderr,"native frame rejected case %u\n",index);return 5;}
  memcpy(oracle_ram,before,RAM_BYTES);const uint32_t node[6]={hud_cameras[0],0,0,0,0,0};
  if(!gx_oracle_run(0x801df458u,node,&empty,service,NULL,&n)||!gx_oracle_run(0x801dfab0u,node,&empty,service,NULL,&n)||
     !gx_oracle_run(0x801ed3a0u,no_args,&empty,service,NULL,&n)){fprintf(stderr,"original frame case %u\n",index);return 6;}
  const uint32_t rig=ram32(ram,0x8034fd88u),lights=rig?ram32(ram,rig+4):0,top=stack_top(ram);
  if(stack_top(oracle_ram)!=top||!same_area(ram,top,top+0x40,"stack top")||!same_area(ram,SLOT3,SLOT3+0x40,"slot 3")||
     !same_area(ram,LIGHT_AREA,LIGHT_AREA_END,"light area")||(rig&&!same_area(ram,lights,lights+0x30*ram32(ram,rig),"rig lights"))){
   fprintf(stderr,"frame-order mismatch case %u\n",index);return 7;}
  if(!index)reproduced=!memcmp(ram+(LIGHT_AREA-RAM_BASE),pristine+(LIGHT_AREA-RAM_BASE),LIGHT_AREA_END-LIGHT_AREA);
  ++frames;digest=fnv(digest,ram+(LIGHT_AREA-RAM_BASE),LIGHT_AREA_END-LIGHT_AREA);}
 printf("Original light update: %u runs of 801ED3A0 (%u with a rig, %u without) match all RAM; lights by type 0-5: %u/%u/%u/%u/%u/%u "
  "(%u with callbacks); %u set lights (%u specular), %u ambient colors; shadows by type 0-3: %u/%u/%u/%u; %u frame-order runs "
  "(801DF458, 801DFAB0, 801ED3A0) match the native field view, slot 3 and update; the saved camera %s the saved light sets; digest %08x.\n",
  CASES,t.rigs,t.none,t.lights[0],t.lights[1],t.lights[2],t.lights[3],t.lights[4],t.lights[5],t.callbacks,t.set_lights,t.specular,t.ambient,
  t.shadow_types[0],t.shadow_types[1],t.shadow_types[2],t.shadow_types[3],frames,reproduced?"reproduces":"does not reproduce",digest);
 return 0;}
