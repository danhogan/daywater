#include <windows.h>
#include "moderngekko/module_abi.h"
#include "live_locomotion.h"
#include "pose.h"
#include "../actor_target_input/command_timing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned u32(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static float real(const uint8_t*p){unsigned w=u32(p);float f;memcpy(&f,&w,4);return f;}
static void scalar(uint8_t*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
static uint8_t*read(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size<0){fclose(f);return NULL;}uint8_t*b=malloc(size);if(!b||fread(b,1,size,f)!=(size_t)size){free(b);b=NULL;}fclose(f);*n=size;return b;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){m->dispatch(c,c->pc);if(c->exception)break;}return !c->pc&&!c->exception;}
int main(int argc,char**argv){if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;
 size_t rn,bn,pn;uint8_t*ram=read(argv[2],&rn),*bank=read(argv[3],&bn),*profile=read(argv[4],&pn);if(!ram||rn!=24*1024*1024||!bank||bn>1024*1024||!profile)return 5;
 nflstreet_live_locomotion native;if(!nflstreet_live_locomotion_open(&native,bank,bn,profile,pn))return 6;
 native.transition_duration=0; /* Preserve the separately checked phase-only path. */
 memcpy(ram+0x1000000,bank,bn);memcpy(ram+0x17f3000,native.curve,native.curve_size);memcpy(ram+0x17f4000,native.descriptor,8);memcpy(ram+0x17f5000,native.channels,248);
 /* Controlled registry bank2 points at the same raw source bank, with its
    program table relocated for the actual original rebind caller. This does
    not assert byte equivalence of the entire resident bank from the capture. */
 unsigned table=u32(bank+12),programs=u32(bank+table),registry=u32(ram+0x3a4ad4);
 if(table+4+(size_t)programs*4>bn||registry<0x80000000||registry-0x80000000+3*28>rn)return 18;
 put(ram+0x1000000+12,0x81000000+table);
 for(unsigned i=0;i<programs;++i)put(ram+0x1000000+table+4+i*4,0x81000000+u32(bank+table+4+i*4));
 put(ram+(registry-0x80000000)+2*28+12,0x81000000);
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=rn;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 unsigned last=65535,loops=0;
 for(unsigned step=0;step<480;++step){
  /* Controlled signed-speed sweep through all primary curve pairs, including
     zero/rest, both signs, sudden changes and repeated timing catch-up. */
  double speed=step<120?-0.2+(double)step*0.003:step<240?0.3-(double)(step-120)*0.004:step<360?0:step&1?-0.15:0.2;
  double delta=step%7==0?3.25:step%7==1?0:0.5;int16_t pose[384];unsigned components;
  if(!nflstreet_live_locomotion_step(&native,speed,delta,pose,384,&components))return 7;
  c.gpr[3]=0x817f3000;c.fpr[1]=speed;if(!call(m,&c,0x80064638)||c.gpr[3]!=native.pair)return 8;unsigned index=c.gpr[3];
  if(index!=last){for(unsigned slot=0;slot<2;++slot){unsigned clip=u32(ram+0x17f3000+0x20+(index+slot)*12),id=u32(bank+u32(bank+12)+4+clip*4);
    if(last==65535){c.gpr[3]=0x817f5000+slot*124+0x4c;c.gpr[4]=0x81000000+id;c.gpr[5]=1;if(!call(m,&c,0x8017724c))return 9;ram[0x17f5000+slot*124+1]=2;}
    else{uint8_t metadata[0x4c];memcpy(metadata,ram+0x17f5000+slot*124,sizeof metadata);ram[0x17f5000+slot*124+1]=2;
     c.gpr[3]=0;c.gpr[4]=0x817f5000;c.gpr[5]=slot;c.gpr[6]=2;c.gpr[7]=clip;c.gpr[8]=0;c.gpr[9]=0x803c19c0;
     if(!call(m,&c,0x8017413c)){fprintf(stderr,"Original rebind failed pc=%08x ex=%08x\n",c.pc,c.exception);return 19;}
     /* Scope is the full 64-byte stream. The caller's channel metadata writes
        and allocation status are outside the two-slot host adapter. */
     memcpy(ram+0x17f5000+slot*124,metadata,sizeof metadata);
    }
    c.gpr[3]=0x81000000+id;if(!call(m,&c,0x8017756c)||c.gpr[3]!=(unsigned)((native.channels[slot*124+0x4c+0x1a]<<8)|native.channels[slot*124+0x4c+0x1b]))return 10;
   }last=index;++loops;}
  c.gpr[3]=index;c.gpr[4]=0x817f4000;c.gpr[5]=0x817f3000;c.gpr[6]=0x817f5000;c.fpr[1]=speed;if(!call(m,&c,0x800648ec))return 11;
  for(unsigned slot=0;slot<2;++slot){c.gpr[3]=0x817f5000+slot*124+0x4c;c.fpr[1]=delta;if(!call(m,&c,0x80177374))return 12;}
  uint8_t channels[248];memcpy(channels,ram+0x17f5000,248);for(unsigned slot=0;slot<2;++slot)put(channels+slot*124+0x4c,u32(channels+slot*124+0x4c)-0x81000000);
  if(memcmp(channels,native.channels,248)||memcmp(ram+0x17f3000,native.curve,native.curve_size)){fprintf(stderr,"Joined mismatch step=%u pair=%u speed=%.9g\n",step,index,speed);for(unsigned i=0;i<248;++i)if(channels[i]!=native.channels[i]){fprintf(stderr,"byte=%u native=%02x original=%02x\n",i,native.channels[i],channels[i]);break;}return 13;}
  /* Exercise original wrapped-short blending on decoded poses and deliberate
     large signed-angle differences, with exact fixed-point weights. */
  for(unsigned i=0;i<components;++i){int16_t other=(int16_t)(pose[i]+((i&1)?32767:-32768));unsigned at=0x17f6000+i*2;
   ram[at]=(uint16_t)pose[i]>>8;ram[at+1]=pose[i];ram[at+0x400]=(uint16_t)other>>8;ram[at+0x401]=other;}
  int16_t other[384],mixed[384];for(unsigned i=0;i<components;++i)other[i]=(int16_t)(pose[i]+((i&1)?32767:-32768));unsigned weight=(step*977)%4097;
  if(!nflstreet_pose_mix(mixed,pose,other,components,weight))return 14;
  c.gpr[3]=0x817f6800;c.gpr[4]=0x817f6400;c.gpr[5]=0x817f6000;c.gpr[6]=weight;c.gpr[7]=components;if(!call(m,&c,0x80166730))return 14;
  for(unsigned i=0;i<components;++i)if((uint16_t)mixed[i]!=(unsigned)(ram[0x17f6800+i*2]<<8|ram[0x17f6801+i*2]))return 15;
 }
 /* Exact cycle boundaries and neighboring floats through the full original
    caller, independent of the carried sweep's incidental elapsed times. */
 unsigned clip=u32(native.curve+0x20),id=u32(bank+table+4+clip*4),tags=0;size_t at=id+4;
 for(;at+2<=bn&&(bank[at]&0x80)==0;at+=2)++tags;
 if(at+2>bn||tags<2||bank[at]!=0x80||bank[at+1]!=1)return 20;
 float cycle=real(native.channels+0x4c+4)*(float)((native.channels[0x4c+0x1a]<<8)|native.channels[0x4c+0x1b]);
 const float elapsed[]={0,cycle,cycle*3,nextafterf(cycle,0),nextafterf(cycle,INFINITY),nextafterf(cycle*3,INFINITY)};
 for(unsigned test=0;test<6;++test){uint8_t s[64];memcpy(s,native.channels+0x4c,64);scalar(s+8,elapsed[test]);uint32_t flags=0;
  memcpy(ram+0x17f5000,native.channels,124);memcpy(ram+0x17f5000+0x4c,s,64);put(ram+0x17f5000+0x4c,0x81000000+u32(s));ram[0x17f5001]=2;
  c.gpr[3]=0;c.gpr[4]=0x817f5000;c.gpr[5]=0;c.gpr[6]=2;c.gpr[7]=clip;c.gpr[8]=0;c.gpr[9]=0x803c19c0;
  if(!call(m,&c,0x8017413c)||!nflstreet_stream_rebind_phase(s,id,bank+id,at+2-id,(uint16_t)tags,&flags))return 21;
  uint8_t expected[64];memcpy(expected,ram+0x17f5000+0x4c,64);put(expected,u32(expected)-0x81000000);
  if(memcmp(expected,s,64)){fprintf(stderr,"Phase boundary mismatch test=%u elapsed=%.9g\n",test,elapsed[test]);return 22;}
 }
 uint8_t guarded[64],saved[64];memcpy(guarded,native.channels+0x4c,64);memcpy(saved,guarded,64);uint32_t flags=0x12345678;
 if(nflstreet_stream_rebind_phase(guarded,id,bank+id,at-id,(uint16_t)tags,&flags)||memcmp(saved,guarded,64)||flags!=0x12345678)return 23;
 uint8_t unsupported[256];if(at+2-id>sizeof unsupported)return 23;memcpy(unsupported,bank+id,at+2-id);unsupported[4]=0xc0;unsupported[5]=0x0c;
 if(nflstreet_stream_rebind_phase(guarded,id,unsupported,at+2-id,(uint16_t)tags,&flags)||memcmp(saved,guarded,64)||flags!=0x12345678)return 23;
 nflstreet_live_locomotion before=native;int16_t output[384];memset(output,0x5a,sizeof output);unsigned count=123;
 if(nflstreet_live_locomotion_step(&native,NAN,1,output,384,&count)||memcmp(&before,&native,sizeof native)||count!=123)return 16;
 for(unsigned i=0;i<sizeof output;++i)if(((uint8_t*)output)[i]!=0x5a)return 16;
 nflstreet_live_locomotion_reset(&native);if(native.updates||native.initialized||memcmp(native.curve,native.seed_curve,native.curve_size))return 17;
 printf("480 joined source-curve/pair/blend/timing updates exact; %u pair starts and original tag-count scans; %u full original rebind stream comparisons; 6 exact phase-boundary controls; 480 wrapped pose blends exact; rollback/reset passed\n",loops,(loops-1)*2);
 free(ram);free(bank);free(profile);FreeLibrary(h);return 0;
}
