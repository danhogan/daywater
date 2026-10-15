/* 800DFBA8 and 800DFB34, separately and paired as at 801641D8, against
   the unhooked original module. All RAM outside the oracle stack, including
   per-actor output vectors and guards, is compared after each batch.
   Case 0 leaves all saved inputs intact (only output scratch is seeded).
   Edited cases supply an otherwise absent scale object, relocate its base,
   edit all consulted fields, seed marker bytes and vector guards, and
   exercise both owner branches, nonboolean enable bytes and full lbz range. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "actor_scale.h"
#define CASES 401u
#define ACTORS 14u
#define VECTOR 0x80f37000u
#define OBJECT0 0x80f33000u
#define OBJECT1 0x80f34000u
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;
static unsigned enabled_runs,disabled_runs,owner_hits,owner_misses,wide_indices;
static uint32_t actor_at(uint8_t *ram,unsigned i){return ram32(ram,ram32(ram,R13-0x7bbcu)+4*i);}
static uint32_t finite_word(void){uint32_t w=(rnd()<<8)|(rnd()&255u);return (w&0x7f800000u)==0x7f800000u?w^0x00800000u:w;}
static void prepare(uint8_t *ram,unsigned index){
 for(unsigned i=0;i<ACTORS*16u;++i)ram_at(ram,VECTOR,ACTORS*16u)[i]=(uint8_t)rnd();
 if(!index)return;
 const uint32_t object=index&1u?OBJECT0:OBJECT1;
 wr32(ram_at(ram,R13-0x7234u,4),object);
 ram_at(ram,R13-0x7238u,1)[0]=index%5u?(uint8_t)(1u+rnd()%255u):0;
 /* Wide-index records overlap the ordinary arrays in the original layout;
    retain that aliasing and prepare both native and oracle identically. */
 for(unsigned i=0;i<0xc00u;++i)ram_at(ram,object,0xc00u)[i]=(uint8_t)rnd();
 for(unsigned i=0;i<263u;++i)wr32(ram_at(ram,object+0x49cu+4*i,4),finite_word());
 for(unsigned i=0;i<263u;++i)ram_at(ram,object+0x4e3u+i,1)[0]=(uint8_t)(2u+rnd()%254u);
 for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor_at(ram,i);unsigned k=index%7u==0?7u+rnd()%249u:rnd()%7u;
  ram_at(ram,a+0x1361u,1)[0]=(uint8_t)k;
  wr32(ram_at(ram,object+0x464u+4*k,4),rnd()%2u?a:actor_at(ram,(i+1u)%ACTORS));}
 /* Aliasing edits above may have produced NaNs. Keep finite binary32 inputs;
    NaN payload/FPSCR behavior is outside this reconstruction. */
 for(unsigned i=0;i<263u;++i){uint8_t *p=ram_at(ram,object+0x49cu+4*i,4);uint32_t w=rd32(p);
  if((w&0x7f800000u)==0x7f800000u)wr32(p,w^0x00800000u);}
 /* The disabled path reads RAM, not a hard-coded 1.0. */
 wr32(ram_at(ram,0x80252d24u,4),finite_word());}
static int run_original(uint32_t pc,uint32_t actor,uint32_t output){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=actor;c.gpr[4]=output;c.pc=pc;
 for(unsigned steps=0;c.pc;++steps){
  if(steps>10000u){fprintf(stderr,"%08x did not return at %08x\n",pc,c.pc);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"%08x exception at %08x\n",pc,c.pc);return 0;}}
 return 1;}
static int native_actor(nflstreet_draw *d,uint32_t actor,uint32_t output,unsigned mode){
 if(mode!=1&&!nflstreet_actor_scale_mark(d,R13,actor))return 0;
 if(mode!=0){float v[3];if(!nflstreet_actor_scale_vector(d,R13,actor,v))return 0;
  /* Original writes z, x, y; serialize the host vector into the RAM output. */
  const unsigned order[3]={2,0,1};for(unsigned i=0;i<3;++i){unsigned k=order[i];uint32_t word;memcpy(&word,v+k,4);wr32(ram_at(d->x->memory,output+4*k,4),word);}}
 return 1;}
static int same(const uint8_t *native){
 const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&oracle[i]!=native[i]){
  fprintf(stderr,"RAM differs at %08x: original %02x native %02x\n",RAM_BASE+i,oracle[i],native[i]);return 0;}
 return 1;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);
 if(!pristine||!ram||!before||!oracle)return 3;
 nflstreet_player_draw_context x;memset(&x,0,sizeof x);x.span=span;x.memory=ram;nflstreet_draw d={&x,NULL};
 uint32_t digest=2166136261u;unsigned runs=0;
 for(unsigned index=0;index<CASES;++index){
  memcpy(before,pristine,RAM_BYTES);prepare(before,index);
  for(unsigned mode=0;mode<3;++mode){
   memcpy(ram,before,RAM_BYTES);memcpy(oracle,before,RAM_BYTES);
   for(unsigned i=0;i<ACTORS;++i){uint32_t a=actor_at(before,i),out=VECTOR+16*i;
    if(!native_actor(&d,a,out,mode)){fprintf(stderr,"native rejected case %u mode %u actor %u\n",index,mode,i);return 5;}
    if((mode!=1&&!run_original(0x800dfba8u,a,out))||(mode!=0&&!run_original(0x800dfb34u,a,out))){fprintf(stderr,"case %u mode %u actor %u\n",index,mode,i);return 6;}
    ++runs;
    if(mode==2){if(ram_at(before,R13-0x7238u,1)[0]){++enabled_runs;
      uint32_t g=ram32(before,R13-0x7234u),k=ram_at(before,a+0x1361u,1)[0];
      /* Use the actual current native owner: earlier markers can alias it. */
      if(ram32(ram,g+0x464u+4*k)==a)++owner_hits;else ++owner_misses;if(k>6)++wide_indices;
     }else ++disabled_runs;}}
   if(!same(ram)){fprintf(stderr,"original actor scale mismatch case %u mode %u\n",index,mode);return 7;}
   /* Stack is deliberately excluded from comparison and the digest. */
   digest=fnv(digest,ram,STACK_LO-RAM_BASE);digest=fnv(digest,ram+(STACK_HI-RAM_BASE),RAM_BYTES-(STACK_HI-RAM_BASE));}}
 if(!enabled_runs||!disabled_runs||!owner_hits||!owner_misses||!wide_indices)return 8;
 printf("Original actor scale: %u batches over 14 actors x %u cases x 3 modes (mark/vector/paired); enabled %u disabled %u owner-hit %u owner-miss %u wide-index %u; all RAM outside oracle stack and every produced vector match; digest %08x.\n",
  runs,CASES,enabled_runs,disabled_runs,owner_hits,owner_misses,wide_indices,digest);
 return 0;}
