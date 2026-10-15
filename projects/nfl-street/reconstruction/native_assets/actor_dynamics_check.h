#ifndef NFLSTREET_ACTOR_DYNAMICS_CHECK_H
#define NFLSTREET_ACTOR_DYNAMICS_CHECK_H
/* Check-only runner. No original entry is hooked. Both implementations run
   the caller's push/load(actor+3AC)/update/pop and the internal bone push.
   Only the oracle CPU stack is excluded; all matrix-stack slots compare. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "check_fixture.h"
#include "actor_dynamics.h"
#define DYNAMICS_CASES 301u
#define DYNAMICS_UPDATES 4u
#define DYNAMICS_ACTORS 14u
static const ModernGekkoModuleDesc *dynamics_module;
static unsigned check_case,check_step,check_actor,check_part,check_enabled;
static uint32_t check_state;
static int original(uint8_t *ram,uint32_t pc,uint32_t a,uint32_t b,uint32_t e){
 CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=e;c.pc=pc;
 for(unsigned step=0;c.pc;++step){if(step>1000000u||c.exception){fprintf(stderr,"original entry %08x pc %08x exception %u\n",pc,c.pc,c.exception);return 0;}
  c.downcount=-1000000000;dynamics_module->dispatch(&c,c.pc);}return 1;}
static uint32_t dynamics_actor(uint8_t *ram,unsigned i){return ram32(ram,ram32(ram,R13-0x7bbcu)+4*i);}
static int same_ram(uint8_t *ram,uint8_t *oracle){
 const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 if(!memcmp(ram,oracle,lo)&&!memcmp(ram+hi,oracle+hi,RAM_BYTES-hi))return 1;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&ram[i]!=oracle[i]){fprintf(stderr,
  "RAM differs %08x native %02x original %02x; case %u step %u actor %u part %u enabled %u state %08x\n",
  RAM_BASE+i,ram[i],oracle[i],check_case,check_step,check_actor,check_part,check_enabled,check_state);break;}return 0;}
static void edit_float(uint8_t *ram,uint32_t a,float lo,float hi){wr32(ram_at(ram,a,4),fbits(rndf(lo,hi)));}
static void edit_matrix(uint8_t *ram,uint32_t a){for(unsigned w=0;w<12;++w)edit_float(ram,a+4*w,w%4==3?-30.0f:-1.5f,w%4==3?30.0f:1.5f);}
static int quiet_state(uint8_t *ram,uint32_t a,uint32_t state,unsigned part){
 /* A low-acceleration path keeps spring updates below saturation and joint
    limits. It exposes damping and force errors that random extremes hide. */
 nflstreet_player_draw_context x={0};x.memory=ram;x.span=span;nflstreet_draw d={&x,NULL};nflstreet_matrix_stacks k=venue_stacks(ram);float pos[3];
 float unit[2]={ramf(ram,0x803a3958u),ramf(ram,0x803a395cu)};
 uint32_t bone=ram32(ram,a+0x60)+0x40u*ram_at(ram,state+1,1)[0];
 if(!nflstreet_matrix_push(&d,&k)||!nflstreet_matrix_load(&d,&k,a+0x3ac)||!nflstreet_matrix_concat(&d,&k,bone,unit)||
  !nflstreet_dynamics_top_translation(&d,&k,pos)||!nflstreet_matrix_pop(&d,&k))return 0;
 ram_at(ram,state+2,1)[0]=1;for(unsigned i=0;i<3;++i){wr32(ram_at(ram,state+4+4*i,4),fbits(pos[i]));wr32(ram_at(ram,state+0x10+4*i,4),fbits(0.0f));}
 unsigned joints=part==2?9:1;for(unsigned j=0;j<joints;++j)for(unsigned axis=0;axis<3;++axis){
  uint32_t table=part==2?0x8026d91cu+24*j+8*axis:(int16_t)nflstreet_rd16(ram_at(ram,ram32(ram,state+0x78)+0x41a,2))<1?0x8026da34u:0x8026da1cu;
  int32_t lo=(int32_t)ram32(ram,table),hi=(int32_t)ram32(ram,table+4);uint32_t middle=(uint32_t)((int64_t)lo+((int64_t)hi-lo)/2);
  wr32(ram_at(ram,state+0x28+12*j+4*axis,4),middle);wr32(ram_at(ram,state+(part==2?0x94:0x34)+12*j+4*axis,4),1024u+axis*256u);
 }
 return 1;
}
static void edit_state(uint8_t *ram,uint32_t state,unsigned part,unsigned index){
 uint8_t *p=ram_at(ram,state,part==2?0x13c:0x7c);p[2]=(uint8_t)(index%3==0?0:index%3==1?1:255);
 for(unsigned w=0;w<6;++w)edit_float(ram,state+4+4*w,-10.0f,10.0f);
 /* Stored Euler words are not floats. They also seed the head's look axes. */
 for(unsigned w=0;w<3;++w)wr32(p+0x1c+4*w,(uint32_t)((int32_t)(rnd()%0x1000000u)-0x800000));
 unsigned joints=part==2?9:1;uint32_t velocity=part==2?0x94:0x34;
 for(unsigned j=0;j<joints;++j)for(unsigned axis=0;axis<3;++axis){
  uint32_t table=part==2?0x8026d91cu+24*j+8*axis:0x8026da1cu;
  int32_t low=(int32_t)ram32(ram,table),high=(int32_t)ram32(ram,table+4);
  /* Deliberate below/inside/above limits, not just saved states which can
     already contain the value a faulty implementation would write. */
  int32_t value=index%4==0?low-0x10000:index%4==1?high+0x10000:(int32_t)(rnd()%0x800001u)-0x400000;
  wr32(p+0x28+12*j+4*axis,(uint32_t)value);wr32(p+velocity+12*j+4*axis,(uint32_t)((int32_t)(rnd()%0x200001u)-0x100000));
 }
 uint32_t rest=ram32(ram,state+(part==2?0x138:0x78));
 for(unsigned j=0;j<joints;++j)for(unsigned axis=0;axis<3;++axis){uint8_t *q=ram_at(ram,rest+(part==2?0x41c+12*j:0x416)+2*axis,2);
  nflstreet_wr16(q,(uint16_t)rnd());}
 if(part!=2){/* Both hand limit/scale branches, with zero tested separately. */
  const int16_t signs[3]={-1200,0,1200};nflstreet_wr16(ram_at(ram,rest+0x41a,2),(uint16_t)signs[(index+part)%3]);
  uint32_t pose=ram32(ram,state+0x70);for(unsigned w=0;w<3;++w)nflstreet_wr16(ram_at(ram,pose+6+2*w,2),(uint16_t)rnd());
 }else{uint32_t pose=ram32(ram,state+0x130);for(unsigned j=0;j<9;++j)for(unsigned w=0;w<3;++w)nflstreet_wr16(ram_at(ram,pose+12+12*j+2*w,2),(uint16_t)rnd());}
}
static int prepare_case(uint8_t *ram,unsigned index,unsigned mode){
 nflstreet_matrix_stacks k=venue_stacks(ram);nflstreet_player_draw_context x={0};x.memory=ram;x.span=span;nflstreet_draw d={&x,NULL};
 if(!nflstreet_matrix_select(&d,&k))return 0;if(!index)return 1;
 uint32_t top;if(!nflstreet_matrix_top(&d,&k,&top))return 0;edit_matrix(ram,top);/* saved camera */
 /* Read the executable's tables unchanged; edit their state/velocity/rest
    inputs on both sides of every range boundary. */
 for(unsigned i=0;i<DYNAMICS_ACTORS;++i){uint32_t a=dynamics_actor(ram,i);edit_matrix(ram,a+0x3ac);
  for(unsigned part=0;part<3;++part){if((mode==1&&part==2)||(mode==2&&part!=2))continue;
   uint32_t state=a+(part==2?0x214u:0x11cu+0x7cu*part),bone=ram32(ram,a+0x60)+0x40u*ram_at(ram,state+1,1)[0];
   edit_matrix(ram,bone);edit_state(ram,state,part,index+i);
   if(index%5==2&&!quiet_state(ram,a,state,part))return 0;
  }
 }
 return 1;
}
static int update_native(uint8_t *ram,uint32_t a,uint32_t state,unsigned part,int on){
 nflstreet_player_draw_context x={0};x.memory=ram;x.span=span;nflstreet_actor_dynamics_context c={&x,venue_stacks(ram)};nflstreet_draw d={&x,NULL};
 return nflstreet_matrix_push(&d,&c.stacks)&&nflstreet_matrix_load(&d,&c.stacks,a+0x3ac)&&
  (part==2?nflstreet_actor_head_dynamics(&c,a,state,on):nflstreet_actor_hand_dynamics(&c,a,state,on))&&nflstreet_matrix_pop(&d,&c.stacks);
}
static int update_original(uint8_t *ram,uint32_t a,uint32_t state,unsigned part,int on){
 return original(ram,0x8018941cu,0,0,0)&&original(ram,0x8018962cu,a+0x3ac,0,0)&&
  original(ram,part==2?0x8004c268u:0x8004bf1cu,a,state,(uint32_t)on)&&original(ram,0x8018949cu,0,0,0);
}
static int dynamics_check_main(int argc,char **argv,unsigned mode){
 if(argc<3||argc>5){fprintf(stderr,"usage: check.exe RAM DLL [cases=301 [updates=4]]\n");return 2;}
 unsigned cases=argc>3?(unsigned)strtoul(argv[3],NULL,10):DYNAMICS_CASES,updates=argc>4?(unsigned)strtoul(argv[4],NULL,10):DYNAMICS_UPDATES;
 if(!cases||cases>10000||!updates||updates>100)return 2;
 HMODULE h=LoadLibraryA(argv[2]);ModernGekkoGetModuleFn get=h?(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL):NULL;
 if(!get||!(dynamics_module=get())){fprintf(stderr,"cannot load original module\n");return 3;}
 uint8_t *base=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*oracle=malloc(RAM_BYTES);if(!base||!ram||!oracle)return 3;
 unsigned hands=0,heads=0,comparisons=0;uint32_t digest=2166136261u;
 for(check_case=0;check_case<cases;++check_case)for(unsigned on=0;on<2;++on){
  memcpy(ram,base,RAM_BYTES);if(!prepare_case(ram,check_case,mode))return 4;memcpy(oracle,ram,RAM_BYTES);
  for(check_step=0;check_step<updates;++check_step){
   /* Step 0 preserves the prepared world. Later steps move it so a repeated
      call exercises stored position AND the previous velocity. */
   if(check_step)for(unsigned i=0;i<DYNAMICS_ACTORS;++i){uint32_t a=dynamics_actor(ram,i);for(unsigned axis=0;axis<3;++axis){uint32_t address=a+0x3ac+12+16*axis;
    uint32_t value=fbits(ramf(ram,address)+rndf(-0.25f,0.25f));wr32(ram_at(ram,address,4),value);wr32(ram_at(oracle,address,4),value);}}
   check_enabled=on;/* An additional enabled -> disabled -> enabled transition. */
   if(on&&check_case&&check_case%7==0&&check_step==2)check_enabled=0;
   for(check_actor=0;check_actor<DYNAMICS_ACTORS;++check_actor){uint32_t a=dynamics_actor(ram,check_actor);
    for(check_part=0;check_part<3;++check_part){if((mode==1&&check_part==2)||(mode==2&&check_part!=2))continue;
     check_state=a+(check_part==2?0x214u:0x11cu+0x7cu*check_part);
     if(!update_native(ram,a,check_state,check_part,(int)check_enabled)){fprintf(stderr,"native rejected case %u step %u actor %u part %u\n",check_case,check_step,check_actor,check_part);return 4;}
     if(!update_original(oracle,a,check_state,check_part,(int)check_enabled))return 5;
     if(!same_ram(ram,oracle))return 6;++comparisons;
     if(check_part==2)++heads;else ++hands;
    }
   }
   /* Each routine already compared all RAM before another part or actor
      can overwrite a difference. Retain a deterministic state digest. */
   for(unsigned i=0;i<DYNAMICS_ACTORS;++i){uint32_t a=dynamics_actor(ram,i);digest=fnv(digest,ram_at(ram,a+0x11c,0x238),0x238);}
  }
  if(check_case%25==0&&!on){fprintf(stderr,"case %u/%u passed\n",check_case,cases);fflush(stderr);}
 }
 printf("Original actor dynamics: %u hand and %u head updates; 14 actors x %u cases x 2 enabled flags x %u consecutive updates; %u all-RAM comparisons outside the oracle CPU stack; digest %08x.\n",
  hands,heads,cases,updates,comparisons,digest);free(base);free(ram);free(oracle);FreeLibrary(h);return 0;
}
#endif
