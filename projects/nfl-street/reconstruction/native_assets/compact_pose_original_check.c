/* Controlled ball association oracle, not possession gameplay. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../ball/ball_attachment.h"
#include "compact_pose.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x801f0bd0)c->pc=c->lr;else m->dispatch(c,c->pc);}return !c->pc&&!c->exception;}
static float real(const uint8_t*p){unsigned w=be(p);float f;memcpy(&f,&w,4);return f;}
static void putfloat(uint8_t*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 f=fopen(argv[3],"rb");if(!f)return 4;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);uint8_t*skl=malloc(size);if(!skl||fread(skl,1,size,f)!=(size_t)size)return 4;fclose(f);
 const unsigned coefficients[6]={0x40490fdb,0xc0a55de8,0x402335e5,0xbf196969,0x3da83c1d,0xbbf183ad};
 for(unsigned i=0;i<6;++i)if(be(ram+0x39e100+i*4)!=coefficients[i])return 4;
 unsigned actors[2]={0x3c04ec,0x3c19c0},matrices=0,joins=0;
 for(unsigned actor=0;actor<2;++actor){unsigned a=actors[actor],model=be(ram+a+4)-0x80000000,skeleton=be(ram+model+0x64)-0x80000000,angles_at=be(ram+model+0x5c)-0x80000000;
  unsigned bones=(unsigned)ram[skeleton+6]<<8|ram[skeleton+7];if(bones!=29||memcmp(ram+skeleton+16,skl+16,bones*16))return 4;
  int16_t base[768],angles[768];float world[12],local[3];for(unsigned i=0;i<bones*3;++i)base[i]=(int16_t)((unsigned)ram[angles_at+i*2]<<8|ram[angles_at+i*2+1]);
  for(unsigned i=0;i<12;++i)world[i]=real(ram+a+0x280+i*4);for(unsigned i=0;i<3;++i)local[i]=real(ram+model+0x40+i*4);
  for(unsigned test=0;test<16;++test){float root[12]={1,0,0,0,0,1,0,0,0,0,1,0};
   if(test&1){root[3]=1.25f;root[7]=-2.5f;root[11]=0.375f;}
   if(test&2){root[0]=0;root[2]=1;root[8]=-1;root[10]=0;}
   if(test&4){root[0]*=1.25f;root[5]*=0.75f;root[10]*=1.5f;}
   for(unsigned i=0;i<bones*3;++i){angles[i]=test<8?base[i]:(int16_t)(uint16_t)((uint16_t)base[i]+((int)(i%7)-3)*437);ram[0x17d1000+i*2]=(uint16_t)angles[i]>>8;ram[0x17d1001+i*2]=angles[i];}
   put(ram+0x17d0030,0x817d1000);put(ram+0x17d0034,0x817c0000);
   put(ram+0x3a3458,0x81400000);put(ram+0x1400000,1);put(ram+0x1400004,0);put(ram+0x1400008,0x81400100);
   for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)putfloat(ram+0x1400100+(row*4+col)*4,row<3?root[row*4+col]:(col==3?1.0f:0.0f));
   CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
   c.gpr[3]=0x817d0000;c.gpr[4]=skeleton+0x80000000;c.gpr[5]=0;
   float native[256][12];unsigned count;
   if(!call(m,&c,0x801eae1c)||!nflstreet_compact_pose(skl,size,angles,bones*3,root,native,&count)||count!=bones)return 5;
   for(unsigned bone=0;bone<bones;++bone){for(unsigned i=0;i<12;++i){unsigned w;memcpy(&w,native[bone]+i,4);if(w!=be(ram+0x17c0000+bone*64+i*4)){fprintf(stderr,"Compact mismatch actor=%u test=%u bone=%u word=%u native=%08x original=%08x\n",actor,test,bone,i,w,be(ram+0x17c0000+bone*64+i*4));return 6;}}++matrices;}
   /* Join native compact matrices to the position service. Original receives its independently calculated matrices. */
   put(ram+model+0x60,0x817c0000);
   for(unsigned side=0;side<2;++side){int selector=side?24:18;float point[3];if(!nflstreet_ball_attachment_position(world,native,count,selector,local,point))return 7;
    ram[model+0x32]=0;ram[model+0x33]=selector;c.gpr[3]=a+0x80000000;c.gpr[4]=0x817f0000;c.gpr[5]=0x817f0010;
    if(!call(m,&c,0x80020c2c))return 7;for(unsigned i=0;i<3;++i){unsigned w;memcpy(&w,point+i,4);if(w!=be(ram+0x17f0000+i*4))return 7;}++joins;
   }
  }
 }
 float root[12]={1,0,0,0,0,1,0,0,0,0,1,0},out[256][12],before[256][12];memset(out,0xa5,sizeof out);memcpy(before,out,sizeof out);unsigned count=99;int16_t zero[87]={0};
 uint8_t*bad=malloc(size);memcpy(bad,skl,size);bad[16+16+12]=0x7f;bad[16+16+13]=0xff;
 if(nflstreet_compact_pose(bad,size,zero,87,root,out,&count)||memcmp(out,before,sizeof out)||count!=99)return 8;
 if(nflstreet_compact_pose(skl,16,zero,87,root,out,&count)||memcmp(out,before,sizeof out)||count!=99)return 8;
 root[0]=NAN;
 if(nflstreet_compact_pose(skl,size,zero,87,root,out,&count)||memcmp(out,before,sizeof out)||count!=99)return 8;
 printf("%u complete compact bone matrices exact, %u left/right attachment joins exact: captured/perturbed rotations and translated/rotated/scaled roots; malformed rollback pass\n",matrices,joins);
 free(bad);free(skl);free(ram);FreeLibrary(h);return 0;
}
