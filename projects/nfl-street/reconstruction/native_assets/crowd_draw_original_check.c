/* Native crowd draw (800D2F20 -> 80168234/80168E08) against the original on
   a saved context. Each case prepares a fresh copy of the saved RAM as the
   frame does before the venue draws (the viewport's stack selected, the
   palette pool emptied) and runs the 45 entries of the scene node in order
   (EARS props, garbage cans, the ten spectators and their copies), natively
   and then in the preserved original module from the same RAM and locked
   cache. Every routine executes in the original except: the GX SDK entry
   points (recorded and compared call by call, gx_oracle.h); the OS cache
   routines 801F0B00..801F1000 (no RAM effect), with the locked-cache DMA
   801F0DD4 served by copying whole 32-byte blocks from the oracle's locked
   cache. The paired-single quantization registers hold the game's values
   (8015668C). All RAM outside the oracle stack, the render-state globals
   and the locked cache must be equal. Case 0 is the saved frame; the others
   edit placements, quaternions, headings, path flags (+E8, +12C, the first
   chunk's CPU-skinning bit), selectors, light sources, poses, the hide-test
   inputs, play-held flags, the scratch descriptor and locked-cache flag,
   the palette pool, the vertex scratch ring (wrapping), vertex positions and
   normals, the normal and palette scales, the stack top, the blend state and
   the enable byte. */
#include <windows.h>
#include <math.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "actor_palettes.h"
#include "crowd_draw.h"
#include "gx_oracle.h"
#define CASES 401u
#define OBJECTS 45u
#define FIRST_OBJECT 0x80c4e094u /* the scene node's 800D2F20 entries, 0x1DC apart */
static uint32_t object_at(unsigned i){return FIRST_OBJECT+0x1dcu*i;}
static uint8_t locked[NFLSTREET_LOCKED_CACHE_BYTES];
static int crowd_present(uint8_t *ram){
 unsigned spectators=0;
 for(unsigned i=0;i<OBJECTS;++i){const uint8_t *name=ram_at(ram,object_at(i)+0x150,32);if(!name||memcmp(name,"EARS_",5))return 0;
  if(!memcmp(name,"EARS_SPECTATOR_",15))++spectators;}
 return spectators==10;}
typedef struct {nflstreet_player_draw_context x;nflstreet_scenery_context scene;nflstreet_actor_palette_context palettes;nflstreet_crowd_context crowd;} fixture;
static void crowd_fixture(uint8_t *ram,fixture *f,trace *t){
 memset(f,0,sizeof *f);f->scene=scenery_context(ram,&f->x,t);
 nflstreet_actor_palette_context *p=&f->palettes;p->x=&f->x;p->stacks=venue_stacks(ram);p->unit[0]=ramf(ram,0x803a3958u);p->unit[1]=ramf(ram,0x803a395cu);
 p->pool=R13-0x5680u;p->pool_index=R13-0x5678u;p->pool_used=R13-0x5670u;p->cache_on=R13-0x6200u;
 nflstreet_crowd_context *c=&f->crowd;c->scene=&f->scene;c->palettes=p;c->enable=R13-0x72a4u;c->view_slot=0x80331ec0u+0xc0u;c->cull_box=0x8028e2b4u;
 c->game_mode=0x802a301cu;c->sides=ram32(ram,R13-0x5ba4u);c->hide_limit=0x802a3050u;c->scratch=R13-0x6430u;
 c->hide_sign[0]=ramf(ram,0x80260f2cu);c->hide_sign[1]=ramf(ram,0x80260f30u);c->light_default[0]=ramf(ram,0x80260f04u);c->light_default[1]=ramf(ram,0x80260f54u);
 c->palette_scale[0]=ramf(ram,0x80260f1cu);c->palette_scale[1]=ramf(ram,0x80260f58u);c->normal_scale=ramf(ram,0x8026381cu);c->quat_one=ramf(ram,0x802634d8u);}
/* The 12 objects with the model hook, and their copies. */
static unsigned models(uint8_t *ram,uint32_t *out){
 unsigned n=0;for(unsigned i=0;i<OBJECTS;++i)if(ram32(ram,object_at(i)+0x14)==0x80168234u)out[n++]=object_at(i);return n;}
static float fword_of(const uint8_t *p){uint32_t v=rd32(p);float f;memcpy(&f,&v,4);return f;}
static void put_floats(uint8_t *p,unsigned n,float lo,float hi){for(unsigned i=0;i<n;++i)wr32(p+4*i,fbits(rndf(lo,hi)));}
static void edit_quat(uint8_t *q){
 if(rnd()%4==0){put_floats(q,4,-1.2f,1.2f);return;}
 float v[4],s=0.0f;for(unsigned i=0;i<4;++i){v[i]=rndf(-1.0f,1.0f);s+=v[i]*v[i];}s=sqrtf(s);if(s<0.01f)s=1.0f;
 for(unsigned i=0;i<4;++i)wr32(q+4*i,fbits(v[i]/s));}
/* Vertex positions and normals of a model's first chunk. */
static void edit_vertices(uint8_t *ram,uint32_t view){
 uint32_t table=ram32(ram,view+8),arrays=ram32(ram,table+8),chunks=ram32(ram,table),total=ram32(ram,table+4);
 for(uint32_t c=0;c<total&&c<8;++c){const uint8_t *ch=ram_at(ram,chunks+0x18*c,0x18);if(!ch)return;
  for(uint32_t i=0;i<ch[0x10];++i){const uint8_t *r=ram_at(ram,arrays+0xc*((uint32_t)(ch[0xe]<<8|ch[0xf])+i),0xc);if(!r)return;
   uint32_t count=(uint32_t)(r[4]<<8|r[5]),src=rd32(r);if(!count)continue;
   if(r[6]==9&&r[9]==3)for(unsigned k=0;k<8;++k){uint8_t *v=ram_at(ram,src+2*(rnd()%(count*3)),2);if(v){uint16_t x=(uint16_t)(rnd()%5==0?(rnd()%2?0x7fff:0x8000):rnd());v[0]=(uint8_t)(x>>8);v[1]=(uint8_t)x;}}
   if(r[6]==10&&r[9]==1)for(unsigned k=0;k<8;++k){uint8_t *v=ram_at(ram,src+rnd()%(count*3),1);if(v)*v=(uint8_t)(rnd()%5==0?(rnd()%2?0x7f:0x80):rnd());}}}}
static void prepare(uint8_t *ram,unsigned index){
 nflstreet_matrix_stacks k=venue_stacks(ram);
 wr32(ram_at(ram,R13-0x65e8u,4),k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0]);wr32(ram_at(ram,R13-0x5670u,4),0);
 for(unsigned i=0;i<sizeof locked;++i)locked[i]=(uint8_t)rnd();
 if(!index)return;
 uint32_t objs[OBJECTS];unsigned n=models(ram,objs);
 if(rnd()%25==0)ram_at(ram,R13-0x72a4u,1)[0]=0;
 if(rnd()%3==0){uint32_t s=ram32(ram,R13-0x65e8u),top=ram32(ram,s+8)+0x40u*ram32(ram,s+4);
  for(unsigned i=0;i<12;++i)if(rnd()%3==0)wr32(ram_at(ram,top+4*i,4),fbits(ramf(ram,top+4*i)+rndf(-0.3f,0.3f)));}
 for(unsigned i=0;i<n;++i){uint8_t *o=ram_at(ram,objs[i],0x1dc);uint32_t m=rd32(o+0x128),view=ram32(ram,m+0x18);
  if(rnd()%3==0){for(unsigned w=0;w<3;++w)wr32(o+4+4*w,fbits(fword_of(o+4+4*w)+rndf(-3.0f,3.0f)));}
  if(rnd()%10==0)put_floats(o+4,3,-120.0f,120.0f); /* often outside the frustum */
  if(rnd()%3==0)edit_quat(o+0x18);if(rnd()%4==0)put_floats(o+0xb4,3,-2.0f,2.0f);if(rnd()%3==0)wr32(o+0xcc,rnd()&0xffffffu);
  if(rnd()%10==0)wr32(o+0xe8,rd32(o+0xe8)^1u);
  /* skinning needs the view's override array (801C6BF4 allocating one is not modeled) */
  if(rnd()%10==0)wr32(o+0x12c,rd32(o+0x12c)||!view||!ram32(ram,view+0x1c)?0:0x802bdbd4u);
  if(view&&rnd()%8==0){uint8_t *f=ram_at(ram,ram32(ram,ram32(ram,view+8))+0x11,1);if(f)*f^=8;}
  if(rnd()%4==0)wr32(o+0x138,rnd()%3);
  if(rnd()%6==0)wr32(o+0x144,0);else if(rd32(o+0x144)&&rnd()%4==0)put_floats(ram_at(ram,rd32(o+0x144)+0xc,12),3,0.2f,1.6f);
  if(rnd()%2&&rd32(o+0xdc)){for(unsigned b=0;b<40*3;++b)if(rnd()%3==0){uint8_t *q=ram_at(ram,rd32(o+0xdc)+2*b,2);if(q){uint16_t v=(uint16_t)rnd();q[0]=(uint8_t)(v>>8);q[1]=(uint8_t)v;}}}
  if(rnd()%4==0)o[0x134]=(uint8_t)(rnd()%2);
  if(m&&rnd()%6==0)wr32(ram_at(ram,m+0x100,4),0); /* palette built in place */
  if(view&&rnd()%3==0)edit_vertices(ram,view);
  /* without copies, the last spectator's own stack writes stay visible */
  if(rnd()%(i+1==n?4:16)==0)wr32(o+0x1d8,0);
  uint32_t copy=rd32(o+0x1d8);
  for(unsigned guard=0;copy&&guard<4;++guard){uint8_t *cp=ram_at(ram,copy,0x60);if(!cp)break;
   if(rnd()%3==0)for(unsigned w=0;w<3;++w)wr32(cp+0x30+4*w,fbits(fword_of(cp+0x30+4*w)+rndf(-3.0f,3.0f)));
   if(rnd()%12==0)put_floats(cp+0x30,3,-120.0f,120.0f);if(rnd()%3==0)edit_quat(cp+0x3c);
   if(rnd()%5==0)wr32(cp+0x58,0);copy=rd32(cp+0x5c);}}
 /* hide test: game mode, mode word, reference player y, limit */
 if(rnd()%2)wr32(ram_at(ram,0x802a301cu,4),(uint32_t[]){4,7,1,7}[rnd()%4]);
 if(rnd()%2)wr32(ram_at(ram,ram32(ram,R13-0x7cf8u),4),(uint32_t[]){2,3,4,7,0,5}[rnd()%6]);
 {uint32_t side=ram32(ram,ram32(ram,R13-0x5ba4u)+8);
  if(side&&rnd()%2)wr32(ram_at(ram,side+8,4),fbits((float[]){rndf(-70.0f,70.0f),55.0f,-55.0f,0.0f,60.0f}[rnd()%5]));
  if(rnd()%3==0)wr32(ram_at(ram,0x802a3050u,4),fbits(rndf(30.0f,60.0f)));
  /* limits exactly at the reference's or a spectator's |y| */
  if(side&&rnd()%6==0)wr32(ram_at(ram,0x802a3050u,4),fbits(fabsf(ramf(ram,side+8))));
  if(n&&rnd()%6==0)wr32(ram_at(ram,0x802a3050u,4),fbits(fabsf(ramf(ram,objs[rnd()%n]+8))));
  /* the sign thresholds exactly at a product (80260F2C model, 80260F30 copy) */
  if(side&&n&&rnd()%5==0){uint32_t o=objs[rnd()%n],copy=ram32(ram,o+0x1d8);float ref=ramf(ram,o+8)<0.0f?-55.0f:55.0f;
   wr32(ram_at(ram,side+8,4),fbits(ref));
   wr32(ram_at(ram,0x80260f2cu,4),fbits(ref*ramf(ram,o+8)));if(copy)wr32(ram_at(ram,0x80260f30u,4),fbits(ref*ramf(ram,copy+0x34)));}}
 if(rnd()%3==0){uint32_t g=ram32(ram,R13-0x7f40u);if(g){wr32(ram_at(ram,g+0xb64,4),(uint32_t[]){0,0x80,0x800,0x400000,0x800000,0x1}[rnd()%6]);if(rnd()%5==0)wr32(ram_at(ram,g+0xb60,4),4);}}
 if(rnd()%4==0)ram_at(ram,R13-0x6200u,1)[0]=0;
 if(rnd()%3==0)wr32(ram_at(ram,R13-0x5670u,4),rnd()%300);
 if(rnd()%3==0){uint32_t end=ram32(ram,R13-0x6428u);wr32(ram_at(ram,R13-0x642cu,4),end-(rnd()%3==0?0x40u:(rnd()%0x3000u)));}
 /* the ring's remaining space exactly one first allocation (or one more) */
 if(n&&rnd()%5==0){uint32_t view=ram32(ram,ram32(ram,objs[0]+0x128)+0x18),table=ram32(ram,view+8),arrays=ram32(ram,table+8);const uint8_t *ch=ram_at(ram,ram32(ram,table),0x18);
  for(uint32_t i=0;ch&&i<ch[0x10];++i){const uint8_t *r=ram_at(ram,arrays+0xc*((uint32_t)(ch[0xe]<<8|ch[0xf])+i),0xc);
   if(r&&((r[6]==9&&r[9]==3)||(r[6]==10&&r[9]==1))){uint32_t bytes=(uint32_t)(r[4]<<8|r[5])*(r[6]==9?12u:8u);
    wr32(ram_at(ram,R13-0x642cu,4),ram32(ram,R13-0x6428u)-bytes-(uint32_t)(rnd()%2));break;}}}
 if(rnd()%4==0)wr32(ram_at(ram,0x8026381cu,4),fbits((float[]){32768.0f,90000.0f,-50000.0f,1000.0f,300000.0f}[rnd()%5]));
 if(rnd()%5==0)wr32(ram_at(ram,0x80260f1cu,4),fbits(rndf(1e-5f,1e-4f)));if(rnd()%5==0)wr32(ram_at(ram,0x80260f58u,4),fbits(rndf(1e-5f,1e-4f)));
 if(rnd()%5==0)wr32(ram_at(ram,STATE+0x1c,4),rnd()%7);}
static int native_case(uint8_t *ram,nflstreet_render_state *s,trace *t,uint8_t *cache){
 static fixture f;crowd_fixture(ram,&f,t);memcpy(f.palettes.locked,cache,sizeof f.palettes.locked);saved_state(ram,s);
 for(unsigned i=0;i<OBJECTS;++i)if(!nflstreet_crowd_draw(&f.crowd,object_at(i),s)){uint32_t o=object_at(i),m=ram32(ram,o+0x128),v=ram32(ram,m+0x18);
  fprintf(stderr,"native object %u (%08x) rejected: +E8 %08x +12C %08x +13C %u view %08x +1C %08x chunk flags %02x\n",i,o,ram32(ram,o+0xe8),ram32(ram,o+0x12c),
   ram_at(ram,o+0x13c,1)[0],v,v?ram32(ram,v+0x1c):0,v?ram_at(ram,ram32(ram,ram32(ram,v+8))+0x11,1)[0]:0);return 0;}
 memcpy(cache,f.palettes.locked,sizeof f.palettes.locked);return 1;}
/* The oracle with the locked cache and the GX pipe as external memory. */
static void *external(CPUState *c,uint32_t a,uint32_t n){(void)c;return a>=0xe0000000u&&(uint64_t)a-0xe0000000u+n<=sizeof locked?locked+(a-0xe0000000u):NULL;}
static uint64_t read_external(CPUState *c,uint32_t a,uint8_t n){const uint8_t *p=external(c,a,n);uint64_t v=0;if(!p)return 0;for(unsigned i=0;i<n;++i)v=v<<8|p[i];return v;}
static void write_external(CPUState *c,uint32_t a,uint64_t v,uint8_t n){
 if(a==0xcc008000u){pipe_write(c,a,v,n);return;}
 uint8_t *p=external(c,a,n);if(!p){fprintf(stderr,"unexpected external write %08x\n",a);pipe.failed=1;return;}for(unsigned i=n;i;--i){p[i-1]=(uint8_t)v;v>>=8;}}
static void write_external32(CPUState *c,uint32_t a,uint32_t v,uint8_t region){(void)region;write_external(c,a,v,4);}
static int run_original(uint32_t pc,const uint32_t args[2],const trace *t,unsigned *calls){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle_ram;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 static const uint32_t gqr[8]={0,0,0x00040004u,0x00050005u,0x00060006u,0x00070007u,0x0f070007u,0x00070007u}; /* 8015668C */
 for(unsigned i=0;i<8;++i)c.gqr[i]=gqr[i];
 c.external_pointer=external;c.external_read=read_external;c.external_write=write_external;c.external_write32=write_external32;
 pipe.t=t;pipe.n=0;pipe.failed=0;c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=args[0];c.gpr[4]=args[1];c.pc=pc;
 for(unsigned step=0;c.pc;++step){
  if(pipe.failed)return 0;
  if(step>40000000u){fprintf(stderr,"%08x did not return (pc %08x)\n",pc,c.pc);return 0;}
  const uint32_t at=c.pc;
  if(at==0x801f0dd4u){const uint32_t n=(c.gpr[5]+0x1fu)&~0x1fu;uint8_t *to=ram_at(oracle_ram,c.gpr[3],n);const uint8_t *from=external(&c,c.gpr[4],n);
   if(n&&(!to||!from)){fprintf(stderr,"DMA %08x <- %08x (%u) outside memory\n",c.gpr[3],c.gpr[4],n);return 0;}if(n)memmove(to,from,n);c.pc=c.lr;continue;}
  if(at>=0x801f0b00u&&at<0x801f1000u){c.pc=c.lr;continue;}
  if(at>=0x80200000u&&at<0x80210000u&&!sdk_executes(at)){
   const sdk_entry *e=NULL;for(unsigned i=0;i<sizeof sdk/sizeof *sdk;++i)if(sdk[i].pc==at)e=sdk+i;
   nflstreet_gx_call g;
   if(!e||!capture(&c,e,&g)){fprintf(stderr,"unexpected SDK entry %08x (lr %08x)\n",at,c.lr);return 0;}
   if(pipe.n>=t->count||memcmp(&g,t->calls+pipe.n,sizeof g)){fprintf(stderr,"GX call %u of %u differs (lr %08x)\n",pipe.n,t->count,c.lr);
    print_call("original",&g);if(pipe.n<t->count)print_call("native",t->calls+pipe.n);return 0;}
   ++pipe.n;c.pc=c.lr;continue;}
  c.downcount=-1000000000;gx_module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"%08x: exception at %08x (lr %08x)\n",pc,c.pc,c.lr);return 0;}}
 if(pipe.failed)return 0;*calls=pipe.n;return 1;}
int main(int argc,char **argv){
 if(argc!=3)return 2;if(!gx_oracle_open(argv[2]))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);
 static nflstreet_gx_call calls[CALLS_MAX];static uint8_t cache[NFLSTREET_LOCKED_CACHE_BYTES],start[NFLSTREET_LOCKED_CACHE_BYTES];
 if(!pristine||!ram||!before)return 3;if(!crowd_present(pristine)){fprintf(stderr,"the saved RAM lacks the EARS crowd\n");return 4;}
 unsigned runs=0,total_calls=0,lists=0,skinned=0,draws_ended=0;uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);prepare(ram,index);memcpy(before,ram,RAM_BYTES);memcpy(start,locked,sizeof start);memcpy(cache,start,sizeof cache);
  trace t={calls,0};nflstreet_render_state s;
  if(!native_case(ram,&s,&t,cache)){fprintf(stderr,"case %u\n",index);return 5;}
  memcpy(oracle_ram,before,RAM_BYTES);memcpy(locked,start,sizeof locked);unsigned n=0;
  for(unsigned i=0;i<OBJECTS;++i){const uint32_t args[2]={object_at(i),1};trace rest={t.calls+n,t.count-n};unsigned k=0;
   if(!run_original(0x800d2f20u,args,&rest,&k)){fprintf(stderr,"case %u object %u (%08x)\n",index,i,object_at(i));return 6;}n+=k;++runs;}
  if(n!=t.count){fprintf(stderr,"case %u: original issued %u GX calls, native %u\n",index,n,t.count);return 7;}
  if(!gx_oracle_same_ram(ram,&s)){fprintf(stderr,"original crowd mismatch case %u\n",index);return 7;}
  if(memcmp(locked,cache,sizeof locked)){fprintf(stderr,"locked cache differs case %u\n",index);return 7;}
  total_calls+=n;for(unsigned i=0;i<n;++i){if(calls[i].op==NFLSTREET_GX_CALL_DISPLAY_LIST)++lists;}
  uint32_t objs[OBJECTS];unsigned m=models(before,objs);
  for(unsigned i=0;i<m;++i){if(ram_at(ram,objs[i]+0x134,1)[0])++skinned;}
  for(unsigned i=0;i<n;++i)if(calls[i].op==NFLSTREET_GX_SET_CULL_MODE&&calls[i].arg[0]==0)++draws_ended;
  digest=fnv(digest,ram,RAM_BYTES);digest=fnv(digest,calls,sizeof *calls*n);}
 printf("Original crowd: %u runs of 800D2F20 over %u entries x %u cases match %u GX calls (%u display lists), all RAM and the locked cache; "
  "%u model draws skinned their palettes, %u model and copy draws completed; digest %08x.\n",runs,OBJECTS,CASES,total_calls,lists,skinned,draws_ended,digest);
 return 0;}
