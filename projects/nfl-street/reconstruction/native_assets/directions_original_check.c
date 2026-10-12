/* Original rest-heading and eight-way projection controls, no runtime linkage. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../input_translation/input_translation.h"
#include "../command_to_movement/command_to_movement.h"
#include "../command_to_movement/actor_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(unsigned char*p,unsigned w){p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){m->dispatch(c,c->pc);if(c->exception)break;}return !c->pc&&!c->exception;}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;
 unsigned char *base=malloc(24*1024*1024),*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!base||!ram||!f||fread(base,1,24*1024*1024,f)!=24*1024*1024)return 5;fclose(f);
 unsigned checks=0;for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){if(!dx&&!dy)continue;memcpy(ram,base,24*1024*1024);unsigned char vector[32]={0},actor[48];float scale=dx&&dy?0.7071067811865475244f:1;float x=dx*scale,y=dy*scale;unsigned xb,yb;memcpy(&xb,&x,4);memcpy(&yb,&y,4);put(vector+4,xb);put(vector+8,yb);memcpy(ram+0x17f0000,vector,32);memcpy(actor,ram+0x3c1bc0,48);
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.gpr[3]=0x803c19c0;c.gpr[4]=0x817f0000;
 if(!call(m,&c,0x8003ddec)||!nflstreet_project_directional_input(vector,actor)||memcmp(actor,ram+0x3c1bc0,48)){printf("projection mismatch dx=%d dy=%d\n",dx,dy);return 6;}
 uint32_t fields[12],state[21],result=0;for(unsigned j=0;j<12;++j)fields[j]=be(actor+j*4);for(unsigned j=0;j<21;++j)state[j]=be(ram+0x3c1b68+j*4);state[7]=0;put(ram+0x3c1b68+28,0);
 c.gpr[3]=0x803c19c0;c.gpr[4]=0x803c1b68;c.gpr[5]=fields[2];c.gpr[6]=0x5b05b;
 if(!call(m,&c,0x800f6fa4)||!nflstreet_result_heading(fields,state,&result)||c.gpr[3]!=result){printf("rest heading mismatch %d %d: %x %x\n",dx,dy,c.gpr[3],result);return 7;}
 unsigned projected_heading=fields[1];
 /* A controlled original stage sequence, beginning with a naturally
    braking actor advanced to rest by its unmodified caller. */
 memcpy(ram,base,24*1024*1024);c.gpr[3]=0x803c19c0;
 for(unsigned tick=0;tick<48;++tick){c.gpr[3]=0x803c19c0;if(!call(m,&c,0x800f86bc))return 8;}
 for(unsigned j=0;j<12;++j)fields[j]=be(ram+0x3c1bc0+j*4);
 for(unsigned j=0;j<21;++j)state[j]=be(ram+0x3c1b68+j*4);
 nflstreet_actor_runtime runtime;nflstreet_actor_runtime_init(&runtime,be(ram+0x3c19c0),(int16_t)((unsigned)ram[0x3c2574]<<8|ram[0x3c2575]),(int16_t)((unsigned)ram[0x3c257c]<<8|ram[0x3c257d]),be(ram+0x3c1dc8),fields,state);runtime.completed=48;
 if(state[7])return 9;
 for(unsigned tick=0;tick<8;++tick){memcpy(ram+0x17f0000,vector,32);c.gpr[3]=0x803c19c0;c.gpr[4]=0x817f0000;if(!call(m,&c,0x8003ddec))return 10;
  memcpy(ram+0x3c1bd4,ram+0x3c1bc4,8);ram[0x3c1bde]=1;put(ram+0x3c1bc0,0x3f3851ec);put(ram+0x3c1bd0,0x3f3851ec);
  c.gpr[3]=0x803c1b68;if(!call(m,&c,0x80036af8))return 10;
  c.gpr[3]=0x803c19c0;if(!call(m,&c,0x800f7c90))return 10;
  c.gpr[3]=0x803c19c0;if(!call(m,&c,0x800f705c))return 10;
  c.gpr[3]=0x803c1b68;if(!call(m,&c,0x80036b28))return 10;
  if(!nflstreet_actor_runtime_step_vector(&runtime,vector)){printf("Native stage guard %d %d %u\n",dx,dy,tick);return 11;}
  for(unsigned j=0;j<12;++j)if(runtime.actor[j]!=be(ram+0x3c1bc0+j*4)){printf("actor mismatch %d %d %u +%x\n",dx,dy,tick,j*4);return 12;}
  for(unsigned j=0;j<21;++j)if(runtime.state[j]!=be(ram+0x3c1b68+j*4)){printf("state mismatch %d %d %u +%x: %08x %08x\n",dx,dy,tick,j*4,runtime.state[j],be(ram+0x3c1b68+j*4));return 12;}
 }
 printf("Eight complete start/movement states exact for %d %d\n",dx,dy);
 printf("Verified projection and rest heading: dx=%d dy=%d heading=%06x result=%06x\n",dx,dy,projected_heading,result);++checks;
 }
 printf("%u complete directional projections and rest-heading controls exact\n",checks);free(base);free(ram);FreeLibrary(h);return 0;}
