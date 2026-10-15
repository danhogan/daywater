/* Compare native camera slot builders with the original module on identical RAM.
   Each context uses its saved node parameters plus 400 randomized edits per node. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "check_fixture.h"
#include "camera_projection.h"
#define CASES 401u
#define SLOTS 0x80331ec0u
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;
static int run_original(uint32_t pc,uint32_t dest,const float *v,unsigned n){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK_HI-0x100u;c.gpr[2]=0x803b9a40u;c.gpr[3]=dest;c.gpr[13]=R13;
 for(unsigned i=0;i<n;++i)c.fpr[i+1]=(double)v[i];c.pc=pc;
 for(unsigned step=0;c.pc;++step){if(step>1000000u)return 0;module->dispatch(&c,c.pc);if(c.exception)return 0;}return 1;
}
static void slot_diffs(const uint8_t *a,const uint8_t *b,uint32_t slot){
 unsigned shown=0;for(unsigned i=0;i<16&&shown<4;++i){const uint32_t at=slot+4*i,oa=ram32((uint8_t *)a,at),nb=ram32((uint8_t *)b,at);
  if(oa!=nb){fprintf(stderr,"slot %u word %u at %08x: original %08x native %08x\n",(slot-SLOTS)/0x40u,i,at,oa,nb);++shown;}}
}
static int eq(const uint8_t *a,const uint8_t *b,int debug){const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&a[i]!=b[i]){fprintf(stderr,"RAM differs at %08x: %02x / %02x\n",RAM_BASE+i,a[i],b[i]);
  if(debug){slot_diffs(a,b,SLOTS+0x40);slot_diffs(a,b,SLOTS+0x80);}return 0;}return 1;}
static uint32_t node_at(uint8_t *ram,unsigned i){static const uint32_t n[]={0x806a1ca8u,0x806a1db0u,0x806a1ba0u};(void)ram;return n[i];}
int main(int argc,char **argv){
 if(argc!=3&&argc!=4)return 2;const int debug=argc==4;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);if(!pristine||!ram||!before||!oracle)return 3;
 unsigned cases=0;for(unsigned ni=0;ni<3;++ni)for(unsigned ci=0;ci<CASES;++ci){memcpy(ram,pristine,RAM_BYTES);const uint32_t node=node_at(ram,ni);
  float p[11];static const uint32_t off[]={4,8,0x14,0x18,0xd8,0xdc,0xe4,0xe0,0xe8,0xec};
  for(unsigned j=0;j<10;++j)p[j]=ramf(ram,node+off[j]);p[10]=ci?rndf(-3.0f,3.0f):0.0f;
  if(ci){for(unsigned j=0;j<10;++j)if(rnd()%3==0){p[j]=j<4?rndf(-500.0f,500.0f):rndf(-20.0f,20.0f);wr32(ram_at(ram,node+off[j],4),fbits(p[j]));}}
  if(ci&&rnd()%2){uint32_t flags=ram32(ram,node+0x1c);wr32(ram_at(ram,node+0x1c,4),flags^4u);}memcpy(before,ram,RAM_BYTES);
  nflstreet_player_draw_context x;memset(&x,0,sizeof x);x.span=span;x.memory=ram;nflstreet_view_context c;memset(&c,0,sizeof c);c.x=&x;c.r13=R13;c.slots=SLOTS;
  if(!nflstreet_camera_projection(&c,node,p[10]))return 5;
  memcpy(oracle,before,RAM_BYTES);float a[5]={p[0],p[1],p[2],p[3],p[10]};if(!run_original(0x801df8c4u,SLOTS+0x40,a,5))return 6;
  if(ram32(before,node+0x1c)&4u){float b[6]={p[4],p[5],p[6],p[7],-p[8],-p[9]};if(!run_original(0x801df970u,SLOTS+0x80,b,6))return 6;}
  else{float b[6]={p[4],p[5],p[6],p[7],p[8],p[9]};if(!run_original(0x801df938u,SLOTS+0x80,b,6))return 6;}
  if(!eq(ram,oracle,debug))return 7;++cases;
 }
 printf("Camera projection builders: %u node cases (saved plus 400 edited per node) match all RAM.\n",cases);return 0;
}
