/* Original group row assembly and descriptor selection. No DLL/source edits. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../actor_target_input/locomotion_selection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned be(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static void putreal(unsigned char*p,float f){unsigned v;memcpy(&v,&f,4);p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){m->dispatch(c,c->pc);if(c->exception)break;}return !c->pc&&!c->exception;}
int main(int argc,char**argv){if((argc!=3&&argc!=4&&argc!=5)||(argc==5&&strcmp(argv[4],"--identities")))return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;
 unsigned char*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 5;fclose(f);
 unsigned reg=be(ram+0x3a4ad4),count=be(ram+0x3a4ad0),context=half(ram+0x3c1e9a);if(reg<0x80000000||count>4096||reg-0x80000000+count*28>24*1024*1024)return 6;
 nflstreet_asset_store store={0x80000000,ram,ram+(reg-0x80000000),24*1024*1024,count*28};
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 const unsigned groups[]={0x4a,0x49,0xa0,0x9d,0x9f};unsigned tests=0;
 for(unsigned g=0;g<5;++g){unsigned char rows[520];memset(ram+0x17f1000,0,520);c.gpr[3]=context;c.gpr[4]=groups[g];c.gpr[5]=0x817f1000;
  if(!call(m,&c,0x80175af8)||!nflstreet_asset_group_rows(&store,(uint16_t)context,(uint16_t)groups[g],rows))return 7;
  unsigned n=half(rows);if(n>64||memcmp(rows,ram+0x17f1000,4+(size_t)n*8))return 8;
  printf("Original group %04x: context=%u rows=%u exact\n",groups[g],context,n);
  for(unsigned i=0;i<81+n;++i){unsigned char keys[4];if(i<81){unsigned v=i;for(unsigned j=0;j<4;++j){keys[j]=(unsigned char)(v%3);v/=3;}}
   else {unsigned id=be(rows+8+(i-81)*8);const uint8_t*p=nflstreet_asset_read(&store,id,8);if(!p)return 8;memcpy(keys,p+4,4);}
   memcpy(ram+0x17f0000,keys,4);c.gpr[3]=0x817f1000;c.gpr[4]=0x817f0000;nflstreet_locomotion_choice choice;
   if(!call(m,&c,0x800211d8)||!nflstreet_locomotion_select(&store,rows,4+(size_t)n*8,keys,&choice)||c.gpr[3]!=(unsigned)choice.row)return 9;
   if(i>=81)printf("Source variant %02x%02x%02x%02x -> row=%d bank=%u clip=%04x score=%u\n",keys[0],keys[1],keys[2],keys[3],choice.row,choice.bank_key,choice.clip_key,choice.score);
   ++tests;
  }
 }

 /* Resolve the actual saved active group-4A variant keys, then compare the
    original final curve installation byte-for-byte. */
 unsigned manager=be(ram+0x3c1cd8),payload=0;
 for(unsigned i=0;i<4;++i){unsigned off=(manager&0x1ffffff)+i*100;
  if(half(ram+off+4)==0x4a&&half(ram+off+6)==3){payload=off+16;break;}}
 if(!payload)return 10;unsigned char data[64],original[64],keys[2];memcpy(data,ram+payload,64);memcpy(original,data,64);memcpy(keys,data+0x3c,2);
 if(!nflstreet_locomotion_resolve(&store,(uint16_t)context,keys,data))return 10;
 unsigned group=0x4a;
 for(unsigned stage=0;stage<3;++stage){c.gpr[3]=context;c.gpr[4]=group;c.gpr[5]=0x817f1000;
  if(!call(m,&c,0x80175af8))return 11;
  if(stage<2){unsigned char variants[4]={keys[stage],255,255,255};memcpy(ram+0x17f0000,variants,4);
   c.gpr[3]=0x817f1000;c.gpr[4]=0x817f0000;if(!call(m,&c,0x800211d8)||c.gpr[3]>=half(ram+0x17f1000))return 11;
   group=half(ram+0x17f1006+c.gpr[3]*8)&0x7fff;
  }
 }
 memcpy(ram+0x17f2000,original,64);c.gpr[3]=0x817f1000;c.gpr[4]=0x817f2000;c.gpr[5]=0x803c19c0;
 if(!call(m,&c,0x80062340)||memcmp(data,ram+0x17f2000,64))return 12;
 printf("Normal variant keys %02x%02x: final group=%04x, entire 64-byte curve payload exact\n",keys[0],keys[1],group);

 if(argc>=4){FILE*out=fopen(argv[3],"wb");if(!out)return 15;
  unsigned char header[12]={'N','L','C','1',0,0,0,0,0,0,0,0};if(argc==5)header[3]='2';header[4]=context>>8;header[5]=context;header[6]=keys[0];header[7]=keys[1];header[11]=3;
  if(fwrite(header,1,12,out)!=12)return 15;
  for(unsigned kind=0;kind<3;++kind){unsigned at=kind==0?0x10:kind==1?0x14:0x18;unsigned identity=be(data+at);const uint8_t*curve=nflstreet_asset_read(&store,identity,8);if(!curve)return 15;
   unsigned size=0x18+be(curve+4)*12;curve=nflstreet_asset_read(&store,identity,size);if(!curve)return 15;unsigned char row[8]={data[kind==0?0x37:kind==1?0x38:0x39],0,0,0,0,0,0,0};
   row[4]=size>>24;row[5]=size>>16;row[6]=size>>8;row[7]=size;
   if(fwrite(row,1,8,out)!=8)return 15;
   if(argc==5){unsigned char id[4]={identity>>24,identity>>16,identity>>8,identity};if(fwrite(id,1,4,out)!=4)return 15;}
   if(fwrite(curve,1,size,out)!=size)return 15;
  }
  fclose(out);puts("Exported private native-resolved curve inputs; no RAM image or expected output states included");
 }
 const double speeds[]={-1,-0.5,-0.1,-0.01,-0.000001,0,0.000001,0.01,0.02,0.05,0.1,0.15,0.2,0.3,0.5,1};unsigned pairs=0,blends=0;
 for(unsigned kind=0;kind<3;++kind){unsigned at=kind==0?0x10:kind==1?0x14:0x18;unsigned identity=be(data+at);const uint8_t*curve=nflstreet_asset_read(&store,identity,8);if(!curve)return 13;
  unsigned rows=be(curve+4);size_t size=0x18+(size_t)rows*12;curve=nflstreet_asset_read(&store,identity,size);if(!curve)return 13;
  unsigned normal=(unsigned)(sizeof speeds/sizeof *speeds),boundaries=rows-2;
  for(unsigned i=0;i<normal+boundaries*3;++i){double speed;
   if(i<normal)speed=speeds[i];else{unsigned b=(i-normal)/3,side=(i-normal)%3;uint32_t bits=be(curve+0x28+b*12);float threshold;memcpy(&threshold,&bits,4);
    if(fabsf(threshold)<0.000001f)threshold=0;
    speed=side==0?nextafterf(threshold,-INFINITY):side==1?threshold:nextafterf(threshold,INFINITY);}
   c.gpr[3]=identity;c.fpr[1]=speed;uint16_t index;uint32_t clips[2];
   if(!call(m,&c,0x80064638)||!nflstreet_locomotion_pair(curve,size,speed,&index,clips)||c.gpr[3]!=index){
    fprintf(stderr,"Pair mismatch kind=%u speed=%.17g original=%u\n",kind,speed,(unsigned)c.gpr[3]);return 14;}
   printf("Curve kind=%u bank=%u speed=%.9g pair=%u clips=%04x,%04x exact\n",kind,data[kind==0?0x37:kind==1?0x38:0x39],speed,index,clips[0],clips[1]);++pairs;
   for(unsigned mode=0;mode<4;++mode){uint8_t native_curve[1600],channels[4*124],descriptor[8]={0,1,255,255,0,0,0,0};float rate=0;
    if(size>sizeof native_curve)return 16;memcpy(native_curve,curve,size);memset(channels,0x5a,sizeof channels);
    putreal(descriptor+4,mode==3?0.75f:1.0f);
    if(mode==2){descriptor[2]=2;descriptor[3]=3;}else if(mode==3){descriptor[2]=0;descriptor[3]=1;}
    for(unsigned slot=0;slot<4;++slot){uint8_t*s=channels+slot*124+0x4c;
     putreal(s+4,mode==1?0:1.25f);putreal(s+8,7.75f);putreal(s+12,-0.125f);
     unsigned tags=mode==0?17:mode==1?0:mode==2?36:15;s[0x1a]=tags>>8;s[0x1b]=tags;
     putreal(channels+slot*124+0x30,slot==0?(mode==2?0.25f:1.0f):slot==1?(mode==2?0.75f:0.0f):0.5f);
    }
    memcpy(ram+0x17f3000,native_curve,size);memcpy(ram+0x17f4000,descriptor,8);memcpy(ram+0x17f5000,channels,sizeof channels);
    c.gpr[3]=index;c.gpr[4]=0x817f4000;c.gpr[5]=0x817f3000;c.gpr[6]=0x817f5000;c.fpr[1]=speed;
    if(!call(m,&c,0x800648ec)||!nflstreet_locomotion_blend(native_curve,size,index,speed,descriptor,channels,sizeof channels,&rate)||
       memcmp(native_curve,ram+0x17f3000,size)||memcmp(channels,ram+0x17f5000,sizeof channels)||(double)rate!=c.fpr[1]){
     fprintf(stderr,"Blend mismatch kind=%u speed=%.17g mode=%u native rate=%.9g original=%.17g\n",kind,speed,mode,rate,c.fpr[1]);
     for(unsigned k=0;k<sizeof channels;++k)if(channels[k]!=ram[0x17f5000+k]){fprintf(stderr,"channel byte %u native=%02x original=%02x\n",k,channels[k],ram[0x17f5000+k]);break;}return 16;
    }++blends;
    if(i==0&&mode==0){uint8_t before_curve[1600],before_channels[4*124],invalid[8];
     memcpy(before_curve,native_curve,size);memcpy(before_channels,channels,sizeof channels);memcpy(invalid,descriptor,8);invalid[2]=4;
     float unchanged=-7;
     if(nflstreet_locomotion_blend(native_curve,size,index,speed,invalid,channels,sizeof channels,&unchanged)||
        memcmp(native_curve,before_curve,size)||memcmp(channels,before_channels,sizeof channels)||unchanged!=-7)return 17;
     channels[0x4c+0x1a]=channels[0x4c+0x1b]=255;memcpy(before_channels,channels,sizeof channels);
     if(nflstreet_locomotion_blend(native_curve,size,index,speed,descriptor,channels,sizeof channels,&unchanged)||
        memcmp(native_curve,before_curve,size)||memcmp(channels,before_channels,sizeof channels)||unchanged!=-7)return 17;
    }
   }
  }
 }
 printf("%u original adjacent clip-pair selections exact\n",pairs);
 printf("%u original paired blend/rate controls exact (entire curves and four channels)\n",blends);
 printf("%u original weighted descriptor choices exact; first-row ties preserved\n",tests);free(ram);FreeLibrary(h);return 0;}
