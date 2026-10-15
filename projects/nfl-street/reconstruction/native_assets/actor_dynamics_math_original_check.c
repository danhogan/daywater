/* Bit-exact leaf oracle for the first dynamics helpers: 80188D98 and
   8004BED8.  The other declarations in actor_dynamics_math.h are staged but
   deliberately not claimed by this check yet. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "check_fixture.h"
#include "venue_fixture.h"
#include "actor_dynamics_math.h"
#define SCRATCH 0x80f37000u
static const ModernGekkoModuleDesc *module;
static void setup(uint8_t *ram,uint32_t pc,CPUState *c){memset(c,0,sizeof *c);c->ram=ram;c->ram_size=RAM_BYTES;c->msr=0x2000;c->hid2=0xe0000000u;c->gqr[6]=0x0f070007;c->gpr[1]=STACK_HI-0x40u;c->gpr[2]=0x803b9a40u;c->gpr[13]=R13;c->pc=pc;}
static int call(CPUState *c){
 for(unsigned steps=0;c->pc;++steps){if(steps>1000000u||c->exception)return 0;c->downcount=-1000000000;module->dispatch(c,c->pc);}return 1;
}
static uint8_t *test_span(void *memory,uint32_t a,uint32_t n){return ram_at(memory,a,n);}
static float bits(uint32_t x){float v;memcpy(&v,&x,4);return v;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);ModernGekkoGetModuleFn get=h?(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL):NULL;if(!get||!(module=get()))return 3;
 uint8_t *ram=load_ram(argv[1]),*oracle=load_ram(argv[1]);if(!ram||!oracle)return 3;nflstreet_player_draw_context x={0};x.memory=ram;x.span=test_span;static nflstreet_render_state state; nflstreet_draw d={&x,&state};CPUState c;
 unsigned atan_cases=0,clamp_cases=0,vector_cases=0,matrix_cases=0;for(unsigned i=0;i<20003;++i){uint32_t a=rnd(),b=rnd();a=(a&0x807fffffu)|((2u+rnd()%250u)<<23);b=(b&0x807fffffu)|((2u+rnd()%250u)<<23);float y=bits(a),xx=bits(b);if(y==0.0f&&xx==0.0f)xx=1.0f;
  setup(oracle,0x80188d98u,&c);c.fpr[1]=y;c.fpr[2]=xx;if(!call(&c))return 4;uint32_t native=nflstreet_dynamics_atan(&d,y,xx);if(c.gpr[3]!=native){fprintf(stderr,"atan %08x/%08x: %08x != %08x\n",a,b,c.gpr[3],native);return 5;}++atan_cases;
  int32_t lo=(int32_t)rnd(),hi=(int32_t)rnd(),offset=(int32_t)rnd(),value=(int32_t)rnd();if(lo>hi){int32_t t=lo;lo=hi;hi=t;}wr32(ram_at(ram,SCRATCH,8),(uint32_t)lo);wr32(ram_at(ram,SCRATCH+4,4),(uint32_t)hi);wr32(ram_at(ram,SCRATCH+8,4),(uint32_t)value);memcpy(oracle+(SCRATCH-RAM_BASE),ram+(SCRATCH-RAM_BASE),12);
  setup(oracle,0x8004bed8u,&c);c.gpr[3]=SCRATCH;c.gpr[4]=(uint32_t)offset;c.gpr[5]=SCRATCH+8;if(!call(&c))return 4;int changed=nflstreet_dynamics_clamp(&d,SCRATCH,offset,SCRATCH+8);if(c.gpr[3]!=(uint32_t)changed||ram32(oracle,SCRATCH+8)!=ram32(ram,SCRATCH+8)){fprintf(stderr,"clamp %d %d %d %d: %08x/%08x %u/%d\n",lo,hi,offset,value,ram32(oracle,SCRATCH+8),ram32(ram,SCRATCH+8),c.gpr[3],changed);return 6;}++clamp_cases;
  float va[3],vb[3],vo[3];for(unsigned j=0;j<3;++j){va[j]=rndf(-100.0f,100.0f);vb[j]=rndf(-100.0f,100.0f);wr32(ram_at(ram,SCRATCH+0x20+4*j,4),fbits(va[j]));wr32(ram_at(ram,SCRATCH+0x30+4*j,4),fbits(vb[j]));}memcpy(oracle+(SCRATCH+0x20-RAM_BASE),ram+(SCRATCH+0x20-RAM_BASE),0x20);
  setup(oracle,0x801de5dcu,&c);c.gpr[3]=SCRATCH+0x40;c.gpr[4]=SCRATCH+0x20;c.gpr[5]=SCRATCH+0x30;if(!call(&c))return 7;nflstreet_dynamics_sub(vo,va,vb);for(unsigned j=0;j<3;++j)if(ram32(oracle,SCRATCH+0x40+4*j)!=fbits(vo[j])){fprintf(stderr,"sub mismatch\n");return 8;}
  setup(oracle,0x801de62cu,&c);c.gpr[3]=SCRATCH+0x20;c.gpr[4]=SCRATCH+0x30;if(!call(&c))return 7;if(fbits((float)c.fpr[1])!=fbits(nflstreet_dynamics_dot(va,vb))){fprintf(stderr,"dot mismatch\n");return 8;}
  setup(oracle,0x801ddffcu,&c);c.gpr[3]=SCRATCH+0x20;if(!call(&c))return 7;if(fbits((float)c.fpr[1])!=fbits(nflstreet_dynamics_length(va))){fprintf(stderr,"length mismatch %08x %08x\n",fbits((float)c.fpr[1]),fbits(nflstreet_dynamics_length(va)));return 8;}
  setup(oracle,0x801de118u,&c);c.gpr[3]=SCRATCH+0x50;c.gpr[4]=SCRATCH+0x20;if(!call(&c))return 7;if(!nflstreet_dynamics_normalize(&d,vo,va))return 8;for(unsigned j=0;j<3;++j)if(ram32(oracle,SCRATCH+0x50+4*j)!=fbits(vo[j])){fprintf(stderr,"normalize mismatch %u %08x %08x\n",j,ram32(oracle,SCRATCH+0x50+4*j),fbits(vo[j]));return 8;}++vector_cases;
  nflstreet_matrix_stacks k=venue_stacks(ram);uint32_t stack=SCRATCH+0x100,slot=SCRATCH+0x140;wr32(ram_at(ram,k.current,4),stack);wr32(ram_at(ram,stack,4),1);wr32(ram_at(ram,stack+4,4),0);wr32(ram_at(ram,stack+8,4),slot);for(unsigned j=0;j<12;++j)wr32(ram_at(ram,slot+4*j,4),fbits(rndf(-3.0f,3.0f)));for(unsigned j=0;j<3;++j)wr32(ram_at(ram,slot+12+16*j,4),fbits(rndf(-50.0f,50.0f)));memcpy(oracle+(k.current-RAM_BASE),ram+(k.current-RAM_BASE),4);memcpy(oracle+(stack-RAM_BASE),ram+(stack-RAM_BASE),12);memcpy(oracle+(slot-RAM_BASE),ram+(slot-RAM_BASE),0x30);
  setup(oracle,0x80189f10u,&c);c.gpr[3]=SCRATCH+0x190;if(!call(&c))return 9;float tr[3];if(!nflstreet_dynamics_top_translation(&d,&k,tr))return 9;for(unsigned j=0;j<3;++j)if(ram32(oracle,SCRATCH+0x190+4*j)!=fbits(tr[j])){fprintf(stderr,"top mismatch\n");return 10;}
  setup(oracle,0x801deb54u,&c);c.gpr[3]=SCRATCH+0x1a0;c.gpr[4]=SCRATCH+0x20;if(!call(&c))return 9;if(!nflstreet_dynamics_rotate(&d,&k,vo,va))return 9;for(unsigned j=0;j<3;++j)if(ram32(oracle,SCRATCH+0x1a0+4*j)!=fbits(vo[j])){fprintf(stderr,"rotate mismatch\n");return 10;}
  setup(oracle,0x80189f54u,&c);c.gpr[3]=SCRATCH+0x1b0;c.gpr[4]=slot;if(!call(&c))return 9;if(!nflstreet_dynamics_angles(&d,&k,SCRATCH+0x1c0))return 9;for(unsigned j=0;j<3;++j)if(ram32(oracle,SCRATCH+0x1b0+4*j)!=ram32(ram,SCRATCH+0x1c0+4*j)){fprintf(stderr,"angles mismatch %u %08x %08x\n",j,ram32(oracle,SCRATCH+0x1b0+4*j),ram32(ram,SCRATCH+0x1c0+4*j));return 10;}++matrix_cases;
 }
 printf("Dynamics math leaves: %u 80188D98 atan, %u 8004BED8 clamp, %u vector-kernel, and %u matrix-kernel cases match bit-exactly.\n",atan_cases,clamp_cases,vector_cases,matrix_cases);return 0;
}
