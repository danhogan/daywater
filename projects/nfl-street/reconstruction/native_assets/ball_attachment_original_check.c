/* Controlled ball association oracle, not possession gameplay. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../ball/ball_attachment.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i)m->dispatch(c,c->pc);return !c->pc&&!c->exception;}
static float real(const uint8_t*p){unsigned w=be(p);float f;memcpy(&f,&w,4);return f;}
static void putfloat(uint8_t*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
int main(int argc,char**argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();
 if(m->cpu_state_size!=sizeof(CPUState))return 3;uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");
 if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 unsigned actors[2]={0x3c04ec,0x3c19c0},checks=0,joins=0;
 for(unsigned actor=0;actor<2;++actor){unsigned a=actors[actor],model=be(ram+a+4)-0x80000000;
  if(model>24*1024*1024-0x68)return 4;
  unsigned cluster=be(ram+model+0x60)-0x80000000,skeleton=be(ram+model+0x64)-0x80000000;
  if(skeleton>24*1024*1024-8||memcmp(ram+skeleton,"SKL1",4))return 4;
  unsigned count=(unsigned)ram[skeleton+6]<<8|ram[skeleton+7];
  if(!count||count>256||cluster>24*1024*1024-count*64)return 4;
  int original_selector=(int)(int16_t)((unsigned)ram[model+0x32]<<8|ram[model+0x33]);
  if(original_selector<0||(unsigned)original_selector>=count)return 4;
  float base_world[12],base_local[3],clusters[256][12];
  for(unsigned i=0;i<12;++i)base_world[i]=real(ram+a+0x280+i*4);
  for(unsigned i=0;i<3;++i)base_local[i]=real(ram+model+0x40+i*4);
  for(unsigned row=0;row<count;++row)for(unsigned i=0;i<12;++i)clusters[row][i]=real(ram+cluster+row*64+i*4);
  for(unsigned test=0;test<4;++test)for(int selection=-2;selection<=original_selector;++selection){
   int selector=selection== -2?original_selector:selection;float world[12],local[3],out[3];memcpy(world,base_world,sizeof world);memcpy(local,base_local,sizeof local);
   if(test==1){world[3]+=3.25f;world[7]-=7.5f;world[11]+=1.125f;}
   if(test==2){local[0]=0.25f;local[1]=-0.5f;local[2]=0.125f;}
   if(test==3){world[0]*=1.25f;world[5]*=0.75f;world[10]*=1.5f;}
   for(unsigned i=0;i<12;++i)putfloat(ram+a+0x280+i*4,world[i]);for(unsigned i=0;i<3;++i)putfloat(ram+model+0x40+i*4,local[i]);
   ram[model+0x32]=(unsigned)selector>>8;ram[model+0x33]=(unsigned)selector;
   CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
   c.gpr[3]=a+0x80000000;c.gpr[4]=0x817f0000;c.gpr[5]=0x817f0010;
   if(!call(m,&c,0x80020c2c)){fprintf(stderr,"Original failed actor=%u test=%u selector=%d pc=%08x exception=%u\n",actor,test,selector,c.pc,c.exception);return 5;}if(!nflstreet_ball_attachment_position(world,clusters,(size_t)original_selector+1,selector,local,out)){fprintf(stderr,"Native rejected actor=%u test=%u selector=%d\n",actor,test,selector);return 5;}
   for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,out+i,4);if(word!=be(ram+0x17f0000+i*4)){fprintf(stderr,"Attachment mismatch actor=%u test=%u selector=%d axis=%u native=%08x original=%08x\n",actor,test,selector,i,word,be(ram+0x17f0000+i*4));return 6;}}++checks;
   unsigned root=be(ram+0x3a2670),ball=root-0x80000000+8;put(ram+ball+0xb4,be(ram+a));
   c.gpr[3]=root+8;c.fpr[1]=1.0;if(!call(m,&c,0x800c0ecc)){fprintf(stderr,"Original ball pose failed actor=%u test=%u selector=%d pc=%08x\n",actor,test,selector,c.pc);return 8;}
   for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,out+i,4);if(word!=be(ram+ball+12+i*4))return 8;}++joins;

  }
 }
 float out[3]={17,18,19},saved[3];memcpy(saved,out,sizeof out);float world[12]={0},cluster[1][12]={{0}},local[3]={0};
 if(nflstreet_ball_attachment_position(world,cluster,1,1,local,out)||memcmp(out,saved,sizeof out))return 7;
 world[0]=NAN;if(nflstreet_ball_attachment_position(world,cluster,1,0,local,out)||memcmp(out,saved,sizeof out))return 7;
 printf("%u original attachment positions exact, %u associated-ball pose joins exact: two real selectors, cluster sweep, absent selector, translated roots, local offsets and scaled roots; malformed rollback pass\n",checks,joins);
 free(ram);FreeLibrary(h);return 0;
}
