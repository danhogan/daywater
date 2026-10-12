/* Research oracle. Original module and saved RAM never enter the runtime. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../ball/ball_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(unsigned char*p,unsigned w){p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
static void putfloat(unsigned char*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
static float real(const unsigned char*p){unsigned w=be(p);float f;memcpy(&f,&w,4);return f;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){m->dispatch(c,c->pc);if(c->exception)break;}return !c->pc&&!c->exception;}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;
 unsigned char*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 5;fclose(f);
 unsigned root=be(ram+0x3a2670);if(root<0x80000000||root>0x817fff00)return 6;unsigned p=root&0x1ffffff;size_t size=8+(size_t)ram[p]*0xf4;if(size>24*1024*1024-p)return 6;
 nflstreet_ball_state state;if(!nflstreet_ball_observe(ram+p,size,&state))return 6;
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 if(!call(m,&c,0x800bddc4)||c.gpr[3]!=root+8+state.active_index*0xf4)return 7;unsigned record=c.gpr[3],r=record&0x1ffffff;c.gpr[4]=0x817f0000;
 if(!call(m,&c,0x800be608))return 7;
 for(unsigned i=0;i<3;++i)if(state.position_words[i]!=be(ram+0x17f0000+i*4))return 8;
 c.gpr[3]=record;if(!call(m,&c,0x800bec50)||state.opaque_state!=c.gpr[3])return 8;
 unsigned owner=be(ram+r+4)&0x1ffffff;if(owner>24*1024*1024-36)return 9;unsigned points=be(ram+owner+32)&0x1ffffff;if(points>24*1024*1024-44)return 9;
 unsigned char original[0xf4];memcpy(original,ram+r,sizeof original);
 for(unsigned test=0;test<4;++test){float current[3],first[3],second[3],out[3];
 if(test){const float heights[3][3]={{3,2,1},{0,3,2},{0,1,3}};putfloat(ram+r+16,heights[test-1][0]);putfloat(ram+points+20,heights[test-1][1]);putfloat(ram+points+36,heights[test-1][2]);}
 for(unsigned i=0;i<3;++i){current[i]=real(ram+r+12+i*4);first[i]=real(ram+points+16+i*4);second[i]=real(ram+points+32+i*4);}
 c.gpr[3]=record;c.gpr[4]=0x817f0000;if(!call(m,&c,0x800be678)||!nflstreet_ball_anchor(current,first,second,out))return 10;
 for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,out+i,4);if(word!=be(ram+0x17f0000+i*4))return 11;}
 }
 /* Original numeric sampler, actual inputs plus controlled edge branches. */
 for(unsigned test=0;test<12;++test){memcpy(ram+r,original,sizeof original);
  if(test>=4){putfloat(ram+r+0x54,0.125f);putfloat(ram+r+0x58,-0.25f);
   putfloat(ram+r+0x5c,test&1?0.01f:-0.01f);putfloat(ram+r+0x70,0.0001f);putfloat(ram+r+0x74,-0.0002f);}
  nflstreet_ball_trajectory input;for(unsigned i=0;i<3;++i){input.position[i]=real(ram+r+12+i*4);input.velocity[i]=real(ram+r+0x54+i*4);}
  for(unsigned i=0;i<2;++i)input.acceleration_xy[i]=real(ram+r+0x70+i*4);
  double plane=test%4==0?0:test%4==1?input.position[2]:test%4==2?input.position[2]+1.0:input.position[2]-1.0;
  float time,xy[2];c.gpr[3]=record;c.gpr[4]=0x817f0000;c.gpr[5]=0x817f0004;c.fpr[1]=plane;
  if(!call(m,&c,0x800c11dc)||!nflstreet_ball_sample_plane(&input,plane,&time,xy))return 12;
  printf("Plane case=%u target=%.9g time=%.9g xy=%.9g,%.9g\n",test,plane,time,xy[0],xy[1]);
  float outputs[3]={time,xy[0],xy[1]};for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,outputs+i,4);if(word!=be(ram+0x17f0000+i*4)){printf("Plane sampler mismatch case %u word %u native=%08x original=%08x\n",test,i,word,be(ram+0x17f0000+i*4));return 13;}}
 }
 puts("Twelve original ball plane-sampler time/XY controls exact");
 printf("Ball active-record, position, opaque-state and four anchor controls exact: pool=%08x index=%u position_words=%08x,%08x,%08x state=%08x\n",root,state.active_index,state.position_words[0],state.position_words[1],state.position_words[2],state.opaque_state);
 free(ram);FreeLibrary(h);return 0;}
