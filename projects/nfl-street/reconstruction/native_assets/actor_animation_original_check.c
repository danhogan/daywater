/* Native actor animation (actor_pose.c, actor_skeleton.c) against the
   original on a saved context. Every case prepares a fresh copy of the
   saved RAM, runs the native routine for all 14 actors and then the
   preserved original module from the same RAM (and locked cache, and
   8015E9C8's stack frame), and compares all RAM outside the oracle stack.
   Suites:
   - skeleton: 8015F08C per visual, either after 801893C8(1) and 801893EC
     (as 800D51B4 runs it) or on the viewport's stack with an edited top;
     edits to pose angles, flag 0x2000, +34/+4C, the skeleton's
     translations and bone count, the destination, the locked cache and its
     enable flag. The whole locked cache is compared.
   - blend: 8015E9C8 per visual with the saved pose list (case 0) or a
     random list of streamed records (built by the native 8015E938),
     compressed frames of the resident bank 7 (animdata 00111, descriptor
     808177E0, both plain and scattered/mirrored) and synthetic records,
     with random flags, weights, modes, +1C, +30 and root fields. The
     routine's stack frame (blend descriptor and decode buffer) is seeded
     identically and compared too.
   - root: 8015EC30 per gameplay actor's channels (+320, count from +31C,
     clip set +318) with edited channels, clip weights, flag 0x80 and
     skeleton bone count.
   - stream: 8015E938 on random bank 7 frames, plain and mirrored.
   - twist: 80053404 per gameplay actor with edited +12C/+130.
   The original module runs everything except the locked-cache enable,
   flush and store (801F0C84, 801F0BA0, 801F0BD0: no RAM effect) and the
   DMA 801F0DD4, served from the oracle's locked cache in 32-byte blocks.
   Scratch lists and frames at 80F10000..80F37000 (zero in the saved
   contexts) are harness allocations. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "actor_palettes.h"
#include "actor_skeleton.h"
#include "actor_pose.h"
#define ACTORS 14u
#define SKELETON_CASES 401u
#define BLEND_CASES 401u
#define ROOT_CASES 201u
#define STREAM_CASES 201u
#define TWIST_CASES 201u
#define LISTS 0x80f10000u   /* 0x200 per actor */
#define FRAMES 0x80f12000u  /* 0x1800 per actor, 0x400 per entry */
#define TEMP 0x80f30000u
#define OUTS 0x80f32000u
#define DESTS 0x80f2c000u
#define BANK_DESC 0x808177e0u /* the resident bank 7 descriptor (bank table entry 2) */
#define FRAME_BASE (STACK_HI-0x40u-0x218u) /* 8015E9C8's r1 when called with r1 = STACK_HI-0x40 */
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;static uint8_t locked[NFLSTREET_LOCKED_CACHE_BYTES];
static const uint8_t *bank;static size_t bank_n;
static void *external(CPUState *c,uint32_t a,uint32_t n){(void)c;return a>=0xe0000000u&&(uint64_t)a-0xe0000000u+n<=sizeof locked?locked+(a-0xe0000000u):NULL;}
static uint64_t read_external(CPUState *c,uint32_t a,uint8_t n){const uint8_t *p=external(c,a,n);uint64_t v=0;if(!p)return 0;for(unsigned i=0;i<n;++i)v=v<<8|p[i];return v;}
static void write_external(CPUState *c,uint32_t a,uint64_t v,uint8_t n){uint8_t *p=external(c,a,n);if(p)for(unsigned i=n;i;--i){p[i-1]=(uint8_t)v;v>>=8;}}
static void write_external32(CPUState *c,uint32_t a,uint32_t v,uint8_t region){(void)region;write_external(c,a,v,4);}
static uint32_t actor_at(uint8_t *ram,unsigned i){return ram32(ram,ram32(ram,R13-0x7bbcu)+4*i);}
static uint32_t gameplay[ACTORS];
static uint32_t gameplay_actor(uint8_t *ram,uint32_t visual){
 for(uint32_t o=0;o+0x400<=RAM_BYTES;o+=4){const uint8_t *p=ram+o;
  if(rd32(p+4)==visual&&(rd32(p+0x1a8)&0x7fffffffu)==(ram32(ram,visual+4)&0x7fffffffu)&&(rd32(p+0x1ac)&0x7fffffffu)==(ram32(ram,visual+8)&0x7fffffffu)&&
     (o+RAM_BASE<STACK_LO||o+RAM_BASE>=STACK_HI))return RAM_BASE+o;}
 return 0;}
static int run_original(uint32_t pc,const uint32_t *args,unsigned n){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.external_pointer=external;c.external_read=read_external;c.external_write=write_external;c.external_write32=write_external32;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;for(unsigned i=0;i<n;++i)c.gpr[3+i]=args[i];c.pc=pc;
 for(unsigned step=0;c.pc;++step){
  if(step>20000000u){fprintf(stderr,"%08x did not return (pc %08x)\n",pc,c.pc);return 0;}
  const uint32_t at=c.pc;
  if(at==0x801f0c84u||at==0x801f0bd0u||at==0x801f0ba0u){c.pc=c.lr;continue;}
  if(at==0x801f0dd4u){const uint32_t m=(c.gpr[5]+0x1fu)&~0x1fu;uint8_t *to=ram_at(oracle,c.gpr[3],m);const uint8_t *from=external(&c,c.gpr[4],m);
   if(m&&(!to||!from)){fprintf(stderr,"DMA %08x <- %08x (%u) outside memory\n",c.gpr[3],c.gpr[4],m);return 0;}if(m)memmove(to,from,m);c.pc=c.lr;continue;}
  if(at>=0x80200000u&&at<0x80210000u){fprintf(stderr,"unexpected SDK entry %08x (lr %08x)\n",at,c.lr);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"%08x: exception at %08x (lr %08x)\n",pc,c.pc,c.lr);return 0;}}
 return 1;}
static int same(const uint8_t *native,const uint8_t *native_cache){
 const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&oracle[i]!=native[i]){
  fprintf(stderr,"RAM differs at %08x: original %02x native %02x\n",RAM_BASE+i,oracle[i],native[i]);return 0;}
 if(native_cache)for(uint32_t i=0;i<sizeof locked;++i)if(locked[i]!=native_cache[i]){fprintf(stderr,"locked cache differs at +%04x\n",i);return 0;}
 return 1;}
static void put16(uint8_t *ram,uint32_t a,uint16_t v){uint8_t *p=ram_at(ram,a,2);p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put32(uint8_t *ram,uint32_t a,uint32_t v){wr32(ram_at(ram,a,4),v);}
static uint32_t u32(const uint8_t *p){return rd32(p);}
static unsigned frames(void){return (unsigned)bank[4]<<8|bank[5];}
static const uint8_t *frame(unsigned i){return bank+u32(bank+u32(bank+16)+4*i);}
/* A compressed bank 7 frame copied into RAM (with slack for the 64-bit reads). */
static void place_frame(uint8_t *ram,uint32_t at,unsigned index){
 const uint8_t *f=frame(index);size_t n=0x120;if((size_t)(f-bank)+n>bank_n)n=bank_n-(size_t)(f-bank);memcpy(ram_at(ram,at,(uint32_t)n),f,n);}
static nflstreet_actor_pose_context pose_ctx(uint8_t *ram){nflstreet_actor_pose_context c;memset(&c,0,sizeof c);c.span=span;c.memory=ram;return c;}
static nflstreet_actor_palette_context *skeleton_ctx(uint8_t *ram,nflstreet_player_draw_context *x){
 static nflstreet_actor_palette_context c;memset(&c,0,sizeof c);memset(x,0,sizeof *x);x->span=span;x->memory=ram;
 c.x=x;c.stacks=venue_stacks(ram);c.unit[0]=ramf(ram,0x803a3958u);c.unit[1]=ramf(ram,0x803a395cu);
 c.pool=R13-0x5680u;c.pool_index=R13-0x5678u;c.pool_used=R13-0x5670u;c.cache_on=R13-0x6200u;return &c;}
/* ---- skeleton (8015F08C) ---- */
static unsigned sk_runs,sk_frame,sk_fixed,sk_moved;
static int skeleton_case(uint8_t *pristine,uint8_t *ram,uint8_t *before,unsigned index){
 memcpy(ram,pristine,RAM_BYTES);static uint8_t cache[NFLSTREET_LOCKED_CACHE_BYTES];memset(cache,0,sizeof cache);
 const int frame_mode=!index||rnd()%3!=0;nflstreet_matrix_stacks k=venue_stacks(ram);
 if(!frame_mode){wr32(ram_at(ram,R13-0x65e8u,4),k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0]);
  uint32_t s=ram32(ram,R13-0x65e8u),top=ram32(ram,s+8)+0x40u*ram32(ram,s+4);
  for(unsigned i=0;i<12;++i)if(rnd()%2)put32(ram,top+4*i,fbits(i%4==3?rndf(-60.0f,60.0f):rndf(-1.5f,1.5f)));}
 uint32_t dest[ACTORS];
 if(index){if(rnd()%2)for(unsigned i=0;i<sizeof cache;++i)cache[i]=(uint8_t)rnd();
  if(rnd()%3==0)ram_at(ram,R13-0x6200u,1)[0]=0;
  const uint32_t skl=ram32(ram,actor_at(ram,0)+0x64);
  if(rnd()%3==0)for(unsigned b=0;b<29;++b)if(rnd()%4==0)for(unsigned w=0;w<3;++w)put32(ram,skl+16+16*b+4*w,fbits(rndf(-0.5f,0.5f)));
  if(rnd()%6==0)put16(ram,skl+6,(uint16_t)(1+rnd()%29));
  for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor_at(ram,i);uint8_t *p=ram_at(ram,a,0x68);
   if(rnd()%2){uint32_t pose=rd32(p+0x5c);for(unsigned b=0;b<90;++b)if(rnd()%3==0)put16(ram,pose+2*b,(uint16_t)rnd());}
   if(rnd()%5==0)wr32(p+0x14,rd32(p+0x14)^0x2000u);
   if(rnd()%2)for(unsigned w=0;w<3;++w)wr32(p+0x34+4*w,fbits(rndf(-2.0f,2.0f)));
   if(rnd()%2)wr32(p+0x4c,rnd()%4?rnd()&0xffffffu:rnd()<<8|rnd()%256);
   if(rnd()%5==0)wr32(p+0x60,DESTS+0x800u*(i%8));}}
 for(unsigned i=0;i<ACTORS;++i){const uint32_t a=actor_at(ram,i);dest[i]=ram32(ram,a+0x60);sk_fixed+=ram32(ram,a+0x14)&0x2000u?1u:0u;sk_moved+=dest[i]!=ram32(pristine,a+0x60);}
 memcpy(before,ram,RAM_BYTES);memcpy(locked,cache,sizeof locked);
 nflstreet_player_draw_context x;nflstreet_actor_palette_context *c=skeleton_ctx(ram,&x);memcpy(c->locked,cache,sizeof cache);
 for(unsigned i=0;i<ACTORS;++i)if(!(frame_mode?nflstreet_actor_skeleton_frame(c,actor_at(ram,i)):nflstreet_actor_skeleton(c,actor_at(ram,i)))){
  fprintf(stderr,"native skeleton actor %u rejected (case %u)\n",i,index);return 0;}
 memcpy(cache,c->locked,sizeof cache);memcpy(oracle,before,RAM_BYTES);
 for(unsigned i=0;i<ACTORS;++i){const uint32_t a=actor_at(before,i),one=1;
  if(frame_mode&&(!run_original(0x801893c8u,&one,1)||!run_original(0x801893ecu,NULL,0)))return 0;
  if(!run_original(0x8015f08cu,&a,1)){fprintf(stderr,"original skeleton actor %u (case %u)\n",i,index);return 0;}++sk_runs;sk_frame+=frame_mode?1u:0u;}
 if(!same(ram,cache)){fprintf(stderr,"skeleton mismatch case %u\n",index);return 0;}return 1;}
/* ---- blend (8015E9C8) ---- */
static unsigned bl_runs,bl_entries,bl_kind[3],bl_mirror,bl_flag4,bl_modes[4];
static void random_list(uint8_t *ram,unsigned actor,uint32_t list){
 const unsigned n=1+rnd()%(rnd()%4?4:6);put16(ram,list,(uint16_t)n);
 for(unsigned j=0;j<n;++j){const uint32_t e=list+4+0x2c*j,f=FRAMES+0x1800u*actor+0x400u*j+(rnd()%3==0?1u:0u);
  for(unsigned b=0;b<0x2c;b+=4)put32(ram,e+b,rnd()<<8^rnd());
  const unsigned r=rnd()%8;put32(ram,e+4,fbits(r==0?0.0f:r==1?1.0f:r==2?1.25f:rndf(0.0f,1.0f)));
  uint8_t flags=(uint8_t)rnd();if(rnd()%2)flags&=0x35;ram_at(ram,e+8,1)[0]=flags;
  const unsigned kind=rnd()%8<3?0u:rnd()%2?1u:2u;
  if(kind==0){nflstreet_actor_pose_context c=pose_ctx(ram);place_frame(ram,TEMP,rnd()%frames());
   if(!nflstreet_actor_pose_stream(&c,f,TEMP,BANK_DESC,0))fprintf(stderr,"native stream failed\n");put32(ram,e+0x24,0);}
  else if(kind==1){place_frame(ram,f,rnd()%frames());put32(ram,e+0x24,BANK_DESC);}
  else{const unsigned bones=(flags&1)?1+rnd()%30:1+rnd()%40;uint8_t *p=ram_at(ram,f,0x200);p[0]=(uint8_t)(bones|(rnd()&0xc0u));
   for(unsigned b=1;b<22+bones*6;++b)p[b]=(uint8_t)rnd();put32(ram,e+0x24,0);}
  put32(ram,e+0x28,f);++bl_kind[kind];bl_mirror+=flags&1;bl_flag4+=(flags>>2)&1;++bl_entries;}}
static int blend_case(uint8_t *pristine,uint8_t *ram,uint8_t *before,unsigned index){
 memcpy(ram,pristine,RAM_BYTES);uint32_t list[ACTORS];uint8_t local[NFLSTREET_POSE_LOCAL_BYTES]={0};
 for(unsigned i=0;i<ACTORS;++i){list[i]=0x8032bf28u;if(!index)continue;const uint32_t v=actor_at(ram,i);uint8_t *p=ram_at(ram,v,0x68);
  list[i]=LISTS+0x200u*i;random_list(ram,i,list[i]);
  if(rnd()%2)wr32(p+0x1c,fbits(rnd()%4?rndf(0.5f,1.5f):ramf(ram,0x80260670u)));
  wr32(p+0x20,rnd()%4);if(rnd()%4==0)put16(ram,v+0x30,(uint16_t)(26+rnd()%8));
  if(rnd()%2)for(unsigned w=0;w<9;++w)wr32(p+0x34+4*w,w==6?rnd()<<8^rnd():fbits(rndf(-2.0f,2.0f)));
  if(rnd()%2)for(unsigned w=0;w<3;++w)wr32(p+0x50+4*w,rnd()<<8^rnd());
  if(rnd()%3==0){uint32_t pose=rd32(p+0x5c);for(unsigned b=0;b<96;++b)put16(ram,pose+2*b,(uint16_t)rnd());}}
 if(index)for(unsigned i=0;i<sizeof local;++i)local[i]=(uint8_t)rnd();
 for(unsigned i=0;i<ACTORS;++i)++bl_modes[ram32(ram,actor_at(ram,i)+0x20)&3u];
 memcpy(before,ram,RAM_BYTES);
 nflstreet_actor_pose_context c=pose_ctx(ram);memcpy(c.local,local,sizeof local);
 for(unsigned i=0;i<ACTORS;++i)if(!nflstreet_actor_pose_blend(&c,actor_at(ram,i),list[i])){fprintf(stderr,"native blend actor %u rejected (case %u)\n",i,index);return 0;}
 memcpy(oracle,before,RAM_BYTES);memcpy(ram_at(oracle,FRAME_BASE+8,sizeof local),local,sizeof local);
 for(unsigned i=0;i<ACTORS;++i){const uint32_t args[2]={actor_at(before,i),list[i]};
  if(!run_original(0x8015e9c8u,args,2)){fprintf(stderr,"original blend actor %u (case %u)\n",i,index);return 0;}++bl_runs;}
 const uint8_t *frame_now=ram_at(oracle,FRAME_BASE+8,sizeof local);
 for(unsigned i=0;i<sizeof local;++i)if((i<0x30||i>=0x34)&&frame_now[i]!=c.local[i]){fprintf(stderr,"blend frame differs at +%03x case %u\n",i,index);return 0;}
 if(rd32(frame_now+0x30)!=FRAME_BASE+0x40u||rd32(c.local+0x30)!=NFLSTREET_POSE_LOCAL+0x38u){fprintf(stderr,"blend buffer pointer case %u\n",index);return 0;}
 if(!same(ram,NULL)){fprintf(stderr,"blend mismatch case %u\n",index);return 0;}return 1;}
/* ---- root (8015EC30) ---- */
static unsigned rt_runs,rt_active,rt_flag80;
static int root_case(uint8_t *pristine,uint8_t *ram,uint8_t *before,unsigned index){
 memcpy(ram,pristine,RAM_BYTES);unsigned count[ACTORS];
 if(index&&rnd()%4==0)put16(ram,ram32(ram,actor_at(ram,0)+0x64)+6,(uint16_t)(20+rnd()%15));
 for(unsigned i=0;i<ACTORS;++i){const uint32_t g=gameplay[i],v=actor_at(ram,i),set=ram32(ram,g+0x31c),ch=ram32(ram,g+0x320),clips=ram32(ram,g+0x318);
  count[i]=ram_at(ram,set+4,2)[0]<<8|ram_at(ram,set+4,2)[1];if(!index)continue;
  if(rnd()%4==0)count[i]=rnd()%11;if(rnd()%4==0)wr32(ram_at(ram,v+0x14,4),ram32(ram,v+0x14)^0x80u);
  if(rnd()%2){const uint32_t pose=ram32(ram,v+0x5c);for(unsigned b=0;b<35*3;++b)put16(ram,pose+2*b,(uint16_t)rnd());} /* bones past 26 nonzero */
  for(unsigned k=0;k<4;++k){const uint32_t e=clips+100*k;if(rnd()%3==0)put16(ram,e+6,(uint16_t)(rnd()%2));if(rnd()%3==0)put16(ram,e+4,(uint16_t)(rnd()%3?0x4a:0x1e));
   if(rnd()%2)put32(ram,e+0x50,fbits(rnd()%5?rndf(0.0f,1.0f):0.0f));}
  for(unsigned k=0;k<10;++k){const uint32_t c=ch+0x7c*k;uint8_t *p=ram_at(ram,c,0x7c);
   if(rnd()%3==0)p[1]=(uint8_t)(rnd()%3);if(rnd()%5==0)wr32(p+0x48,fbits(rnd()%2?1.0f:-0.0f));if(rnd()%2)wr32(p+0x30,fbits(rnd()%6?rndf(0.0f,1.0f):0.0f));
   if(rnd()%2)for(unsigned w=0;w<3;++w)wr32(p+0x10+4*w,rnd()%4?rnd()<<8^rnd():rnd()<<8);
   if(rnd()%4==0)put16(ram,c+8,(uint16_t)(rnd()%3?0x4a:rnd()%2?0x1e:rnd()));}}
 for(unsigned i=0;i<ACTORS;++i){const uint32_t ch=ram32(ram,gameplay[i]+0x320);rt_flag80+=ram32(ram,actor_at(ram,i)+0x14)&0x80u?1u:0u;
  for(unsigned k=0;k<count[i];++k)rt_active+=ram_at(ram,ch+0x7c*k+1,1)[0]&&ram32(ram,ch+0x7c*k+0x48)==0?1u:0u;}
 memcpy(before,ram,RAM_BYTES);nflstreet_actor_pose_context c=pose_ctx(ram);
 for(unsigned i=0;i<ACTORS;++i){const uint32_t g=gameplay[i];
  if(!nflstreet_actor_pose_root(&c,actor_at(ram,i),ram32(ram,g+0x320),count[i],ram32(ram,g+0x318))){fprintf(stderr,"native root %u rejected\n",i);return 0;}}
 memcpy(oracle,before,RAM_BYTES);
 for(unsigned i=0;i<ACTORS;++i){const uint32_t g=gameplay[i],args[4]={actor_at(before,i),ram32(before,g+0x320),count[i],ram32(before,g+0x318)};
  if(!run_original(0x8015ec30u,args,4))return 0;++rt_runs;}
 if(!same(ram,NULL)){fprintf(stderr,"root mismatch case %u\n",index);return 0;}return 1;}
/* ---- stream (8015E938) ---- */
static unsigned st_runs,st_mirror;
static int stream_case(uint8_t *pristine,uint8_t *ram,uint8_t *before,unsigned index){
 memcpy(ram,pristine,RAM_BYTES);unsigned mirror[4];uint32_t from[4];
 for(unsigned k=0;k<4;++k){from[k]=TEMP+0x200u*k+(rnd()%2);place_frame(ram,from[k],index?rnd()%frames():k);mirror[k]=index&&rnd()%2;st_mirror+=mirror[k];
  if(index&&rnd()%2)for(unsigned b=0;b<0x100;++b)ram_at(ram,OUTS+0x200u*k+b,1)[0]=(uint8_t)rnd();}
 memcpy(before,ram,RAM_BYTES);nflstreet_actor_pose_context c=pose_ctx(ram);
 for(unsigned k=0;k<4;++k)if(!nflstreet_actor_pose_stream(&c,OUTS+0x200u*k,from[k],BANK_DESC,(int)mirror[k])){fprintf(stderr,"native stream rejected\n");return 0;}
 memcpy(oracle,before,RAM_BYTES);
 for(unsigned k=0;k<4;++k){const uint32_t args[4]={OUTS+0x200u*k,from[k],BANK_DESC,mirror[k]};if(!run_original(0x8015e938u,args,4))return 0;++st_runs;}
 if(!same(ram,NULL)){fprintf(stderr,"stream mismatch case %u\n",index);return 0;}return 1;}
/* ---- twist (80053404) ---- */
static unsigned tw_runs;
static int twist_case(uint8_t *pristine,uint8_t *ram,uint8_t *before,unsigned index){
 memcpy(ram,pristine,RAM_BYTES);
 if(index)for(unsigned i=0;i<ACTORS;++i)for(unsigned k=0;k<2;++k)if(rnd()%4)put32(ram,gameplay[i]+0x12c+4*k,rnd()%3?rnd()<<8^rnd():rnd()&0xffffffu);
 memcpy(before,ram,RAM_BYTES);nflstreet_actor_pose_context c=pose_ctx(ram);
 for(unsigned i=0;i<ACTORS;++i)if(!nflstreet_actor_pose_twist(&c,gameplay[i]))return 0;
 memcpy(oracle,before,RAM_BYTES);for(unsigned i=0;i<ACTORS;++i){if(!run_original(0x80053404u,&gameplay[i],1))return 0;++tw_runs;}
 if(!same(ram,NULL)){fprintf(stderr,"twist mismatch case %u\n",index);return 0;}return 1;}
/* The bank file is the resident bank 7: its descriptor arrays equal RAM's. */
static int resident(uint8_t *ram){
 const uint8_t *d=bank+u32(bank+20);if(u32(d)!=ram32(ram,BANK_DESC))return 0;const uint32_t n=u32(d);
 for(unsigned k=1;k<6;++k){const uint8_t *r=ram_at(ram,ram32(ram,BANK_DESC+4*k),2*n);if(!r||memcmp(bank+u32(d+4*k),r,2*n))return 0;}
 const uint32_t header=ram32(ram,ram32(ram,R13-0x4f6cu)+0x1cu*2+0xc); /* bank table entry 2 (80175F48) */
 return !bank[2]&&bank[3]==7&&ram32(ram,header)==0x00010007u&&ram32(ram,header+0x14)==BANK_DESC;}
/* The live pose source's channel-root inference (live_pose.c): a saved
   active bank 7 channel's +10..+18 against the shortest-turn interpolation
   by its fraction (+60) between the list-entry roots 8015E9C8 computes for
   its two frames (+5C, +5E) under its flags (+7), heading 0 under flag 4.
   Frames are decoded from the bank file by the verified 8015E938. Counts
   the channels reproduced exactly (all three words); the others hold roots
   no frame pair of theirs explains (their producer is not identified). */
static uint32_t turn24(float t,uint32_t a,uint32_t b){
 a&=0xffffffu;b&=0xffffffu;const int32_t d=(int32_t)(a-b);if(d>0x800000)b+=0x1000000u;else if(d< -0x800000)a+=0x1000000u;
 return b+(uint32_t)(int32_t)(((int64_t)(int32_t)(a-b)*nflstreet_fctiwz(t*16777216.0f))>>24);}
static int inference(uint8_t *pristine,uint8_t *ram,unsigned *checked,unsigned *matched){
 memcpy(ram,pristine,RAM_BYTES);nflstreet_actor_pose_context c=pose_ctx(ram);
 for(unsigned i=0;i<ACTORS;++i){const uint32_t g=gameplay[i],set=ram32(ram,g+0x31c),ch=ram32(ram,g+0x320);const unsigned n=ram_at(ram,set+4,2)[0]<<8|ram_at(ram,set+4,2)[1];
  for(unsigned k=0;k<n;++k){const uint32_t a=ch+0x7c*k;const uint8_t *p=ram_at(ram,a,0x7c);if(!p[1]||p[6]!=2)continue;
   uint32_t root[2][3];
   for(unsigned s=0;s<2;++s){const unsigned f=(unsigned)p[0x5c+2*s]<<8|p[0x5d+2*s];if(f>=frames())return 0;place_frame(ram,TEMP,f);
    if(!nflstreet_actor_pose_stream(&c,OUTS,TEMP,BANK_DESC,0))return 0;
    put16(ram,LISTS,1);for(unsigned b=4;b<0x30;b+=4)put32(ram,LISTS+b,0);ram_at(ram,LISTS+12,1)[0]=p[7];put32(ram,LISTS+0x2c,OUTS);
    if(!nflstreet_actor_pose_blend(&c,actor_at(ram,i),LISTS))return 0;for(unsigned w=0;w<3;++w)root[s][w]=ram32(ram,LISTS+0x10+4*w);}
   const float t=ramf(ram,a+0x60);int exact=1;
   for(unsigned w=0;w<3;++w){const uint32_t expect=w==1&&(p[7]&4)?0u:turn24(t,root[1][w],root[0][w]);
    if(expect!=ram32(ram,a+0x10+4*w))exact=0;}
   ++*checked;*matched+=(unsigned)exact;}}
 return 1;}
int main(int argc,char **argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 FILE *f=fopen(argv[3],"rb");if(!f)return 3;fseek(f,0,SEEK_END);bank_n=(size_t)ftell(f);rewind(f);uint8_t *b=malloc(bank_n);
 if(!b||fread(b,1,bank_n,f)!=bank_n)return 3;fclose(f);bank=b;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);
 if(!pristine||!ram||!before||!oracle)return 3;if(!resident(pristine)){fprintf(stderr,"bank file is not the resident bank 7\n");return 4;}
 for(unsigned i=0;i<ACTORS;++i)if(!(gameplay[i]=gameplay_actor(pristine,actor_at(pristine,i)))){fprintf(stderr,"no gameplay actor for visual %u\n",i);return 4;}
 unsigned inferred=0,reproduced=0;if(!inference(pristine,ram,&inferred,&reproduced)||!inferred){fprintf(stderr,"channel root inference failed\n");return 10;}
 uint32_t digest=2166136261u;
 for(unsigned i=0;i<SKELETON_CASES;++i){if(!skeleton_case(pristine,ram,before,i))return 5;digest=fnv(digest,ram,RAM_BYTES);}
 for(unsigned i=0;i<BLEND_CASES;++i){if(!blend_case(pristine,ram,before,i))return 6;digest=fnv(digest,ram,RAM_BYTES);}
 for(unsigned i=0;i<ROOT_CASES;++i){if(!root_case(pristine,ram,before,i))return 7;digest=fnv(digest,ram,RAM_BYTES);}
 for(unsigned i=0;i<STREAM_CASES;++i){if(!stream_case(pristine,ram,before,i))return 8;digest=fnv(digest,ram,RAM_BYTES);}
 for(unsigned i=0;i<TWIST_CASES;++i){if(!twist_case(pristine,ram,before,i))return 9;digest=fnv(digest,ram,RAM_BYTES);}
 printf("Original actor animation: %u runs of 8015F08C (%u after 801893C8(1)/801893EC, %u with flag 0x2000, %u moved destinations) "
  "match all RAM and the locked cache; %u runs of 8015E9C8 over %u entries (streamed/compressed/synthetic %u/%u/%u, %u mirrored, %u flag 4; "
  "modes %u/%u/%u/%u) match all RAM and its stack frame; %u runs of 8015EC30 (%u active channels, %u with flag 0x80), %u of 8015E938 "
  "(%u mirrored) and %u of 80053404 match all RAM; the live channel-root inference reproduces %u of %u saved active bank 7 channels exactly; digest %08x.\n",
  sk_runs,sk_frame,sk_fixed,sk_moved,bl_runs,bl_entries,bl_kind[0],bl_kind[1],bl_kind[2],bl_mirror,bl_flag4,bl_modes[0],bl_modes[1],bl_modes[2],bl_modes[3],
  rt_runs,rt_active,rt_flag80,st_runs,st_mirror,tw_runs,reproduced,inferred,digest);
 return 0;}
