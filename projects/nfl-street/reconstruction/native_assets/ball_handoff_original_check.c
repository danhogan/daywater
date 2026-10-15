/* Untouched original DLL, no hooks. Compare every MEM1 byte outside the
   reserved oracle stack after every ownership operation, retain both RAMs
   independently over four updates. Inputs are events, not eligibility. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "ball_handoff.h"
#include "check_fixture.h"
#define CASES 401u
#define UPDATES 4u
static const ModernGekkoModuleDesc *module;
static uint8_t *oracle;
static unsigned comparisons;
static int original(uint32_t pc,uint32_t a,uint32_t b,uint32_t arg){
 CPUState c={0};c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.gpr[1]=STACK_HI-0x40;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=arg;c.pc=pc;
 for(unsigned step=0;c.pc;++step){if(step>2000000){fprintf(stderr,"original timeout at %08x\n",c.pc);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"original exception at %08x\n",c.pc);return 0;}}
 return 1;}
static int same(uint8_t *ram,unsigned index,unsigned step,const char *phase){
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<STACK_LO-RAM_BASE||i>=STACK_HI-RAM_BASE)&&ram[i]!=oracle[i]){
  fprintf(stderr,"RAM differs case %u step %u phase %s at %08x: native %08x original %08x\n",index,step,phase,RAM_BASE+i,ram32(ram,(RAM_BASE+i)&~3u),ram32(oracle,(RAM_BASE+i)&~3u));return 0;}++comparisons;return 1;}
static nflstreet_ball_update_context context(uint8_t *ram,nflstreet_player_draw_context *x){
 memset(x,0,sizeof *x);x->span=span;x->memory=ram;return(nflstreet_ball_update_context){x,venue_stacks(ram),R13};}
static uint32_t actor(uint8_t *ram,unsigned i){uint32_t t=ram32(ram,R13-0x7c30u);return ram32(ram,t)+i*0x14d4u;}
static void setf(uint8_t *ram,uint32_t a,float f){wr32(ram_at(ram,a,4),fbits(f));}
static void prepare(uint8_t *ram,uint32_t ball,unsigned index){
 if(!index)return;uint8_t *r=ram_at(ram,ball,244);static const unsigned old[]={0,1,2,3,5,6,8,9};
 for(unsigned o=4;o<244;o+=4)if(o<12||o>=0xa4)wr32(r+o,rnd());
 wr32(r+0xa4,old[index%8]);uint32_t previous=actor(ram,(index+3)%14);wr32(r+0xb4,index%3?ram32(ram,previous):0);
 for(unsigned i=0;i<3;++i){setf(ram,ball+12+4*i,rndf(-80,80));setf(ram,ball+0x54+4*i,rndf(-4,4));setf(ram,ball+0x7c+4*i,rndf(-10,10));}
 for(unsigned i=0;i<14;++i){uint32_t a=actor(ram,i),m=ram32(ram,a+4);wr32(ram_at(ram,a+12,4),rnd()|0x08000000u);
  for(unsigned j=0;j<3;++j){setf(ram,a+0x28c+16*j,rndf(-40,40));setf(ram,m+0x3b8+16*j,rndf(-40,40));setf(ram,m+0x40+4*j,rndf(-0.5f,0.5f));}
  setf(ram,m+0x18,rndf(0.75f,1.25f));if(index%4==0)nflstreet_wr16(ram_at(ram,m+0x32,2),0xffffu);
  static const int16_t stats[]={-32768,-1,0,149,150,151,32767};nflstreet_wr16(ram_at(ram,a+0xbbc,2),(uint16_t)stats[(index+i)%7]);
  setf(ram,a+0x1c4,rndf(0,12));wr32(ram_at(ram,a+0x1c0,4),rnd()&0xffffffu);ram_at(ram,a+0x180,1)[0]=0xa5;
  memset(ram_at(ram,a+0x150,0x2c),0xa5,0x2c);memset(ram_at(ram,a+0x200,12),0x5a,12);ram_at(ram,a+0x20e,1)[0]=0xa5;
 }
 uint32_t q=0x817e2000u;wr32(ram_at(ram,R13-0x7cf8u,4),q);wr32(ram_at(ram,q,4),index%3==0?3:index%3);
 wr32(ram_at(ram,R13-0x7cf4u,4),0x11223344);wr32(ram_at(ram,R13-0x6a00u,4),0x55667788);
 uint32_t clock=index%3?0x817e2100u:0;wr32(ram_at(ram,R13-0x61f0u,4),clock);
 if(clock){wr32(ram_at(ram,clock,4),index%3==1?37:0xfffffff0u);wr32(ram_at(ram,clock+4,4),index%3==1?119:0x20);}
 uint32_t field=ram32(ram,R13-0x7078u);setf(ram,field+12,rndf(-40,40));setf(ram,field+16,rndf(-40,40));setf(ram,R13-0x70c4u,rndf(0.5f,2));
}
static int rejected(uint8_t *ram,const nflstreet_ball_update_context *c,uint32_t ball){
 uint8_t *before=malloc(RAM_BYTES);if(!before)return 0;memcpy(before,ram,RAM_BYTES);
 int bad=nflstreet_ball_handoff_transition(c,ball,4,0)||nflstreet_ball_handoff_transition(c,ball,5,1)||
 nflstreet_ball_handoff_release(c,ball,0,0,2)||nflstreet_ball_handoff_assign(c,ball,0);
 int good=!bad&&!memcmp(before,ram,RAM_BYTES);free(before);return good;}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t *saved=load_ram(argv[1]),*ram=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);if(!saved||!ram||!oracle)return 3;
 uint32_t pool=ram32(saved,R13-0x73d0u),ball=pool+8+0xf4u*ram_at(saved,pool+1,1)[0],digest=2166136261u;unsigned events=0;
 /* Audited callback table and constants are part of the supported revision.
    Never replace an original callback with a native helper in the oracle. */
 static const uint32_t exits[]={0,0x800c1a50,0,0,0x800c17cc,0,0x800c1ae4,0x800c1a90,0x800c1ae4,0};
 static const uint32_t entries[]={0,0x800c1748,0,0,0x800c1790,0x800c1874};
 for(unsigned i=0;i<10;++i)if(ram32(saved,0x8026f79c+i*28)!=exits[i])return 4;
 for(unsigned i=0;i<6;++i)if(ram32(saved,0x8026f798+i*28)!=entries[i])return 4;
 if(ram32(saved,0x80251fb0u)||ram32(saved,0x802521c0u))return 4;
 for(unsigned index=0;index<CASES;++index){memcpy(ram,saved,RAM_BYTES);prepare(ram,ball,index);memcpy(oracle,ram,RAM_BYTES);
  nflstreet_player_draw_context x;nflstreet_ball_update_context c=context(ram,&x);
  for(unsigned step=0;step<UPDATES;++step){uint32_t a=actor(ram,(index+step)%14),b=actor(ram,(index+step+1)%14);
   /* First operation of case zero starts from byte-for-byte saved MEM1. */
   static const uint32_t states[]={0,1,2,3,5};uint32_t state=states[(index+step)%5],arg=state==5?0:0x12340000u+index+step;unsigned keep=(index+step)&1;
   if(!nflstreet_ball_handoff_transition(&c,ball,state,arg))return 5;
   if(!original(0x800c25fcu,ball,state,arg)||!same(ram,index,step,"transition"))return 6;
   if(!nflstreet_ball_handoff_assign(&c,ball,a)){fprintf(stderr,"native rejected assign %u/%u\n",index,step);return 5;}
   if(!original(0x800be2b4u,ball,a,0)||!same(ram,index,step,"assign"))return 6;
   if(index)for(unsigned machine=0;machine<2;++machine)for(unsigned j=0;j<3;++j){uint8_t *mem=machine?oracle:ram;setf(mem,ball+0x54+4*j,(float)(j+1));setf(mem,ball+0x7c+4*j,-(float)(j+1));}
   if(!nflstreet_ball_handoff_release(&c,ball,state,arg,keep)){fprintf(stderr,"native rejected release\n");return 5;}
   if(!original(keep?0x800be264u:0x800be204u,ball,state,arg)||!same(ram,index,step,"release"))return 6;
   if(!nflstreet_ball_handoff_snap(&c,b)){fprintf(stderr,"native rejected snap %u/%u\n",index,step);return 5;}
   if(!original(0x800815b4u,b,0,0)||!same(ram,index,step,"snap"))return 6;
   uint32_t q=ram32(ram,R13-0x7cf8u);
   if(ram32(ram,q)!=3){if(!nflstreet_ball_handoff_event(&c,ball,a))return 5;
    /* 8007BEE8 ABI: r3 opaque, r4 actor, r5 ball, r6 unused on this path. */
    if(!original(0x8007bee8u,0,a,ball)||!same(ram,index,step,"event"))return 6;++events;}
   if(!rejected(ram,&c,ball))return 5;
   digest=fnv(digest,ram_at(ram,ball,244),244);digest=fnv(digest,ram_at(ram,a,0x2c0),0x2c0);digest=fnv(digest,ram_at(ram,b,0x2c0),0x2c0);
   /* Inputs advance equally; prior ownership/history/scratch survive. */
   if(index)for(unsigned machine=0;machine<2;++machine){uint8_t *mem=machine?oracle:ram;
    for(unsigned j=0;j<3;++j){setf(mem,a+0x28c+16*j,ramf(mem,a+0x28c+16*j)+0.03125f);setf(mem,ball+0x54+4*j,(float)(step+1));setf(mem,ball+0x7c+4*j,-(float)(step+1));}
    uint32_t clock=ram32(mem,R13-0x61f0u);if(clock)wr32(ram_at(mem,clock+4,4),ram32(mem,clock+4)+1);}
  }}
 printf("Original ball handoff: %u cases x %u updates; %u all-RAM comparisons; %u bounded events; assignment, both releases, 800815B4 and history match; no hooks; digest %08x.\n",CASES,UPDATES,comparisons,events,digest);
 free(saved);free(ram);free(oracle);FreeLibrary(dll);return 0;}
