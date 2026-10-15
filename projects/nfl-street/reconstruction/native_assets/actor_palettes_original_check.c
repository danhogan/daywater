/* Native actor palettes (80162760) and placement (800D4A6C with the world
   matrix 800D4B0C) against the original on a saved context. Each case prepares a fresh copy of the saved RAM (the viewport's
   stack selected, the palette pool emptied as at frame start) and runs all
   14 actors, natively and then in the preserved original module from the
   same RAM and locked cache. Every routine executes in the original
   (including the head and hand dynamics 8004C268/8004BF1C) except: the
   locked-cache enable and cache flush (801F0C84, 801F0BD0), which have no RAM effect;
   and the locked-cache DMA 801F0DD4, served by copying whole 32-byte blocks
   from the oracle's locked cache. All RAM outside the oracle stack and the
   locked cache must be equal. Case 0 is the saved frame; the others edit
   world matrices, scales, offsets, headings, flags, LOD, pose angles,
   attachment scale, held-item enables, scratch words, the dynamics and
   locked-cache flags, hand models (borrowed held-item models; the saved
   actors have none) and the camera. Placement cases run 800D4A6C for every
   gameplay actor (found as the record whose +04 names a visual actor and
   whose +1A8 position equals the visual's +04) with edited positions,
   headings, +30C, visual flags, offsets and the mirrored side.
   Optional third argument "slerp" adds flag 0x4000, compact-pose/rest
   comparisons, cluster weights in [0,4096] and 8028DD70 enable-byte edits.
   It reports slerp coverage separately and omits the unrelated placement
   suite; without that argument the historical palette suite is unchanged. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "actor_palettes.h"
#include "actor_world.h"
#define CASES 401u
#define PLACE_CASES 401u
#define ACTORS 14u
#define SCALE_VECTOR 0x80f37f00u /* below the oracle stack: an optional x/y/z scale */
#define ITEMS 0x80f37f10u        /* the two held-item model words (80164000 passes its +28) */
#define SLERP_ENABLES 0x8028dd70u
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;static uint8_t locked[NFLSTREET_LOCKED_CACHE_BYTES];static int slerp;
typedef struct {uint32_t lod[ACTORS];int scaled[ACTORS];} palette_case;
static void *external(CPUState *c,uint32_t a,uint32_t n){(void)c;return a>=0xe0000000u&&(uint64_t)a-0xe0000000u+n<=sizeof locked?locked+(a-0xe0000000u):NULL;}
static uint64_t read_external(CPUState *c,uint32_t a,uint8_t n){const uint8_t *p=external(c,a,n);uint64_t v=0;if(!p)return 0;for(unsigned i=0;i<n;++i)v=v<<8|p[i];return v;}
static void write_external(CPUState *c,uint32_t a,uint64_t v,uint8_t n){uint8_t *p=external(c,a,n);if(p)for(unsigned i=n;i;--i){p[i-1]=(uint8_t)v;v>>=8;}}
static void write_external32(CPUState *c,uint32_t a,uint32_t v,uint8_t region){(void)region;write_external(c,a,v,4);}
static uint32_t actor_at(uint8_t *ram,unsigned i){return ram32(ram,ram32(ram,R13-0x7bbcu)+4*i);}
static nflstreet_actor_palette_context *native_context(uint8_t *ram,nflstreet_player_draw_context *x){
 static nflstreet_actor_palette_context c;memset(&c,0,sizeof c);memset(x,0,sizeof *x);x->span=span;x->memory=ram;
 c.x=x;c.stacks=venue_stacks(ram);c.unit[0]=ramf(ram,0x803a3958u);c.unit[1]=ramf(ram,0x803a395cu);
 c.pool=R13-0x5680u;c.pool_index=R13-0x5678u;c.pool_used=R13-0x5670u;c.cache_on=R13-0x6200u;return &c;}
static void prepare(uint8_t *ram,unsigned index,palette_case *pc){
 nflstreet_matrix_stacks k=venue_stacks(ram);
 wr32(ram_at(ram,R13-0x65e8u,4),k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0]);wr32(ram_at(ram,R13-0x5670u,4),0);
 for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor_at(ram,i),p5=ram32(ram,a+0x4a8);pc->lod[i]=ram32(ram,p5+8);pc->scaled[i]=0;}
 wr32(ram_at(ram,ITEMS,4),ram32(ram,ram32(ram,actor_at(ram,0)+0x4a8)+0x28));wr32(ram_at(ram,ITEMS+4,4),ram32(ram,ram32(ram,actor_at(ram,0)+0x4a8)+0x2c));
 for(unsigned i=0;i<3;++i)wr32(ram_at(ram,SCALE_VECTOR+4*i,4),fbits(1.0f));
 if(!index)return;
 if(rnd()%3==0)ram_at(ram,R13-0x6200u,1)[0]=0; /* the locked cache not yet enabled this frame */
 if(rnd()%2){uint32_t s=ram32(ram,R13-0x65e8u),top=ram32(ram,s+8)+0x40u*ram32(ram,s+4);
  for(unsigned i=0;i<12;++i)if(rnd()%2)wr32(ram_at(ram,top+4*i,4),fbits(i%4==3?rndf(-60.0f,60.0f):rndf(-1.5f,1.5f)));}
 for(unsigned i=0;i<3;++i)wr32(ram_at(ram,SCALE_VECTOR+4*i,4),fbits(rndf(0.5f,1.5f)));
 for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor_at(ram,i);uint8_t *p=ram_at(ram,a,0x1360);
  if(rnd()%2)for(unsigned w=0;w<12;++w)if(rnd()%2)wr32(p+0x3ac+4*w,fbits(w%4==3?rndf(-30.0f,30.0f):rndf(-1.2f,1.2f)));
  if(rnd()%3==0)wr32(p+0x18,fbits(rndf(0.8f,1.2f)));if(rnd()%3==0)for(unsigned w=0;w<3;++w)wr32(p+0x34+4*w,fbits(rndf(-2.0f,2.0f)));
  if(rnd()%2)wr32(p+0x4c,rnd()&0xffffffu);if(rnd()%6==0)wr32(p+0x14,rd32(p+0x14)^0x2000u);if(rnd()%3==0)wr32(p+0x14,rd32(p+0x14)^8u);
  if(rnd()%3==0)wr32(p+0x1358,fbits(rndf(0.7f,1.3f)));for(unsigned k=0;k<2;++k)if(rnd()%4==0)p[0x78+0x48*k]=(uint8_t)(rnd()%3);
  if(rnd()%3==0)pc->lod[i]=rnd()%4;pc->scaled[i]=rnd()%4==0;
  for(unsigned k=0;k<3;++k)if(rnd()%3==0)p[(k==2?0x214:0x11c+0x7c*k)+2]=(uint8_t)(1+rnd()%255); /* dynamics state +02 */
  /* The saved actors have no hand models (+D28, +E18): borrow the held-item
     models so the hand path runs on real model data. */
  for(unsigned k=0;k<2;++k)if(rnd()%2)wr32(p+0xd28+0xf0*k,ram32(ram,ITEMS+4*(rnd()%2)));
  if(rnd()%8==0)wr32(p+0x1008+4,0); /* the body built in place, then flushed */
  if(rnd()%2){uint32_t pose=rd32(p+0x5c);for(unsigned b=0;b<75*3;++b)if(rnd()%3==0){uint8_t *q=ram_at(ram,pose+2*b,2);uint16_t v=(uint16_t)rnd();q[0]=(uint8_t)(v>>8);q[1]=(uint8_t)v;}}}}
static int prepare_slerp(uint8_t *ram,unsigned index){
 unsigned max_bones=0;
 for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor_at(ram,i),model=ram32(ram,a+0x4b8),skel=ram32(ram,model+16),bones=ram32(ram,skel);
  if(!bones||bones>256)return 0;if(bones>max_bones)max_bones=bones;
  /* Case 0: the original pose with every body on the slerp route. Case 1:
     mixed ordinary/slerp bodies. Cases 2/3/4: disabled/weight 0/weight 1.
     Remaining cases mix both routes, in-place and locked-cache output. */
  uint32_t flags=ram32(ram,a+0x14)&~0x4000u;
  if(index==0||(index>=2&&index<=4)||((index+i)&1u))flags|=0x4000u;
  wr32(ram_at(ram,a+0x14,4),flags);
  if(!index)continue;
  uint32_t clst=ram32(ram,model+12),count=ram32(ram,clst+4),rows=ram32(ram,clst),pose=ram32(ram,a+0x5c),rest=ram32(ram,skel+12);
  if(!count||count>256||!ram_at(ram,rows,count*4)||!ram_at(ram,pose,bones*6)||!ram_at(ram,rest,bones*6))return 0;
  for(unsigned b=0;b<bones;++b)for(unsigned axis=0;axis<3;++axis)if(index<=4||rnd()%3==0){
   uint8_t *p=ram_at(ram,pose+6*b+2*axis,2);uint16_t value=(uint16_t)rnd();
   if(index%11==0){const uint8_t *r=ram_at(ram,rest+6*b+2*axis,2);value=(uint16_t)((unsigned)r[0]<<8|r[1]);}
   p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value;}
  for(unsigned row=1;row<count;++row){uint8_t *p=ram_at(ram,rows+4*row,4);unsigned weight;
   if(index==3)weight=0;else if(index==4)weight=4096;else if(rnd()%2)weight=rnd()%4097;else {static const unsigned edge[]={0,1,2048,4095,4096};weight=edge[(index+row)%5];}
   p[2]=(uint8_t)(weight>>8);p[3]=(uint8_t)weight;}}
 if(index){uint8_t *enable=ram_at(ram,SLERP_ENABLES,max_bones);if(!enable)return 0;
  for(unsigned bone=0;bone<max_bones;++bone)enable[bone]=(uint8_t)(index==2?0:index<=4?1:rnd()%3==0?0:1+rnd()%255);}
 return 1;}
static int native_case(uint8_t *ram,const palette_case *pc,uint8_t *cache){
 nflstreet_player_draw_context x;nflstreet_actor_palette_context *c=native_context(ram,&x);memcpy(c->locked,cache,sizeof c->locked);
 for(unsigned i=0;i<ACTORS;++i){float v[3];for(unsigned k=0;k<3;++k)v[k]=ramf(ram,SCALE_VECTOR+4*k);uint32_t a=actor_at(ram,i);
  if(!nflstreet_actor_palettes(c,a,pc->scaled[i]?v:NULL,a+0x3ac,ITEMS,pc->lod[i])){fprintf(stderr,"native actor %u (%08x) rejected\n",i,a);return 0;}}
 memcpy(cache,c->locked,sizeof c->locked);return 1;}
static int run_original(uint32_t pc,const uint32_t *args,unsigned n){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.external_pointer=external;c.external_read=read_external;c.external_write=write_external;c.external_write32=write_external32;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 for(unsigned i=0;i<n;++i)c.gpr[3+i]=args[i];c.pc=pc;
 for(unsigned step=0;c.pc;++step){
  if(step>20000000u){fprintf(stderr,"%08x did not return (pc %08x)\n",pc,c.pc);return 0;}
  const uint32_t at=c.pc;
  if(at==0x801f0c84u||at==0x801f0bd0u){c.pc=c.lr;continue;}
  if(at==0x801f0dd4u){const uint32_t n=(c.gpr[5]+0x1fu)&~0x1fu;uint8_t *to=ram_at(oracle,c.gpr[3],n);const uint8_t *from=external(&c,c.gpr[4],n);
   if(n&&(!to||!from)){fprintf(stderr,"DMA %08x <- %08x (%u) outside memory\n",c.gpr[3],c.gpr[4],n);return 0;}if(n)memmove(to,from,n);c.pc=c.lr;continue;}
  if(at>=0x80200000u&&at<0x80210000u){fprintf(stderr,"unexpected SDK entry %08x (lr %08x)\n",at,c.lr);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"%08x: exception at %08x (lr %08x)\n",pc,c.pc,c.lr);return 0;}}
 return 1;}
static int run_original(uint32_t pc,const uint32_t *args,unsigned n);
static int original_actor(uint32_t actor,uint32_t scale,uint32_t lod){
 const uint32_t args[8]={actor,scale,actor+0x3ac,0,0,0,ITEMS,lod};return run_original(0x80162760u,args,8);}
/* The gameplay actor whose +04 is the visual and whose position matches. */
static uint32_t gameplay_actor(uint8_t *ram,uint32_t visual){
 for(uint32_t o=0;o+0x310<=RAM_BYTES;o+=4){const uint8_t *p=ram+o;
  if(rd32(p+4)==visual&&(rd32(p+0x1a8)&0x7fffffffu)==(ram32(ram,visual+4)&0x7fffffffu)&&(rd32(p+0x1ac)&0x7fffffffu)==(ram32(ram,visual+8)&0x7fffffffu)&&
     (o+RAM_BASE<STACK_LO||o+RAM_BASE>=STACK_HI))return RAM_BASE+o;}
 return 0;}
static uint32_t gameplay[ACTORS];
static void prepare_place(uint8_t *ram,unsigned index){
 nflstreet_matrix_stacks k=venue_stacks(ram);wr32(ram_at(ram,R13-0x65e8u,4),k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0]);
 if(!index)return;uint32_t side=ram32(ram,R13-0x7078u);
 if(side&&rnd()%3==0)ram_at(ram,side+0x39,1)[0]=(uint8_t)(rnd()%3);
 if(rnd()%5==0)wr32(ram_at(ram,R13-0x7078u,4),0);
 for(unsigned i=0;i<ACTORS;++i){uint8_t *g=ram_at(ram,gameplay[i],0x310),*v=ram_at(ram,actor_at(ram,i),0x50);
  if(rnd()%2)for(unsigned w=0;w<3;++w)wr32(g+0x1a8+4*w,fbits(rndf(-40.0f,40.0f)));if(rnd()%2)wr32(g+0x1c0,rnd()&0xffffffu);
  if(rnd()%3==0)wr32(g+0x30c,rnd());if(rnd()%4==0)wr32(v+0x14,rd32(v+0x14)^0x2000u);
  if(rnd()%3==0){for(unsigned w=0;w<3;++w)wr32(v+0x34+4*w,fbits(rndf(-2.0f,2.0f)));wr32(v+0x4c,rnd()&0xffffffu);}}}
static int place_case(uint8_t *ram){
 nflstreet_player_draw_context x;memset(&x,0,sizeof x);x.span=span;x.memory=ram;static nflstreet_render_state unused;nflstreet_draw d={&x,&unused};
 const nflstreet_matrix_stacks k=venue_stacks(ram);
 for(unsigned i=0;i<ACTORS;++i)if(!nflstreet_actor_place(&d,&k,gameplay[i],R13-0x7078u)){fprintf(stderr,"native placement %u rejected\n",i);return 0;}
 return 1;}
static int same(const uint8_t *native,const uint8_t *native_cache){
 const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&oracle[i]!=native[i]){
  fprintf(stderr,"RAM differs at %08x: original %02x native %02x (words %08x/%08x)\n",RAM_BASE+i,oracle[i],native[i],ram32(oracle,(RAM_BASE+i)&~3u),ram32((uint8_t *)native,(RAM_BASE+i)&~3u));return 0;}
 for(uint32_t i=0;i<sizeof locked;++i)if(locked[i]!=native_cache[i]){fprintf(stderr,"locked cache differs at +%04x\n",i);return 0;}
 return 1;}
int main(int argc,char **argv){
 if(argc!=3&&argc!=4)return 2;if(argc==4&&strcmp(argv[3],"slerp"))return 2;slerp=argc==4;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);
 static uint8_t cache[NFLSTREET_LOCKED_CACHE_BYTES];if(!pristine||!ram||!before||!oracle)return 3;
 unsigned runs=0,matrices=0,lods[4]={0},scaled=0,hands=0,items=0,flagged=0,enabled_rows=0,disabled_rows=0,zero_weights=0,unit_weights=0;uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);palette_case pc;prepare(ram,index,&pc);if(slerp&&!prepare_slerp(ram,index)){fprintf(stderr,"bad slerp fixture case %u\n",index);return 4;}memcpy(before,ram,RAM_BYTES);
  memset(cache,0,sizeof cache);if(!native_case(ram,&pc,cache)){fprintf(stderr,"case %u\n",index);return 5;}
  memcpy(oracle,before,RAM_BYTES);memset(locked,0,sizeof locked);
  for(unsigned i=0;i<ACTORS;++i){if(!original_actor(actor_at(before,i),pc.scaled[i]?SCALE_VECTOR:0,pc.lod[i])){fprintf(stderr,"case %u actor %u\n",index,i);return 6;}
   ++runs;++lods[pc.lod[i]];scaled+=pc.scaled[i]?1u:0u;
   const uint32_t a=actor_at(before,i);if(!pc.lod[i])for(unsigned k=0;k<2;++k)hands+=ram32(before,a+0xd28+0xf0*k)?1u:0u;
   if(pc.lod[i]<2)for(unsigned k=0;k<2;++k)items+=ram_at(before,a+0x78+0x48*k,1)[0]==1?1u:0u;
   if(slerp&&(ram32(before,a+0x14)&0x4000u)){++flagged;uint32_t m=ram32(before,a+0x4b8),clst=ram32(before,m+12),rows=ram32(before,clst),count=ram32(before,clst+4);
    for(unsigned j=1;j<count;++j){const uint8_t *row=ram_at(before,rows+4*j,4);unsigned weight=(unsigned)row[2]<<8|row[3];
     if(ram_at(before,SLERP_ENABLES+row[0],1)[0])++enabled_rows;else ++disabled_rows;zero_weights+=weight==0;unit_weights+=weight==4096;}}}
  if(!same(ram,cache)){fprintf(stderr,"original actor palettes mismatch case %u\n",index);return 7;}
  matrices+=ram32(ram,R13-0x5670u);digest=fnv(digest,ram,RAM_BYTES);}
 if(slerp){
  if(!flagged||flagged==runs||!enabled_rows||!disabled_rows||!zero_weights||!unit_weights){fprintf(stderr,"incomplete slerp coverage\n");return 9;}
  printf("Original actor slerp: %u runs of 80162760 over 14 actors x %u cases; %u flagged bodies, %u enabled and %u disabled rows, %u zero and %u unit weights; all RAM and locked cache match; %u palette matrices allocated; digest %08x.\n",runs,CASES,flagged,enabled_rows,disabled_rows,zero_weights,unit_weights,matrices,digest);return 0;}
 for(unsigned i=0;i<ACTORS;++i)if(!(gameplay[i]=gameplay_actor(pristine,actor_at(pristine,i)))){fprintf(stderr,"no gameplay actor for visual %u\n",i);return 8;}
 unsigned placed=0,mirrored=0;
 for(unsigned index=0;index<PLACE_CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);prepare_place(ram,index);memcpy(before,ram,RAM_BYTES);if(!place_case(ram)){fprintf(stderr,"case %u\n",index);return 5;}
  memcpy(oracle,before,RAM_BYTES);uint32_t side=ram32(before,R13-0x7078u);mirrored+=side&&ram_at(before,side+0x39,1)[0]==1?1u:0u;
  for(unsigned i=0;i<ACTORS;++i){const uint32_t args[1]={gameplay[i]};if(!run_original(0x800d4a6cu,args,1)){fprintf(stderr,"placement case %u actor %u\n",index,i);return 6;}++placed;}
  memset(cache,0,sizeof cache);memset(locked,0,sizeof locked);
  if(!same(ram,cache)){fprintf(stderr,"original placement mismatch case %u\n",index);return 7;}digest=fnv(digest,ram,RAM_BYTES);}
 printf("Original actor palettes: %u runs of 80162760 over 14 actors x %u cases (LOD 0/1/2/3: %u/%u/%u/%u, %u with a scale vector) "
  "with %u hand and %u held-item palettes match all RAM and the locked cache; %u palette matrices allocated; "
  "%u runs of 800D4A6C over %u cases (%u mirrored) match all RAM; digest %08x.\n",
  runs,CASES,lods[0],lods[1],lods[2],lods[3],scaled,hands,items,matrices,placed,PLACE_CASES,mirrored,digest);
 return 0;}
