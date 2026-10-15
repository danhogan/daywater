#include <windows.h>
#include "moderngekko/module_abi.h"
#include "check_fixture.h"
#include "actor_dynamics.h"
#define ACTORS 14u
static const ModernGekkoModuleDesc *module;
static void setup(uint8_t *ram,CPUState *c){memset(c,0,sizeof *c);c->ram=ram;c->ram_size=RAM_BYTES;c->msr=0x2000;c->hid2=0xe0000000u;c->gqr[6]=0x0f070007;c->gpr[1]=STACK_HI-0x40u;c->gpr[2]=0x803b9a40u;c->gpr[13]=R13;}
static int run(CPUState *c){for(unsigned n=0;c->pc;++n){if(n>10000000u||c->exception)return 0;c->downcount=-1000000000;module->dispatch(c,c->pc);}return 1;}
static uint8_t *span0(void *m,uint32_t a,uint32_t n){return ram_at(m,a,n);}
static uint32_t actor(uint8_t *r,unsigned i){return ram32(r,ram32(r,R13-0x7bbcu)+4*i);}
static int equal(const uint8_t *a,const uint8_t *b){uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE,s=ram32((uint8_t *)a,R13-0x65e8u),slots=ram32((uint8_t *)a,s+8)-RAM_BASE;for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&(i<slots||i>=slots+0x800u)&&a[i]!=b[i]){fprintf(stderr,"difference %08x %02x %02x\n",RAM_BASE+i,a[i],b[i]);return 0;}return 1;}
static int native(uint8_t *ram,uint32_t a,uint32_t s,int on){nflstreet_player_draw_context x={0};x.memory=ram;x.span=span0;nflstreet_actor_dynamics_context c={&x,venue_stacks(ram)};return nflstreet_actor_hand_dynamics(&c,a,s,on);}
static int prepare(uint8_t *ram,uint32_t a){nflstreet_player_draw_context x={0};x.memory=ram;x.span=span0;nflstreet_matrix_stacks k=venue_stacks(ram);static nflstreet_render_state unused;nflstreet_draw d={&x,&unused};return nflstreet_matrix_push(&d,&k)&&nflstreet_matrix_load(&d,&k,a+0x3ac);}
int main(int argc,char **argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[2]);ModernGekkoGetModuleFn get=h?(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL):NULL;if(!get||!(module=get()))return 3;uint8_t *base=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*oracle=malloc(RAM_BYTES);if(!base||!ram||!oracle)return 3;CPUState c;unsigned runs=0;
 for(unsigned test=0;test<101;++test)for(unsigned i=0;i<ACTORS;++i)for(unsigned hand=0;hand<2;++hand){memcpy(ram,base,RAM_BYTES);uint32_t a=actor(ram,i),s=a+0x11cu+0x7cu*hand;if(test){uint8_t *p=ram_at(ram,a,0x400);for(unsigned w=0;w<12;++w)if(rnd()%2)wr32(p+0x3ac+4*w,fbits(rndf(-2.0f,2.0f)));p[(0x11c+0x7c*hand)+2]=0;}memcpy(oracle,ram,RAM_BYTES);if(!prepare(ram,a)||!prepare(oracle,a)||!native(ram,a,s,test%3!=0))return 4;setup(oracle,&c);c.gpr[3]=a;c.gpr[4]=s;c.gpr[5]=test%3!=0;c.pc=0x8004bf1cu;if(!run(&c)){fprintf(stderr,"original failed\n");return 5;}if(!equal(ram,oracle)){fprintf(stderr,"case %u actor %u hand %u\n",test,i,hand);return 6;}++runs;}
 printf("Original hand dynamics: %u push/load/update/pop runs match all RAM outside the oracle stack.\n",runs);return 0;}
