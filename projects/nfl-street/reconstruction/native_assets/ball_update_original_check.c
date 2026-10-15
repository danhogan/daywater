/* No hooks: original 800C0FE4, 800C09D8 and 801A3008. All MEM1 except
   the reserved oracle stack is compared after EACH phase of EACH update. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "ball_update.h"
#include "check_fixture.h"
#define CASES 401u
#define UPDATES 4u
static const ModernGekkoModuleDesc *module;
static uint8_t *oracle;
static int original(uint32_t pc,uint32_t a,uint32_t b,double dt){
 CPUState c={0};c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.gpr[1]=STACK_HI-0x40;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=a;c.gpr[4]=b;c.fpr[1]=dt;c.pc=pc;
 for(unsigned step=0;c.pc;++step){if(step>2000000){fprintf(stderr,"original timeout at %08x\n",c.pc);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"original exception at %08x\n",c.pc);return 0;}}
 return 1;}
static int same(uint8_t *ram,unsigned index,unsigned step,const char *phase){
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<STACK_LO-RAM_BASE||i>=STACK_HI-RAM_BASE)&&ram[i]!=oracle[i]){
  fprintf(stderr,"RAM differs case %u step %u phase %s at %08x: native %08x original %08x\n",index,step,phase,RAM_BASE+i,ram32(ram,(RAM_BASE+i)&~3u),ram32(oracle,(RAM_BASE+i)&~3u));return 0;}return 1;}
static nflstreet_ball_update_context context(uint8_t *ram,nflstreet_player_draw_context *x){
 memset(x,0,sizeof *x);x->span=span;x->memory=ram;return(nflstreet_ball_update_context){x,venue_stacks(ram),R13};}
static uint32_t actor(uint8_t *ram,unsigned i){uint32_t t=ram32(ram,R13-0x7c30u);return ram32(ram,t)+i*0x14d4u;}
static void setf(uint8_t *ram,uint32_t a,float f){wr32(ram_at(ram,a,4),fbits(f));}
static void prepare(uint8_t *ram,uint32_t ball,unsigned index){
 if(!index)return;uint8_t *r=ram_at(ram,ball,244);unsigned mode=index%5;
 /* Opaque fields and flags are randomized too: this routine must leave
    them unchanged, including its state words and ownership history. */
 for(unsigned o=4;o<12;o+=4)wr32(r+o,rnd());for(unsigned o=0xa0;o<0xe0;o+=4)if(o!=0xb4&&o!=0xd0)wr32(r+o,rnd());
 for(unsigned i=0;i<3;++i){setf(ram,ball+12+4*i,rndf(-80,80));setf(ram,ball+0x54+4*i,rndf(-4,4));setf(ram,ball+0x7c+4*i,rndf(-10,10));setf(ram,ball+0x88+4*i,0);}
 memset(r+0x28,0xa5,28);wr32(r+0xd0,0);setf(ram,ball+0xe0,0);setf(ram,ball+0x9c,1);memset(r+0x60,0,16);setf(ram,ball+0x6c,1);
 if(mode>=2){for(unsigned i=0;i<3;++i)setf(ram,ball+0x88+4*i,rndf(-2,2));unsigned ai=(index/5)%14;uint32_t a=actor(ram,ai),m=ram32(ram,a+4);wr32(r+0xb4,ram32(ram,a)^((index&1)?0x5a000000u:0));
  if(mode==4)nflstreet_wr16(ram_at(ram,m+0x32,2),0xffffu);
  for(unsigned i=0;i<3;++i){setf(ram,a+0x280+12+16*i,rndf(-40,40));setf(ram,m+0x3ac+12+16*i,rndf(-40,40));setf(ram,m+0x40+4*i,rndf(-0.5f,0.5f));}
  setf(ram,m+0x18,rndf(0.75f,1.25f));
 }else {wr32(r+0xb4,mode==1?0xabcdef02u:0);for(unsigned i=0;i<4;++i)setf(ram,ball+0x18+4*i,i==3?0.9f:(float)(i+1)*0.1f);}
 uint32_t visual=rd32(r);for(unsigned i=0;i<3;++i)setf(ram,visual+4+4*i,rndf(-100,100));wr32(ram_at(ram,visual+0x294,4),0xaaaaaaaa);
 uint32_t side=ram32(ram,R13-0x7078u);if(side)ram_at(ram,side+0x39,1)[0]=(uint8_t)(index%3);
 setf(ram,R13-0x73b0u,index&1?0.5f:-1.25f);
}
int main(int argc,char **argv){
 if(argc!=3)return 2;HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get||!(module=get())||module->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t *saved=load_ram(argv[1]),*ram=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);if(!saved||!ram||!oracle)return 3;
 uint32_t ball=ram32(saved,R13-0x73d0u)+8,visual=ram32(saved,ball);unsigned attached=0,detached=0;uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index){memcpy(ram,saved,RAM_BYTES);prepare(ram,ball,index);memcpy(oracle,ram,RAM_BYTES);
  nflstreet_player_draw_context x;nflstreet_ball_update_context c=context(ram,&x);
  for(unsigned step=0;step<UPDATES;++step){
   if(index&&step){for(unsigned machine=0;machine<2;++machine){uint8_t *mem=machine?oracle:ram;uint32_t h=ram32(mem,ball+0xb4);
    if((h&255)==1){uint32_t table=ram32(mem,R13-0x7c30u);const uint8_t *t=ram_at(mem,table,12);
     uint32_t a=rd32(t)+(((h>>8)&255)*nflstreet_rd16(t+8)+((h>>16)&255))*0x14d4u,m=ram32(mem,a+4);
     for(unsigned i=0;i<3;++i){setf(mem,a+0x28c+16*i,ramf(mem,a+0x28c+16*i)+0.03125f);setf(mem,m+0x3b8+16*i,ramf(mem,m+0x3b8+16*i)-0.0625f);}}
    else setf(mem,ball+0x84,-9.0f);}}
   /* Step 0 of case 0 uses the saved RAM byte-for-byte. Later updates
      retain BOTH machines' RAM independently, including matrix scratch. */
   static const double dt[4]={1.0,0.0,1.0/60.0,0.5};double delta=dt[step];
   if(!nflstreet_ball_update(&c,ball,delta)){fprintf(stderr,"native rejected case %u step %u\n",index,step);return 5;}
   if(!original(0x800c0fe4u,ball,0,delta)||!same(ram,index,step,"update"))return 6;
   if((ram32(ram,ball+0xb4)&255)==1)++attached;else ++detached;
   if(!nflstreet_ball_update_visual(&c,ball)||!original(0x800c09d8u,ball,0,0)||!same(ram,index,step,"visual"))return 7;
   /* Isolate the exact 801A3008 rotation stores. Translation is an
      explicit host connection, applied equally before this phase. */
   for(unsigned i=0;i<3;++i)wr32(ram_at(oracle,visual+0x2c+16*i,4),ram32(oracle,visual+4+4*i));
   if(!nflstreet_ball_update_matrix(&c,visual)||!original(0x801a3008u,visual+0x20,visual+0x6c,0)||!same(ram,index,step,"matrix"))return 8;
   digest=fnv(digest,ram_at(ram,ball,244),244);digest=fnv(digest,ram_at(ram,visual,0x298),0x298);
  }}
 printf("Original ball update: %u cases x %u updates; %u attached, %u detached; 800C0FE4 + 800C09D8 + 801A3008 match all RAM after each phase; no hooks; digest %08x.\n",CASES,UPDATES,attached,detached,digest);
 free(saved);free(ram);free(oracle);FreeLibrary(dll);return 0;}
