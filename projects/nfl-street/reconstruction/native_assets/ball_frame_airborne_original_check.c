/* Full-record original airborne branch oracle; supplied forces, no launch policy. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../ball/ball_flight.h"
#include "../ball/ball_frame.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void pf(uint8_t*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
static unsigned spatial_calls;static uint32_t query_words[3];
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->exception=0;for(unsigned i=0;c->pc&&i<2000000;++i){if(c->pc==0x801087b8){unsigned at=c->gpr[3]-0x80000000;if(at>24*1024*1024-12)return 0;for(unsigned j=0;j<3;++j)query_words[j]=be(c->ram+at+j*4);++spatial_calls;c->gpr[3]=0x12345678;c->pc=c->lr;continue;}c->downcount=-1000000000;m->dispatch(c,c->pc);if(c->exception)return 0;}return !c->pc;}
static float real(const uint8_t*p){unsigned w=be(p);float f;memcpy(&f,&w,4);return f;}

int main(int argc,char**argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();
 if(m->cpu_state_size!=sizeof(CPUState))return 3;uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 if(be(ram+0x252164)!=0)return 4;unsigned at=be(ram+0x3a2670)-0x80000000+8;if(at>24*1024*1024-244)return 4;uint8_t seed[244],native[244];memcpy(seed,ram+at,244);unsigned checks=0;
 const unsigned addresses[]={0x252074,0x2520c0,0x2520c8,0x2520cc,0x2520d0,0x2520d4,0x2520f0,0x2521ac};
 const unsigned constants[]={0,0x42280000,0x42960000,0xc2280000,0xc2960000,0x41f00000,0x3b5f4ed0,0};
 for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i)if(be(ram+addresses[i])!=constants[i])return 4;
 for(unsigned scenario=0;scenario<8;++scenario)for(unsigned step=0;step<64;++step){
  if(!step){memcpy(native,seed,244);put(native+0xb4,0);put(native+0xd0,0);pf(native+0xe0,0);pf(native+0x9c,1);memset(native+0x60,0,16);pf(native+0x6c,1);memset(native+0x88,0,12);
   pf(native+12,scenario&1?41.9f:-41.9f);pf(native+16,scenario&2?74.9f:-74.9f);pf(native+20,scenario&4?29.9f:2.0f);
   pf(native+0x54,scenario&1?3.25f:-2.75f);pf(native+0x58,scenario&2?1.5f:-4.5f);pf(native+0x5c,scenario&4?2.0f:-1.0f);
   pf(native+0x7c,0.25f);pf(native+0x80,-0.5f);pf(native+0x84,-9.0f);
   pf(native+0x18,0.1f);pf(native+0x1c,-0.2f);pf(native+0x20,0.3f);pf(native+0x24,0.9f);
  }
  double dt=step%4==0?0:step%4==1?1.0/60:step%4==2?0.5:1.0;float plane=scenario&1?0.5f:-1.25f;pf(ram+0x3a2690,plane);memcpy(ram+at,native,244);
  unsigned visual=be(native)-0x80000000;if(visual>24*1024*1024-0x298)return 4;nflstreet_ball_pose previous;for(unsigned i=0;i<3;++i)previous.position[i]=real(ram+visual+4+i*4);for(unsigned i=0;i<4;++i)previous.quaternion[i]=real(ram+visual+0x6c+i*4);nflstreet_ball_frame_output output;
  if(!nflstreet_ball_frame(native,244,dt,plane,NULL,&previous,&output))return 5;spatial_calls=0;
  CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.gpr[3]=at+0x80000000;c.fpr[1]=dt;
  if(!call(m,&c,0x800c0fe4))return 6;
  if(spatial_calls!=1||output.spatial_query!=1||memcmp(output.spatial_query_position,previous.position,12)||be(ram+visual+0x294)!=0x12345678)return 9;
  for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,output.spatial_query_position+i,4);if(word!=query_words[i])return 9;}
  if(memcmp(native,ram+at,244)){for(unsigned i=0;i<244;i+=4)if(be(native+i)!=be(ram+at+i))fprintf(stderr,"Mismatch scenario=%u step=%u offset=%02x native=%08x original=%08x\n",scenario,step,i,be(native+i),be(ram+at+i));return 7;}++checks;
 }
 nflstreet_ball_pose visual_pose={{1,2,3},{0,0,0,1}};nflstreet_ball_frame_output output,saved;memset(&output,0x5a,sizeof output);saved=output;uint8_t last[244],before[244];memcpy(last,native,244);
 for(unsigned invalid=0;invalid<9;++invalid){memcpy(native,last,244);double dt=1,plane=0;size_t bytes=244;
  if(invalid==0)dt=NAN;if(invalid==1)put(native+0xd0,1);if(invalid==2)pf(native+0xe0,1);if(invalid==3)pf(native+0x9c,0);if(invalid==4)put(native+0xb4,0x101);
  if(invalid==5)pf(native+0x7c,NAN);if(invalid==6)pf(native+0x88,1);if(invalid==7)bytes=243;if(invalid==8)plane=0.1;
  memcpy(before,native,244);if(nflstreet_ball_frame(native,bytes,dt,plane,NULL,&visual_pose,&output)||memcmp(native,before,244)||memcmp(&output,&saved,sizeof output))return 8;
 }
 printf("%u complete original detached frame records and retained spatial-query inputs exact across consecutive updates, supplied forces/deltas/planes, field bounds and no-spin orientation; unsupported guards pass\n",checks);free(ram);FreeLibrary(h);return 0;
}
