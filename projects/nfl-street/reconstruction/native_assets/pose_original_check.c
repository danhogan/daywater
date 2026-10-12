/* Research-only oracle: original DLL dispatch. Not linked by the native preview.
   Compare exact frame rotations and approximate host-trigonometry palettes.
   Scratch RAM identities are harness allocations, not game-state claims.
   Arguments: original DLL, direct bank, optional TMdl + original DOL.
   Palette comparison bypasses only 0x801F0BD0 hardware cache flush. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "pose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned u16(const unsigned char*p){return p[0]*256u+p[1];}
static unsigned u32(const unsigned char*p){return p[0]*16777216u+p[1]*65536u+p[2]*256u+p[3];}
static void put(unsigned char*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char**argv){if(argc!=3&&argc!=5)return 2;
 HMODULE h=LoadLibraryA(argv[1]);if(!h){printf("load %lu\n",GetLastError());return 3;}
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
 const ModernGekkoModuleDesc*m=get();printf("ABI %u cpu %u size %u/%zu\n",m->abi_version,m->cpu_abi_version,m->cpu_state_size,sizeof(CPUState));if(m->cpu_state_size!=sizeof(CPUState))return 4;
 FILE*f=fopen(argv[2],"rb");if(!f)return 5;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);unsigned char*b=malloc(n);fread(b,1,n,f);fclose(f);
 unsigned char*ram=calloc(1,24*1024*1024);memcpy(ram+0x100000,b,n);
 unsigned d=u32(b+20),ft=u32(b+16),components=u32(b+d);
 for(unsigned j=1;j<6;++j)put(ram+0x100000+d+j*4,0x80100000+u32(b+d+j*4));
 unsigned checked=0;for(unsigned i=0;i<u16(b+4);++i){unsigned frame=u32(b+ft+i*4),header=10+((b[frame]&64)?6:0)+((b[frame]&128)?6:0);
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.hid2=0xe0000000;c.msr=0x2000;c.ram=ram;c.ram_size=24*1024*1024;c.gpr[1]=0x81700000;c.gpr[3]=0x81000000;c.gpr[4]=0x80100000+frame+header;c.gpr[5]=0x80100000+d;c.gpr[6]=0;c.lr=0;c.pc=0x8015d104;c.downcount=10000000;c.cycle_budget=10000000;
 int ret=0;for(unsigned step=0;c.pc&&step<100000;++step)ret=m->dispatch(&c,c.pc);if(c.pc!=0){printf("dispatch %d pc %x ex %x frame %u\n",ret,c.pc,c.exception,i);return 6;}
 c.gpr[3]=c.gpr[4]=0x81000000;c.gpr[5]=components;c.gpr[6]=0x80100000+u32(b+d+4);c.gpr[7]=0x80100000+u32(b+d+8);c.pc=0x801667cc;c.lr=0;c.downcount=10000000;
 for(unsigned step=0;c.pc&&step<100000;++step)m->dispatch(&c,c.pc);if(c.pc!=0)return 7;
 int16_t native[384];unsigned count;if(!nflstreet_pose_frame(b,n,i,native,384,&count)||count!=components)return 8;
 for(unsigned j=0;j<components;++j)if((uint16_t)native[j]!=u16(ram+0x1000000+j*2)){printf("mismatch frame %u component %u: %d vs %d\n",i,j,native[j],(int16_t)u16(ram+0x1000000+j*2));return 9;}
 ++checked;}
 printf("Verified %u original compressed frames, %u components each, exact rotations\n",checked,components);
 if(argc==5){
 FILE*df=fopen(argv[4],"rb");if(!df)return 10;unsigned char dh[256];fread(dh,1,256,df);
 for(unsigned i=0;i<18;++i){unsigned file=u32(dh+i*4),address=u32(dh+72+i*4),size=u32(dh+144+i*4);if(size){if((address&0x1ffffff)+size>24*1024*1024)return 11;fseek(df,file,SEEK_SET);fread(ram+(address&0x1ffffff),1,size,df);}}fclose(df);
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.hid2=0xe0000000;c.msr=0x2000;c.ram=ram;c.ram_size=24*1024*1024;c.gpr[1]=0x81700000;c.pc=0x801888f0;c.lr=0;c.downcount=c.cycle_budget=100000000;
 for(unsigned step=0;c.pc&&step<1000000;++step)m->dispatch(&c,c.pc);if(c.pc){printf("init pc %x\n",c.pc);return 12;}
 FILE*bf=fopen(argv[3],"rb");if(!bf)return 13;fseek(bf,0,SEEK_END);long size=ftell(bf);rewind(bf);unsigned char*model=malloc(size);fread(model,1,size,bf);fclose(bf);
 unsigned sk=0,cl=0;for(unsigned i=0;i<u16(model+8);++i){unsigned char*r=model+u16(model+10)+i*16;if(!memcmp(r,"Skel",4))sk=u32(r+4);if(!memcmp(r,"Clst",4))cl=u32(r+4);}if(!sk||!cl)return 14;
 memcpy(ram+0x400000,model,size);for(unsigned j=1;j<5;++j)put(ram+0x400000+sk+j*4,0x80400000+u32(model+sk+j*4));put(ram+0x400000+cl,0x80400000+u32(model+cl));
 put(ram+0x1300000,0x81000000);put(ram+0x1300004,0);
 int16_t pose[768];unsigned bones=u32(model+sk);for(unsigned j=0;j<bones*3;++j){pose[j]=(int16_t)(u16(model+u32(model+sk+12)+j*2)+((int)(j%7)-3)*437);ram[0x1200000+j*2]=(uint16_t)pose[j]>>8;ram[0x1200001+j*2]=pose[j];}
 float native[256][12];unsigned clusters;if(!nflstreet_pose_clusters(model,size,pose,bones*3,native,&clusters))return 15;
 put(ram+0x3a3458,0x81400000);put(ram+0x1400000,1);put(ram+0x1400004,0);put(ram+0x1400008,0x81400100);for(unsigned j=0;j<16;++j)put(ram+0x1400100+j*4,(j==0||j==5||j==10||j==15)?0x3f800000:0);
 memset(&c,0,sizeof c);c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.hid2=0xe0000000;c.msr=0x2000;c.ram=ram;c.ram_size=24*1024*1024;c.gpr[1]=0x81700000;c.gpr[3]=0x81300000;c.gpr[4]=0x80400000+sk;c.gpr[5]=0x80400000+cl;c.gpr[6]=0x81200000;c.gqr[6]=0x0f070007;c.lr=0;c.pc=0x801ca4e8;c.downcount=c.cycle_budget=100000000;
 for(unsigned step=0;c.pc&&step<1000000;++step){if(c.pc==0x801f0bd0)c.pc=c.lr;else m->dispatch(&c,c.pc);}if(c.pc){printf("palette pc %x srr0 %x program %x exception %x\n",c.pc,c.srr0,c.program_exception,c.exception);return 16;}
  float error=0;for(unsigned j=0;j<clusters*12;++j){uint32_t bits=u32(ram+0x1000000+j*4);float original;memcpy(&original,&bits,4);float difference=fabsf(((float*)native)[j]-original);if(difference>error)error=difference;}
 printf("Original weighted cluster comparison: %u clusters, max float difference %.9g (host trigonometry)\n",clusters,error);if(!isfinite(error)||error>0.001f)return 17;free(model);
 }
free(ram);free(b);FreeLibrary(h);return 0;}
