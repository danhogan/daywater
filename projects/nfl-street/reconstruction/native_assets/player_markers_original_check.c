/* Native player-select rings (800E4A24), camera slots (the tail of 801DFAB0)
   and name tag projection (800E20E4 / 800DB0E4) against the original on a
   saved context. Each case prepares a fresh copy of the saved RAM, runs the
   native routine, then the preserved original module from the same RAM, and
   compares all RAM outside the oracle stack. Every routine executes in the
   original; the only skip is the player draw 80164000 that 800E20E4 ends
   with, which returns at once (as the original routine returning early).
   The camera slots run the original 801DFAB0 from its first selection of
   the viewport stack (801DFBE8) to the call that follows its last pop
   (801DFC70), with the scratch pointer r30 on the oracle stack, because the
   earlier part builds slots 1 and 2 from camera node parameters and is not
   modeled. Case 0 is the saved frame; the others edit actor positions,
   mirroring, pads and their table, the controller entry, ring flags,
   references, angles and sizes, the animated pool, the stack, the camera
   slots, tags, the camera node and the constants the routines read. */
#include <math.h>
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "hud_fixture.h"
#include "select_rings.h"
#include "camera_slots.h"
#include "name_tag_project.h"
#define CASES 401u
#define ACTORS 14u
#define POOL 0x802ac020u
#define A_TABLE 0x8027be8cu
#define B_TABLE 0x8027beb0u
#define SLOTS 0x80331ec0u
#define NODE 0x806a1ca8u /* the field camera node 801DEF50 sets at r13-4EEC while it draws */
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;
static int run_original(uint32_t pc,const uint32_t *args,unsigned n,uint32_t stop,const uint32_t *regs);
static int field_camera_slots(uint8_t *ram);
static uint32_t gameplay(uint8_t *ram,unsigned k){uint32_t t=ram32(ram,R13-0x7c30u);return ram32(ram,t)+k*0x14d4u;}
static uint32_t visual(uint8_t *ram,unsigned i){return ram32(ram,ram32(ram,R13-0x7bbcu)+4*i);}
static void setw(uint8_t *ram,uint32_t a,uint32_t v){wr32(ram_at(ram,a,4),v);}
static void setf(uint8_t *ram,uint32_t a,float v){setw(ram,a,fbits(v));}
static void setb(uint8_t *ram,uint32_t a,uint8_t v){ram_at(ram,a,1)[0]=v;}
static int chance(unsigned n){return rnd()%n==0;}
static uint32_t pick_actor(uint8_t *ram){return gameplay(ram,rnd()%ACTORS);}
/* A reference word naming a gameplay actor (kind 1, team, index) of the table. */
static uint32_t pick_ref(void){uint32_t team=rnd()%2;return 1u|team<<8|(rnd()%7)<<16;}
/* Occasionally replace one of the constants the routines read. */
static void edit_constants(uint8_t *ram,const uint32_t *k,unsigned n,float lo,float hi){
 for(unsigned i=0;i<n;++i)if(chance(5))setf(ram,k[i],rndf(lo,hi));}
/* A float v with v * k == a exactly, when one exists near a / k (sizes that land on the animation's comparison thresholds). */
static float exact_div(float a,float k){
 float v=a/k;for(int i=0;i<8;++i){const float t=v*k;if(t==a)return v;v=nextafterf(v,t<a?INFINITY:-INFINITY);}return a/k;}
static void prepare_rings(uint8_t *ram,unsigned index){
 if(!index)return;
 {static const uint32_t k[]={0x802536f4u,0x802536f8u,0x802536fcu,0x80253700u,0x80253704u,0x80253708u,0x8025370cu,0x80253710u,0x80253718u,0x80253758u};
  edit_constants(ram,k,10,0.2f,3.0f);}
 const uint32_t record=ram32(ram,R13-0x7078u),pads=ram32(ram,R13-0x7c10u);
 if(chance(4))setb(ram,record+0x39,(uint8_t)(rnd()%3));if(chance(12))setw(ram,R13-0x7078u,0);
 for(unsigned k=0;k<ACTORS;++k){uint32_t g=gameplay(ram,k),v=ram32(ram,g+4);
  if(chance(2)){setf(ram,g+0x1a8,rndf(-40.0f,40.0f));setf(ram,g+0x1ac,rndf(-40.0f,40.0f));}
  if(chance(3))setw(ram,g+0xc,(ram32(ram,g+0xc)&~0x04000400u)|(chance(2)?0x400u:0)|(chance(2)?0x04000000u:0));
  if(chance(4))setw(ram,v+0x14,ram32(ram,v+0x14)^1u);
  if(chance(5))setw(ram,g,(chance(5)?2u:1u)|(rnd()%6)<<8|(rnd()%7)<<16);} /* the actor's own identity word: teams beyond the two pads */
 {static const uint8_t pv[5]={0xff,0xff,0,1,2};if(chance(2))setb(ram,pads+6,pv[rnd()%5]);if(chance(2))setb(ram,pads+7,pv[rnd()%5]);if(chance(2))setb(ram,pads+8,pv[rnd()%5]);}
 {const uint32_t t0=ram32(ram,pads);for(unsigned e=0;e<8;++e)if(chance(2)){if(chance(5)){setb(ram,t0+4*e,0xff);}else{setb(ram,t0+4*e,(uint8_t)(rnd()%2));setb(ram,t0+4*e+1,(uint8_t)(rnd()%7));}}}
 uint32_t controller_ref;
 {const uint32_t base=ram32(ram,R13-0x73d0u);if(chance(4))setb(ram,base+1,(uint8_t)(rnd()%4));
  const uint32_t entry=base+ram_at(ram,base+1,1)[0]*0xf4u+8u;
  if(chance(2)||ram_at(ram,base+1,1)[0])setw(ram,entry+0xb4,chance(6)?0u:pick_ref());controller_ref=ram32(ram,entry+0xb4);}
 for(unsigned i=0;i<9;++i){const uint32_t o=ram32(ram,A_TABLE+4*i),b=ram32(ram,B_TABLE+4*i);uint32_t flags=0;
  if(chance(5))flags|=rnd()%0x7fu&~2u;if(!chance(6))flags|=2;if(chance(3))flags|=4;if(chance(2))flags|=0x10;
  setw(ram,o+0x20,flags);setw(ram,b+0x20,rnd()&0x7fu);
  if(i<8){const uint32_t e=chance(6)?0xffffffffu:rnd()%8;setw(ram,o+0x2c,e);
   /* often let a ring follow the controlled player through its table entry */
   if(e<8&&(controller_ref&0xff)==1&&chance(3)){const uint32_t t0=ram32(ram,pads);setb(ram,t0+4*e,(uint8_t)(controller_ref>>8));setb(ram,t0+4*e+1,(uint8_t)(controller_ref>>16));}}
  setw(ram,o+0x100,(flags&4)||chance(2)?pick_ref():rnd()%2?2u:0u);
  setw(ram,o+0xfc,chance(2)?0u:pick_actor(ram));
  setw(ram,o+0x14,rnd()&0xffffffu);setw(ram,o+0x30,rnd()%4);
  for(unsigned w=1;w<4;++w)if(chance(3))setf(ram,o+4*w,rndf(-40.0f,40.0f));}
 {const uint32_t ring=ram32(ram,R13-0x71e0u);setw(ram,ring+0x20,rnd()&0x7fu);setw(ram,ring+0x14,rnd()&0xffffffu);setw(ram,ring+0x30,rnd()%4);
  for(unsigned w=1;w<4;++w)if(chance(3))setf(ram,ring+4*w,rndf(-40.0f,40.0f));}
 for(unsigned j=0;j<5;++j){const uint32_t o=ram32(ram,POOL+4*j);uint32_t flags=0;
  /* Three kinds: free rings (no target; the pool starts these), rings that follow a target (0x20, 0x40) and rings that shrink or grow (4, 8),
     which can drop their target and so never combine with the following forms that read it. */
  const unsigned kind=rnd()%3;const int following=kind==0,free_ring=kind==2;
  if(!chance(6))flags|=2;if(chance(2))flags|=0x10;
  if(following)flags|=chance(3)?0x60u:chance(2)?0x20u:0x40u;else if(!free_ring)flags|=chance(3)?0xcu:chance(2)?4u:8u;
  setw(ram,o+0x20,flags);
  setf(ram,o+0x38,rndf(0.5f,3.5f));
  setf(ram,o+0x18,chance(3)?rndf(0.5f,2.0f):chance(2)?rndf(5.0f,9.5f):rndf(0.2f,3.0f));
  /* sizes exactly on the thresholds: 1.0, the cap after the following growth, the limit after the growth */
  if(chance(6))setf(ram,o+0x18,ramf(ram,0x802536fcu));else if(chance(6))setf(ram,o+0x18,exact_div(ramf(ram,o+0x38),ramf(ram,0x8025370cu)));
  else if(chance(8))setf(ram,o+0x18,exact_div(ramf(ram,0x80253708u),ramf(ram,0x80253704u)));setw(ram,o+0x14,rnd()&0xffffffu);setw(ram,o+0x30,rnd()%4);
  setw(ram,o+0xfc,free_ring?0u:pick_actor(ram));}
}
static int native_rings(uint8_t *ram){
 nflstreet_player_draw_context x;memset(&x,0,sizeof x);x.span=span;x.memory=ram;
 const nflstreet_rings_context c={&x,R13};return nflstreet_rings_update(&c);}
/* Matrix edits: the 3x4 or the 4x4 rows of a slot. */
static void random_matrix(uint8_t *ram,uint32_t m,int perspective){
 for(unsigned i=0;i<16;++i){float v=rndf(-1.5f,1.5f);if(i%4==3&&i<12)v=rndf(-60.0f,60.0f);if(chance(7))v=chance(2)?0.0f:-0.0f;
  if(!perspective&&i>=12)v=i==15?1.0f:0.0f;setf(ram,m+4*i,v);}}
static void perturb_matrix(uint8_t *ram,uint32_t m){for(unsigned i=0;i<16;++i)if(chance(3))setf(ram,m+4*i,ramf(ram,m+4*i)*rndf(0.9f,1.1f)+(chance(4)?rndf(-3.0f,3.0f):0.0f));}
static uint32_t stack_top(uint8_t *ram,int depth_extra){
 uint32_t t=ram32(ram,R13-0x65f0u),i=ram_at(ram,R13-0x4ee8u,1)[0],s=t+12u*i;return ram32(ram,s+8)+0x40u*(ram32(ram,s+4)+depth_extra);}
static void prepare_stack(uint8_t *ram){
 const uint32_t t=ram32(ram,R13-0x65f0u);
 if(chance(4))setb(ram,R13-0x4ee8u,(uint8_t)(rnd()%4));
 const uint32_t s=t+12u*ram_at(ram,R13-0x4ee8u,1)[0];setw(ram,s+4,chance(2)?0u:rnd()%4);
 setw(ram,R13-0x65e8u,t+12u*(rnd()%4)); /* a stale selection: the routines select the viewport's stack themselves */
 for(unsigned d=0;d<6;++d)if(chance(2))random_matrix(ram,ram32(ram,s+8)+0x40u*d,0);}
static void prepare_slots(uint8_t *ram,unsigned index){
 if(!index)return;
 prepare_stack(ram);
 for(unsigned k=1;k<=2;++k){if(chance(2))random_matrix(ram,SLOTS+0x40u*k,1);else if(chance(2))perturb_matrix(ram,SLOTS+0x40u*k);}
 for(unsigned k=3;k<=6;++k)if(chance(2))random_matrix(ram,SLOTS+0x40u*k,1); /* stale results must be overwritten */
 if(chance(4))setf(ram,0x803a3958u,rndf(-1.0f,1.0f));if(chance(4))setf(ram,0x803a395cu,rndf(0.5f,1.5f));}
/* Slot 4 (the clip matrix): mostly as saved or slightly moved so tags stay on screen, sometimes arbitrary or with a w near zero. */
static void edit_clip(uint8_t *ram){
 const uint32_t m=SLOTS+0x100;const unsigned r=rnd()%8;
 if(r<3)return;
 if(r<6){for(unsigned i=0;i<16;++i)if(chance(4))setf(ram,m+4*i,ramf(ram,m+4*i)*rndf(0.97f,1.03f)+(i%4==3?rndf(-8.0f,8.0f):0.0f));return;}
 if(r==6){random_matrix(ram,m,1);return;}
 if(chance(2)){setf(ram,m+0x3c,rndf(-5.0f,5.0f));return;}
 /* a row equal to (or the negative of) the w row: that coordinate sits exactly on the clip boundary */
 {const unsigned row=rnd()%3;for(unsigned i=0;i<4;++i){const float w=ramf(ram,m+0x30+4*i);setf(ram,m+16*row+4*i,chance(2)?w:-w);}}}
static void prepare_tags(uint8_t *ram,unsigned index){
 if(!field_camera_slots(ram)){fprintf(stderr,"field camera slots failed\n");exit(5);}
 if(!index){setw(ram,R13-0x4eecu,NODE);return;}
 prepare_stack(ram);
 setw(ram,R13-0x4eecu,NODE);
 if(chance(3))setf(ram,NODE+0xd0,rndf(0.5f,2.5f));if(chance(3))setf(ram,R13-0x7244u,rndf(-2.0f,2.0f));
 edit_clip(ram);
 if(chance(3))perturb_matrix(ram,SLOTS+0x40);else if(chance(6))random_matrix(ram,SLOTS+0x40,1);
 {static const uint32_t k[]={0x80252b6cu,0x80252b70u,0x80252b74u,0x80252b78u,0x80252b7cu};edit_constants(ram,k,5,0.5f,400.0f);}
 for(unsigned i=0;i<ACTORS;++i){const uint32_t a=visual(ram,i),tag=ram32(ram,a+0x3fc);
  if(chance(2))for(unsigned w=1;w<4;++w)setf(ram,a+4*w,w==3?rndf(-1.0f,5.0f):ramf(ram,a+4*w)+rndf(-6.0f,6.0f));
  if(chance(2))setb(ram,tag+0x1c,(uint8_t)(chance(4)?0:chance(3)?rnd():1));
  if(chance(30))setw(ram,a+0x3fc,0);}}
static int native_slots(uint8_t *ram,nflstreet_view_context *c,nflstreet_player_draw_context *x);
/* The saved slots belong to the last camera node processed (the overlay). Rebuild the field camera's, as the frame does before the players
   draw: the field camera's view on the viewport stack (8017D16C, native), then the slot tail (checked above). Slots 1 and 2 are the
   same for all three nodes (full-screen 640 x 448 with one projection). */
static int field_camera_slots(uint8_t *ram){
 static nflstreet_gx_call calls[CALLS_MAX];trace t={calls,0};nflstreet_player_draw_context x;
 nflstreet_scenery_context sc=scenery_context(ram,&x,&t);nflstreet_hud_context h=hud_context(ram,&sc);nflstreet_render_state s;saved_state(ram,&s);
 if(!nflstreet_camera_view(&h,hud_cameras[0],&s))return 0;
 nflstreet_view_context c;nflstreet_player_draw_context nx;native_slots(ram,&c,&nx);return nflstreet_camera_slots(&c);}
static int native_slots(uint8_t *ram,nflstreet_view_context *c,nflstreet_player_draw_context *x){
 memset(x,0,sizeof *x);x->span=span;x->memory=ram;memset(c,0,sizeof *c);c->x=x;c->stacks=venue_stacks(ram);c->r13=R13;c->slots=SLOTS;return 1;}
static int same(const uint8_t *native,const char *what,unsigned index){
 const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&oracle[i]!=native[i]){
  fprintf(stderr,"%s case %u: RAM differs at %08x: original %02x native %02x\n",what,index,RAM_BASE+i,oracle[i],native[i]);return 0;}
 return 1;}
static int run_original(uint32_t pc,const uint32_t *args,unsigned n,uint32_t stop,const uint32_t *regs){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.gpr[1]=STACK_HI-0x200;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 for(unsigned i=0;i<n;++i)c.gpr[3+i]=args[i];
 if(regs)c.gpr[30]=regs[0];
 c.pc=pc;c.lr=0;
 for(unsigned step=0;c.pc;++step){
  if(step>20000000u){fprintf(stderr,"%08x did not return (pc %08x)\n",pc,c.pc);return 0;}
  if(stop&&c.pc==stop)return 1;
  if(c.pc==0x80164000u){c.pc=c.lr;continue;} /* the player draw that 800E20E4 ends with returns at once */
  if(c.pc>=0x80200000u&&c.pc<0x80210000u){fprintf(stderr,"unexpected SDK entry %08x (lr %08x)\n",c.pc,c.lr);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"%08x: exception at %08x (lr %08x)\n",pc,c.pc,c.lr);return 0;}}
 return 1;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);
 if(!pristine||!ram||!before||!oracle)return 3;
 uint32_t digest=2166136261u;unsigned found=0,started=0,released=0,moved=0,mirrors=0,controlled=0,slot_runs=0,projected=0,hidden=0,skipped=0;
 for(unsigned index=0;index<CASES;++index){ /* rings */
  memcpy(ram,pristine,RAM_BYTES);prepare_rings(ram,index);memcpy(before,ram,RAM_BYTES);
  if(!native_rings(ram)){fprintf(stderr,"native rings rejected case %u\n",index);return 5;}
  memcpy(oracle,before,RAM_BYTES);if(!run_original(0x800e4a24u,NULL,0,0,NULL)){fprintf(stderr,"rings case %u\n",index);return 6;}
  if(!same(ram,"rings",index))return 7;
  for(unsigned i=0;i<9;++i)found+=(ram32(ram,ram32(ram,A_TABLE+4*i)+0x20)&2)?1u:0u;
  for(unsigned j=0;j<5;++j){const uint32_t o=ram32(ram,POOL+4*j);started+=!ram32(before,o+0xfc)&&ram32(ram,o+0xfc)?1u:0u;released+=ram32(before,o+0xfc)&&!ram32(ram,o+0xfc)?1u:0u;}
  moved+=ram32(ram,ram32(ram,R13-0x71e0u)+4)!=ram32(before,ram32(ram,R13-0x71e0u)+4)?1u:0u;
  {const uint32_t rec=ram32(before,R13-0x7078u);mirrors+=rec&&ram_at(before,rec+0x39,1)[0]==1?1u:0u;}
  controlled+=ram_at(ram,R13-0x71dau,1)[0]!=ram_at(before,R13-0x71dau,1)[0]?1u:0u;
  digest=fnv(digest,ram,RAM_BYTES);}
 for(unsigned index=0;index<CASES;++index){ /* camera slots */
  memcpy(ram,pristine,RAM_BYTES);prepare_slots(ram,index);memcpy(before,ram,RAM_BYTES);
  nflstreet_player_draw_context x;nflstreet_view_context c;native_slots(ram,&c,&x);
  if(!nflstreet_camera_slots(&c)){fprintf(stderr,"native camera slots rejected case %u\n",index);return 5;}
  memcpy(oracle,before,RAM_BYTES);const uint32_t scratch=STACK_HI-0x200+0x88;
  if(!run_original(0x801dfbe8u,NULL,0,0x801dfc70u,&scratch)){fprintf(stderr,"camera slots case %u\n",index);return 6;}
  if(!same(ram,"camera slots",index))return 7;
  ++slot_runs;digest=fnv(digest,ram,RAM_BYTES);}
 for(unsigned index=0;index<CASES;++index){ /* name tags: each of the 14 players' step */
  memcpy(ram,pristine,RAM_BYTES);prepare_tags(ram,index);memcpy(before,ram,RAM_BYTES);
  nflstreet_player_draw_context x;nflstreet_view_context c;native_slots(ram,&c,&x);
  for(unsigned i=0;i<ACTORS;++i)if(!nflstreet_actor_name_tag(&c,visual(before,i))){fprintf(stderr,"native name tag %u rejected case %u\n",i,index);return 5;}
  memcpy(oracle,before,RAM_BYTES);
  for(unsigned i=0;i<ACTORS;++i){const uint32_t args[2]={visual(before,i),1};if(!run_original(0x800e20e4u,args,2,0,NULL)){fprintf(stderr,"name tag case %u actor %u\n",index,i);return 6;}
   const uint32_t tag=ram32(before,args[0]+0x3fc);
   if(!tag||!ram_at(before,tag+0x1c,1)[0])++skipped;else if(ram_at(ram,tag+0x1c,1)[0])++projected;else ++hidden;}
  if(!same(ram,"name tags",index))return 7;
  digest=fnv(digest,ram,RAM_BYTES);}
 /* Plausibility of the saved frame: the native projection with the field camera's rebuilt slots lands on the saved tags (stored by the frame
    before, so a few pixels off at most as the players moved). */
 float worst=0.0f;unsigned saved_tags=0;
 memcpy(ram,pristine,RAM_BYTES);if(!field_camera_slots(ram))return 5;setw(ram,R13-0x4eecu,NODE);
 {nflstreet_player_draw_context x;nflstreet_view_context c;native_slots(ram,&c,&x);
  for(unsigned i=0;i<ACTORS;++i){const uint32_t a=visual(pristine,i),tag=ram32(pristine,a+0x3fc);if(!ram_at(pristine,tag+0x1c,1)[0])continue;
   if(!nflstreet_actor_name_tag(&c,a)||!ram_at(ram,tag+0x1c,1)[0]){fprintf(stderr,"saved tag %u not projected\n",i);return 8;}
   const float dx=ramf(ram,tag+4)-ramf(pristine,tag+4),dy=ramf(ram,tag+8)-ramf(pristine,tag+8),d=dx*dx>dy*dy?(dx<0?-dx:dx):(dy<0?-dy:dy);
   if(d>worst)worst=d;++saved_tags;}}
 if(!saved_tags||worst>8.0f){fprintf(stderr,"saved tags: %u, worst offset %f\n",saved_tags,worst);return 8;}
 printf("Saved frame: %u shown tags re-projected within %.0f pixels of the stored positions.\n",saved_tags,worst+0.5f);
 printf("Original player markers: %u runs of 800E4A24 (%u tracked rings, %u pooled rings started, %u released, %u moves of the controlled ring, "
  "%u mirrored, %u controlled marks); %u runs of 801DFAB0's slot tail; %u x 14 runs of 800E20E4 (%u projected, %u hidden, %u without a shown tag) "
  "match all RAM; digest %08x.\n",CASES,found,started,released,moved,mirrors,controlled,slot_runs,CASES,projected,hidden,skipped,digest);
 return 0;}
