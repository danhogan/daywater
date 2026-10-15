/* Native per-actor strips (80040C84 -> 801649C4 -> 801590F4) and particle
   emitters (800CF284 -> 80158D24, 80158798) against the original on a saved
   context. Each case edits a fresh copy of the saved RAM, runs the two scene
   node entries natively in the frame's order (15, then 16), then runs the
   same two entries in the preserved original module from the same RAM.
   Every routine executes in the original except the GX SDK entry points
   (recorded and compared call by call, gx_oracle.h) and the OS cache
   routines 801F0B00..801F1000 (no RAM effect); the immediate vertex data
   written to the GX pipe (0xCC008000) is compared write by write. All RAM
   outside the oracle stack and the render-state globals must be equal.
   Case 0 is the saved frame. The others edit: the stack top; the gates
   (r13-72C0, r13-7304, actor count, actor flags 0x10000 and 8, record
   flag 1); the strip objects (mode byte, scale, colors, alpha, ring
   positions and head, counts, edge selectors and points, the history
   table); the emitters (enable, count, textured, blend kind, size source,
   fade flag and times, colors, alpha, color scale, spin, the sprite sheet
   record, particle positions, sizes, ages, frames, spins); the shader
   pointers and the render state's shader and blend. */
#include <windows.h>
#include <math.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "effects_draw.h"
#include "gx_oracle.h"
#define CASES 401u
typedef struct {nflstreet_player_draw_context x;nflstreet_scenery_context scene;nflstreet_matrix_stacks stacks;
 nflstreet_fans_context fans;nflstreet_strips_context strips;nflstreet_particles_context particles;} fixture;
static void effects_fixture(uint8_t *ram,fixture *f,trace *t){
 memset(f,0,sizeof *f);f->scene=scenery_context(ram,&f->x,t);f->stacks=venue_stacks(ram);
 f->fans.scene=&f->scene;f->fans.stacks=&f->stacks;f->fans.byte_scale=ramf(ram,0x80260b88u);f->fans.depth[0]=ramf(ram,0x80260b8cu);f->fans.depth[1]=ramf(ram,0x80260b90u);
 nflstreet_strips_context *s=&f->strips;s->draw=&f->x;s->stacks=&f->stacks;s->actors=ram32(ram,R13-0x7bbcu);s->actor_count=R13-0x7bb8u;
 s->gate=R13-0x72c0u;s->scaled_gate=R13-0x7238u;s->history=0x80270ca4u;s->byte_scale=ramf(ram,0x80260438u);
 nflstreet_particles_context *p=&f->particles;p->draw=&f->x;p->stacks=&f->stacks;p->enable=R13-0x7304u;p->count=R13-0x7310u;p->list=ram32(ram,R13-0x730cu);
 p->gate_72fc=R13-0x72fcu;p->gate_58bc=R13-0x58bcu;p->shader_flat=ram32(ram,R13-0x68f8u);p->shader_textured=ram32(ram,R13-0x68fcu);p->corners=0x802898c0u;
 p->color_unit=ramf(ram,0x802603d8u);p->zero=ramf(ram,0x802603dcu);p->one=ramf(ram,0x802603e0u);p->angle_scale=ramf(ram,0x802603e4u);
 p->byte_scale=ramf(ram,0x802603e8u);p->cell[0]=ramf(ram,0x80261218u);p->cell[1]=ramf(ram,0x8026121cu);p->fade_one=ramf(ram,0x8026323cu);}
static float fword_of(const uint8_t *p){uint32_t v=rd32(p);float f;memcpy(&f,&v,4);return f;}
static void put_floats(uint8_t *p,unsigned n,float lo,float hi){for(unsigned i=0;i<n;++i)wr32(p+4*i,fbits(rndf(lo,hi)));}
static void put16(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
/* A float that often lands exactly on a boundary of its use. */
static float pick(const float *list,unsigned n,float lo,float hi){return rnd()%3==0?list[rnd()%n]:rndf(lo,hi);}
static unsigned actor_list(uint8_t *ram,uint32_t *out){
 unsigned n=ram_at(ram,R13-0x7bb8u,2)[0]<<8|ram_at(ram,R13-0x7bb8u,2)[1];uint32_t l=ram32(ram,R13-0x7bbcu);if(n>14)n=14;
 for(unsigned i=0;i<n;++i)out[i]=ram32(ram,l+4*i);return n;}
static void edit_fans(uint8_t *ram,uint32_t actor){
 uint8_t *o=ram_at(ram,actor+0x109cu,0x2a2);if(!o)return;
 if(rnd()%2)o[0x2a1]=(uint8_t)(rnd()%5!=0);
 if(rnd()%2)o[0x2a0]=(uint8_t)(rnd()%5);
 if(rnd()%2)put_floats(o,4,-0.2f,1.3f);
 for(unsigned k=0;k<4;++k){uint8_t *r=o+0x60+0x90*k;
  if(rnd()%2)put_floats(r,21,-40.0f,40.0f);
  if(rnd()%2)r[0x8c]=(uint8_t)(rnd()%8);}}
static void edit_strips(uint8_t *ram,uint32_t actor){
 uint8_t *s=ram_at(ram,actor+0x1368u,0x250);if(!s)return;
 if(rnd()%2)s[0x23c]=(uint8_t)(rnd()%3==0?1:rnd()%2);
 if(rnd()%2)wr32(s+0x230,fbits(rndf(0.1f,1.6f)));
 if(rnd()%2)put_floats(s+0x240,3,-0.2f,1.3f);
 if(rnd()%2)wr32(s+0x24c,fbits(rndf(-0.1f,1.3f)));
 for(unsigned i=0;i<4;++i){uint8_t *q=s+0x8c*i;
  if(rnd()%2)put_floats(q,24,-60.0f,60.0f);
  if(rnd()%2)wr32(q+0x78,(uint32_t)(int32_t)((int32_t)(rnd()%18)-5));
  if(rnd()%4==0)wr32(q+0x78,7); /* the wrap */
  if(rnd()%8==0)wr32(q+0x78,(uint32_t)-(int32_t)(8+rnd()%17)); /* far negative heads */
  if(rnd()%2)wr32(q+0x7c,rnd()%12==0?(uint32_t)(rnd()%2):rnd()%9);
  if(rnd()%2)wr32(q+0x80,rnd()%3);}}
static unsigned valid_run(uint8_t *ram,uint32_t array){
 unsigned n=0;while(n<64){uint32_t v=ram32(ram,array+4*n);if(v<0x80000000u||v>=0x81800000u)break;++n;}return n;}
static void edit_emitter(uint8_t *ram,uint32_t e){
 uint8_t *x=ram_at(ram,e,0x3d4);if(!x)return;uint32_t rec=rd32(x+0x2ac);const unsigned cap=valid_run(ram,rd32(x+0x3c0));
 if(rnd()%12==0)x[0x2b4]^=1;
 if(rnd()%2)wr32(x+0x3b8,rnd()%8==0?0:(uint32_t)(cap?rnd()%(cap+1):0));
 if(rnd()%5==0)x[0x45]^=1;
 if(rnd()%2)wr32(x+0x60,rnd()%5);
 if(rnd()%3==0)x[0x41]^=1;
 if(rnd()%3==0)x[0x42]^=1;
 if(rnd()%6==0){x[0x3d]=1;wr32(x+0x8c,0);} /* the gradient test with no gradient */
 if(rnd()%2)put_floats(x+0x7c,3,-0.2f,1.4f);
 if(rnd()%2)wr32(x+0x88,fbits(rndf(-0.1f,1.3f)));
 {static const float times[]={0.0f,0.25f,0.5f,1.0f};
  if(rnd()%2)wr32(x+0xec,fbits(pick(times,4,0.0f,0.9f)));if(rnd()%2)wr32(x+0xe8,fbits(pick(times,4,0.0f,1.2f)));}
 if(rnd()%2)x[0x35b]=(uint8_t)rnd();
 if(rnd()%3==0)put16(x+0x48,rnd()%4==0?0:rnd()%40);
 if(rec){uint8_t *r=ram_at(ram,rec,0x78);
  if(r&&rnd()%2){put16(r+0x4a,rnd()%3==0?rnd()%2:2+rnd()%6);r[0x48]=(uint8_t)(rnd()%2);put16(r+0x4c,1+rnd()%4);put16(r+0x4e,1+rnd()%4);
   put_floats(r+0x70,2,0.05f,0.6f);}}
 uint32_t array=rd32(x+0x3c0);
 for(unsigned i=0;i<cap;++i){uint8_t *p=ram_at(ram,ram32(ram,array+4*i),0x54);if(!p||rnd()%2)continue;
  put_floats(p+0xc,3,-60.0f,60.0f);
  if(rnd()%2)wr32(p+0x30,fbits(rndf(-0.5f,3.0f)));if(rnd()%2)wr32(p+0x4c,fbits(rndf(-0.5f,3.0f)));
  if(rnd()%2){static const float ages[]={0.0f,0.25f,0.5f,1.0f};wr32(p+0x48,fbits(pick(ages,4,-0.1f,1.3f)));}
  if(rnd()%2)wr32(p+0x50,fbits(rndf(-8.0f,8.0f)));
  if(rnd()%2)put16(p+0x3a,rnd()%60);
  if(rnd()%2)wr32(p+0x40,rnd()%400);}}
static void prepare(uint8_t *ram,unsigned index){
 nflstreet_matrix_stacks k=venue_stacks(ram);
 wr32(ram_at(ram,R13-0x65e8u,4),k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0]);
 if(!index)return;
 uint32_t actors[14];unsigned n=actor_list(ram,actors);
 if(rnd()%3==0){uint32_t s=ram32(ram,R13-0x65e8u),top=ram32(ram,s+8)+0x40u*ram32(ram,s+4);
  for(unsigned i=0;i<12;++i)if(rnd()%3==0)wr32(ram_at(ram,top+4*i,4),fbits(ramf(ram,top+4*i)+rndf(-0.3f,0.3f)));}
 if(rnd()%10==0)ram_at(ram,R13-0x72c0u,1)[0]=0;
 if(rnd()%10==0)ram_at(ram,R13-0x7304u,1)[0]=0;
 if(rnd()%6==0)ram_at(ram,R13-0x72fcu,1)[0]=(uint8_t)(rnd()%2?0:2);
 if(rnd()%5==0){uint8_t *c=ram_at(ram,R13-0x7bb8u,2);put16(c,rnd()%(n+1));}
 for(unsigned i=0;i<n;++i){uint8_t *a=ram_at(ram,actors[i],0x20);if(!a)continue;
  if(rnd()%3==0)wr32(a+0x14,rd32(a+0x14)^0x10000u);if(rnd()%5==0)wr32(a+0x14,rd32(a+0x14)^8u);
  uint32_t rec=rd32(a+0x4a8);if(rnd()%6==0&&rec)wr32(ram_at(ram,rec+4,4),rd32(ram_at(ram,rec+4,4))^(rnd()%2?1u:1u<<(1+rnd()%5)));
  if(rd32(a+0x14)&0x10000u||rnd()%2)edit_strips(ram,actors[i]);
  if(rnd()%2)edit_fans(ram,actors[i]);}
 if(rnd()%3==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);
 if(rnd()%4==0)put_floats(ram_at(ram,0x80270ca4u,0x80),32,-0.2f,1.3f);
 {uint32_t list=ram32(ram,R13-0x730cu);unsigned cnt=ram_at(ram,R13-0x7310u,2)[0]<<8|ram_at(ram,R13-0x7310u,2)[1];
  for(unsigned i=0;i<cnt&&i<32;++i)if(rnd()%2)edit_emitter(ram,ram32(ram,list+4*i));
  if(rnd()%6==0){uint8_t *c=ram_at(ram,R13-0x7310u,2);put16(c,rnd()%(cnt+1));}}
 if(rnd()%8==0){uint32_t a=ram32(ram,R13-0x68f8u),b=ram32(ram,R13-0x68fcu);wr32(ram_at(ram,R13-0x68f8u,4),b);wr32(ram_at(ram,R13-0x68fcu,4),a);}
 if(rnd()%2){const uint32_t sh[]={0,ram32(ram,R13-0x68f8u),ram32(ram,R13-0x68fcu),0x806ef5f8u,0x806ef5d4u};wr32(ram_at(ram,STATE+0x18,4),sh[rnd()%5]);}
 if(rnd()%3==0)wr32(ram_at(ram,STATE+0x1c,4),rnd()%8);}
static int native_case(uint8_t *ram,nflstreet_render_state *s,trace *t){
 static fixture f;effects_fixture(ram,&f,t);saved_state(ram,s);
 if(!nflstreet_fans_draw(&f.fans,s)){fprintf(stderr,"native fans rejected\n");return 0;}
 if(!nflstreet_strips_draw(&f.strips,s)){fprintf(stderr,"native strips rejected\n");return 0;}
 if(!nflstreet_particles_draw(&f.particles,s)){fprintf(stderr,"native particles rejected\n");return 0;}
 return 1;}
/* Inputs whose original paths are not modeled must be refused, not drawn. */
static unsigned rejected_cases(uint8_t *pristine,uint8_t *ram){
 unsigned refused=0;
 for(unsigned kind=0;kind<6;++kind){
  memcpy(ram,pristine,RAM_BYTES);prepare(ram,0);uint32_t actors[14];unsigned n=actor_list(ram,actors);
  uint32_t list=ram32(ram,R13-0x730cu);uint32_t e=ram32(ram,list);
  switch(kind){
  case 0:ram_at(ram,R13-0x7238u,1)[0]=1;for(unsigned i=0;i<n;++i)wr32(ram_at(ram,actors[i]+0x14,4),rd32(ram_at(ram,actors[i]+0x14,4))|8u);
   {uint32_t rec=ram32(ram,actors[0]+0x4a8);wr32(ram_at(ram,rec+4,4),0);}break;
  case 1:ram_at(ram,e+0x3d,1)[0]=1;wr32(ram_at(ram,e+0x8c,4),0x80d3a30cu);break;
  case 2:ram_at(ram,R13-0x72fcu,1)[0]=1;break;
  case 3:wr32(ram_at(ram,R13-0x58bcu,4),0x80000000u);break;
  case 4:{uint32_t rec=ram32(ram,e+0x2ac);uint8_t *r=ram_at(ram,rec,0x78);put16(r+0x4a,4);put16(r+0x4e,0);}break;
  default:ram_at(ram,e+0x45,1)[0]=1;wr32(ram_at(ram,R13-0x68fcu,4),0);}
  trace t={malloc(sizeof(nflstreet_gx_call)*CALLS_MAX),0};nflstreet_render_state s;static fixture f;
  effects_fixture(ram,&f,&t);saved_state(ram,&s);
  int ok=nflstreet_strips_draw(&f.strips,&s)&&nflstreet_particles_draw(&f.particles,&s);
  free(t.calls);if(ok){fprintf(stderr,"unmodeled input %u was drawn\n",kind);return 0xffffu;}++refused;}
 return refused;}
int main(int argc,char **argv){
 if(argc!=3)return 2;if(!gx_oracle_open(argv[2]))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);
 static nflstreet_gx_call calls[CALLS_MAX];
 if(!pristine||!ram||!before)return 3;
 unsigned total_calls=0,writes=0,strips=0,quads=0,fans=0,runs=0;uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);prepare(ram,index);memcpy(before,ram,RAM_BYTES);
  trace t={calls,0};nflstreet_render_state s;
  if(!native_case(ram,&s,&t)){fprintf(stderr,"case %u\n",index);return 5;}
  memcpy(oracle_ram,before,RAM_BYTES);unsigned n=0,k=0;const uint32_t none[6]={0};
  {trace rest={t.calls,t.count};if(!gx_oracle_run(0x80040c60u,none,&rest,NULL,NULL,&k)){fprintf(stderr,"case %u entry 11\n",index);return 6;}n+=k;++runs;}
  {trace rest={t.calls+n,t.count-n};if(!gx_oracle_run(0x80040c84u,none,&rest,NULL,NULL,&k)){fprintf(stderr,"case %u entry 15\n",index);return 6;}n+=k;++runs;}
  {trace rest={t.calls+n,t.count-n};if(!gx_oracle_run(0x800cf284u,none,&rest,NULL,NULL,&k)){fprintf(stderr,"case %u entry 16\n",index);return 6;}n+=k;++runs;}
  if(n!=t.count){fprintf(stderr,"case %u: original issued %u GX calls, native %u\n",index,n,t.count);return 7;}
  if(!gx_oracle_same_ram(ram,&s)){fprintf(stderr,"original effects mismatch case %u\n",index);return 7;}
  total_calls+=n;if(!index)printf("Saved frame: %u GX calls (%u pipe writes)\n",n,(unsigned)({unsigned w=0;for(unsigned i=0;i<n;++i)w+=calls[i].op==NFLSTREET_GX_WRITE;w;}));
  for(unsigned i=0;i<n;++i){if(calls[i].op==NFLSTREET_GX_WRITE)++writes;
   if(calls[i].op==NFLSTREET_GX_BEGIN){if(calls[i].arg[0]==0x80)quads+=calls[i].arg[2]>>2;else if(calls[i].arg[0]==0xa0)++fans;else if(calls[i].arg[1]==0x98)++strips;}}
  digest=fnv(digest,ram,RAM_BYTES);digest=fnv(digest,calls,sizeof *calls*n);}
 unsigned refused=rejected_cases(pristine,ram);if(refused!=6)return 8;
 printf("Refused unmodeled inputs: %u of 6.\n",refused);
 printf("Original effects: %u runs of 80040C60, 80040C84 and 800CF284 over %u cases match %u GX calls and %u pipe writes (%u fans, %u strips, %u particle quads), all RAM; digest %08x.\n",
  runs,CASES,total_calls,writes,fans,strips,quads,digest);
 return 0;}
