/* Full-record original lifecycle oracle; supplied state choices are not gameplay policy. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../ball/ball_state.h"
#include "../ball/ball_lifecycle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i)m->dispatch(c,c->pc);return !c->pc&&!c->exception;}
int main(int argc,char**argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 unsigned root=be(ram+0x3a2670),at=root-0x80000000+8;
 if(root<0x80000000||at>24*1024*1024-244)return 4;
 uint8_t seed[244],native[244],pool[252];memcpy(seed,ram+at,sizeof seed);
 unsigned header=be(ram+0x3c19c0),old_states[]={0,1,2,3,5,6,8,9},checks=0,joins=0,samples=0;
 /* Fail if the source callback table differs from the audited revision. */
 unsigned entries[]={0,0x800c1748,0,0},exits[]={0,0x800c1a50,0,0,0x800c17cc,0,0x800c1ae4,0x800c1a90,0x800c1ae4,0};
 for(unsigned i=0;i<10;++i){if(be(ram+0x26f79c+i*28)!=exits[i])return 4;if(i<4&&be(ram+0x26f798+i*28)!=entries[i])return 4;}
 if(be(ram+0x251fa8)||be(ram+0x251fac)||be(ram+0x2521c0))return 4;
 for(unsigned clock=0;clock<3;++clock)for(unsigned o=0;o<8;++o)for(unsigned state=0;state<4;++state)for(unsigned op=0;op<3;++op){
  memcpy(native,seed,244);put(native+0xa4,old_states[o]);put(native+0xa8,0xfedcba98);put(native+8,0xffffffff);
  put(native+0xb4,header);put(native+0xcc,0xa5a5a5a5);put(native+0xbc,0xc0c0c0c0);put(native+0xe0,0x3f800000);
  for(unsigned i=0;i<3;++i){put(native+0x54+i*4,0x3b000000+i);put(native+0x7c+i*4,0x40000000+i);}
  memcpy(ram+at,native,244);uint32_t elapsed=0;
  if(clock){put(ram+0x3a3850,0x817e0000);unsigned start=clock==1?37:0xfffffff0,end=clock==1?119:0x20;
   put(ram+0x17e0000,start);put(ram+0x17e0004,end);elapsed=end-start;
  }else put(ram+0x3a3850,0);
  CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
  unsigned argument=0x12345678;int ok=op?nflstreet_ball_release(native,244,state,argument,elapsed,op==1):nflstreet_ball_transition(native,244,state,argument,elapsed);
  if(!ok)return 5;c.gpr[3]=root+8;c.gpr[4]=state;c.gpr[5]=argument;
  if(!call(m,&c,op==0?0x800c25fc:op==1?0x800be204:0x800be264))return 6;
  if(memcmp(native,ram+at,244)){for(unsigned i=0;i<244;++i)if(native[i]!=ram[at+i])fprintf(stderr,"Mismatch clock=%u old=%u new=%u op=%u byte=%02x native=%02x original=%02x\n",clock,old_states[o],state,op,i,native[i],ram[at+i]);return 7;}++checks;
  memset(pool,0,8);pool[0]=1;memcpy(pool+8,native,244);unsigned found=99,index=99;
  if(!nflstreet_ball_find_actor(pool,sizeof pool,header,&found,&index)||found!=(op==0)||index)return 8;
  c.gpr[3]=0x803c19c0;if(!call(m,&c,0x800be4f8)||c.gpr[3]!=(found?root+8:0))return 8;++joins;
  nflstreet_ball_trajectory trajectory;if(!nflstreet_ball_read_trajectory(native,244,&trajectory))return 11;
  for(unsigned plane_case=0;plane_case<2;++plane_case){
   double plane=trajectory.position[2]+(plane_case?1.0:-1.0);float time,xy[2];
   if(!nflstreet_ball_sample_plane(&trajectory,plane,&time,xy))return 11;
   c.gpr[3]=root+8;c.gpr[4]=0x817f0000;c.gpr[5]=0x817f0004;c.fpr[1]=plane;
   if(!call(m,&c,0x800c11dc))return 11;
   float output[3]={time,xy[0],xy[1]};for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,output+i,4);
    if(word!=be(ram+0x17f0000+i*4)){fprintf(stderr,"Lifecycle sampler mismatch %u/%u/%u/%u/%u\n",clock,o,state,op,i);return 12;}}
   ++samples;
  }
 }
 /* Original captured record, with no controlled state/flag overrides. */
 memcpy(native,seed,244);memcpy(ram+at,seed,244);put(ram+0x3a3850,0);
 if(!nflstreet_ball_release(native,244,0,0,0,1))return 9;
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=root+8;
 if(!call(m,&c,0x800be204)||memcmp(native,ram+at,244))return 9;++checks;
 for(unsigned invalid=0;invalid<4;++invalid){memcpy(native,seed,244);if(invalid<2)put(native+0xa4,invalid?7:4);
  uint8_t before[244];memcpy(before,native,244);int accepted=nflstreet_ball_release(native,invalid==2?243:244,invalid==3?4:1,0,0,1);
  if(accepted||memcmp(before,native,244))return 10;}
 uint8_t before[244];memcpy(native,seed,244);memcpy(before,native,244);
 if(nflstreet_ball_release(native,244,1,0,0,2)||memcmp(before,native,244))return 10;
 nflstreet_ball_trajectory sentinel;memset(&sentinel,0xa5,sizeof sentinel);nflstreet_ball_trajectory saved=sentinel;
 put(native+0x70,0x7fc00000);
 if(nflstreet_ball_read_trajectory(native,244,&sentinel)||memcmp(&sentinel,&saved,sizeof sentinel))return 10;
 if(nflstreet_ball_read_trajectory(seed,243,&sentinel)||memcmp(&sentinel,&saved,sizeof sentinel))return 10;
 printf("%u complete original ball records exact, %u lifecycle-to-association joins exact, %u joined plane samples exact; null/ordinary/wrapping clock, state history, both releases and unsupported rollback pass\n",checks,joins,samples);
 free(ram);FreeLibrary(h);return 0;
}
