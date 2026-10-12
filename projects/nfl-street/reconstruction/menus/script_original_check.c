/* Original 801D16A0 integer/control oracle, actual title helper bytecode.
   Owned scratch bytes/stack; no game callbacks, GPU or input mapping. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void pushcode(uint8_t*b,unsigned*pc,uint32_t v){b[(*pc)++]=0x10;put(b+*pc,v);*pc+=4;}
static int compare(const ModernGekkoModuleDesc*m,uint8_t*ram,const uint8_t*source,unsigned start,unsigned sp,unsigned seed,unsigned flags){
 uint8_t original[4096],native[4096];memcpy(original,source,4096);memcpy(native,source,4096);put(original+0x600,seed);put(native+0x600,seed);
 nflstreet_ui_script s={0};s.sp=sp;s.pc=0x81100000+start;s.flags=flags;s.words[1]=0x81100000;s.words[2]=0;
 if(sp==7){s.words[1]=UINT32_MAX;s.words[2]=0x81100098;s.words[3]=0;s.words[4]=0;s.words[5]=0x81100098;s.words[6]=0;}
 memcpy(ram+0x1100000,original,4096);for(unsigned i=0;i<256;++i)put(ram+0x17c0000+i*4,s.words[i]);
 put(ram+0x17f202c,0);memset(ram+0x17f0000,0,32);put(ram+0x17f000c,0x817c0000+sp*4);put(ram+0x17f0010,s.pc);memset(ram+0x17f1000,0,20);put(ram+0x17f1000,flags);put(ram+0x17f1010,0x81100000);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f2000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f0000;c.gpr[6]=0x817f1000;c.gpr[7]=0;c.pc=0x801d16a0;c.lr=0;c.downcount=c.cycle_budget=100000000;
 for(unsigned i=0;c.pc&&i<1000000;++i){m->dispatch(&c,c.pc);if(c.exception)break;}
 if(c.pc||c.exception){fprintf(stderr,"Original script failed start=%x pc=%x ex=%x\n",start,c.pc,c.exception);return 0;}
 if(!nflstreet_ui_script_run(native,4096,0x81100000,0x817c0000,&s,10000)){fprintf(stderr,"Native script failed %x\n",start);return 0;}
 if(memcmp(native,ram+0x1100000,4096)||s.sp*4+0x817c0000!=be(ram+0x17f000c)||s.pc!=be(ram+0x17f0010)||s.flags!=be(ram+0x17f1000)){fprintf(stderr,"State mismatch %x native sp=%u originalsp=%x\n",start,s.sp,be(ram+0x17f000c));return 0;}
 for(unsigned i=0;i<256;++i)if(s.words[i]!=be(ram+0x17c0000+i*4)){fprintf(stderr,"Stack mismatch start=%x index=%u native=%x original=%x\n",start,i,s.words[i],be(ram+0x17c0000+i*4));return 0;}return 1;
}
static int compare_byteop(const ModernGekkoModuleDesc*m,uint8_t*ram,const uint8_t*source,unsigned op,uint32_t descriptor,unsigned capacity,uint32_t data,unsigned index,unsigned value){
 uint8_t original[4096],native[4096];memcpy(original,source,4096);memcpy(native,source,4096);
 if(descriptor){put(original+0x700+4,capacity);put(native+0x700+4,capacity);put(original+0x700+8,data);put(native+0x700+8,data);}
 original[0x740]=native[0x740]=0x80;original[0x741]=native[0x741]=0xff;original[0x742]=native[0x742]=0x7f;original[0x743]=native[0x743]=0x01;
 unsigned pc=0x800;if(op==0x4a)pushcode(native,&pc,value);pushcode(native,&pc,descriptor);pushcode(native,&pc,index);if(op==0x4b)pushcode(native,&pc,value);native[pc++]=(uint8_t)op;pushcode(native,&pc,0);native[pc++]=0x44;memcpy(original+0x800,native+0x800,pc-0x800);
 nflstreet_ui_script s={0};s.pc=0x81100800;memcpy(ram+0x1100000,original,4096);memcpy(ram+0x17c0000,(uint8_t[1024]){0},1024);put(ram+0x17f000c,0x817c0000);put(ram+0x17f0010,s.pc);put(ram+0x17f1000,0);put(ram+0x17f1010,0x81100000);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f2000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f0000;c.gpr[6]=0x817f1000;c.gpr[7]=0;c.pc=0x801d16a0;c.lr=0;c.downcount=c.cycle_budget=100000000;
 for(unsigned i=0;c.pc&&i<1000000;++i){m->dispatch(&c,c.pc);if(c.exception)break;}
 if(c.pc||c.exception){fprintf(stderr,"Original byte opcode failed op=%x desc=%x cap=%u index=%u pc=%x ex=%x\n",op,descriptor,capacity,index,c.pc,c.exception);return 0;}
 if(!nflstreet_ui_script_run(native,sizeof native,0x81100000,0x817c0000,&s,10000)){fprintf(stderr,"Native byte opcode failed op=%x desc=%x cap=%u index=%u\n",op,descriptor,capacity,index);return 0;}
 if(memcmp(native,ram+0x1100000,sizeof native)||s.sp*4+0x817c0000!=be(ram+0x17f000c)||s.pc!=be(ram+0x17f0010)||s.flags!=be(ram+0x17f1000)){fprintf(stderr,"Byte opcode state mismatch op=%x desc=%x cap=%u index=%u\n",op,descriptor,capacity,index);return 0;}
 for(unsigned i=0;i<256;++i)if(s.words[i]!=be(ram+0x17c0000+i*4)){fprintf(stderr,"Byte opcode stack mismatch op=%x index=%u native=%x original=%x\n",op,i,s.words[i],be(ram+0x17c0000+i*4));return 0;}return 1;
}
static int compare_array_address(const ModernGekkoModuleDesc*m,uint8_t*ram,const uint8_t*source,unsigned dims,const uint32_t*indices){
 uint8_t original[4096],native[4096];memcpy(original,source,4096);memcpy(native,source,4096);
 put(original+0x700,dims);put(native+0x700,dims);const unsigned sizes[3]={2,3,2};unsigned product=1;
 for(unsigned j=0;j<dims;++j){put(original+0x704+j*4,sizes[j]);put(native+0x704+j*4,sizes[j]);product*=sizes[j];}
 unsigned dataoff=0x700+(dims+1)*4;
 for(unsigned i=0;i<product;++i){put(original+dataoff+i*4,0x1000+i);put(native+dataoff+i*4,0x1000+i);}
 unsigned p=0x800;pushcode(native,&p,0x81100000);for(unsigned j=0;j<dims;++j)pushcode(native,&p,indices[j]);native[p++]=0x6e;put(native+p,0x700);p+=4;native[p++]=0xff;native[p++]=(uint8_t)(-(int)(dims+1));native[p++]=0x42;pushcode(native,&p,0);native[p++]=0x44;memcpy(original+0x800,native+0x800,p-0x800);
 nflstreet_ui_script s={0};s.pc=0x81100800;
 memcpy(ram+0x1100000,original,4096);memset(ram+0x17c0000,0,1024);put(ram+0x17f000c,0x817c0000);put(ram+0x17f0010,s.pc);put(ram+0x17f1000,0);put(ram+0x17f1010,0x81100000);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f2000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f0000;c.gpr[6]=0x817f1000;c.pc=0x801d16a0;c.lr=0;c.downcount=c.cycle_budget=100000000;
 for(unsigned i=0;c.pc&&i<1000000;++i){m->dispatch(&c,c.pc);if(c.exception)break;}
 if(c.pc||c.exception){fprintf(stderr,"Original6E failed dims%u pc%x ex%x\n",dims,c.pc,c.exception);return 0;}
 if(!nflstreet_ui_script_run(native,sizeof native,0x81100000,0x817c0000,&s,10000)){fprintf(stderr,"Native6E failed dims%u\n",dims);return 0;}
 if(memcmp(native,ram+0x1100000,sizeof native)||s.sp*4+0x817c0000!=be(ram+0x17f000c)||s.pc!=be(ram+0x17f0010)||s.flags!=be(ram+0x17f1000)){fprintf(stderr,"6E dimensions/memory/PC mismatch dims%u\n",dims);return 0;}
 for(unsigned i=0;i<256;++i)if(s.words[i]!=be(ram+0x17c0000+i*4)){fprintf(stderr,"6E stack mismatch dims%u word%u native%x original%x\n",dims,i,s.words[i],be(ram+0x17c0000+i*4));return 0;}return 1;
}
static int array6e_guard(uint32_t descriptor,unsigned dims,unsigned stack_words,unsigned size){
 uint8_t screen[4096]={0},before[4096];if(descriptor==0x81100700){put(screen+0x700,dims);put(screen+0x704,size);for(unsigned i=1;i<dims&&i<10;++i)put(screen+0x704+i*4,2);}
 unsigned p=0x800;pushcode(screen,&p,descriptor);for(unsigned i=0;i<stack_words;++i)pushcode(screen,&p,0);screen[p++]=0x6e;put(screen+p,0x700);p+=4;screen[p++]=0xff;screen[p++]=(uint8_t)(-(int)(stack_words+1));screen[p++]=0x44;
 memcpy(before,screen,sizeof before);nflstreet_ui_script s={0},prior;s.pc=0x81100800;prior=s;
 if(nflstreet_ui_script_run(screen,sizeof screen,0x81100000,0x817c0000,&s,10000)||memcmp(screen,before,sizeof screen)||memcmp(&prior,&s,sizeof s))return 0;return 1;
}
static int byteop_guard(unsigned op,uint32_t descriptor,unsigned capacity,uint32_t data,unsigned index,unsigned value){
 uint8_t screen[4096]={0},before[4096];if(descriptor==0x81100700){put(screen+0x704,capacity);put(screen+0x708,data);}unsigned pc=0x800;if(op==0x4a)pushcode(screen,&pc,value);pushcode(screen,&pc,descriptor);pushcode(screen,&pc,index);if(op==0x4b)pushcode(screen,&pc,value);screen[pc++]=(uint8_t)op;pushcode(screen,&pc,0);screen[pc++]=0x44;memcpy(before,screen,sizeof before);
 nflstreet_ui_script s={0},old;s.pc=0x81100800;old=s;if(nflstreet_ui_script_run(screen,sizeof screen,0x81100000,0x817c0000,&s,10000)||memcmp(screen,before,sizeof screen)||memcmp(&s,&old,sizeof s))return 0;return 1;
}
int main(int argc,char**argv){if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);uint8_t screen[4096]={0};f=fopen(argv[3],"rb");if(!f||fread(screen,1,1279,f)!=1279)return 4;fclose(f);
 unsigned checked=0,helpers[]={0x4d1,0x4e1,0x4f1};
 for(unsigned j=0;j<3;++j)for(unsigned seed=0;seed<4;++seed){put(screen+0x108,seed*0x55555555);if(!compare(m,ram,screen,helpers[j],7,seed,seed*0x11111111))return 5;++checked;}
 unsigned binary[]={0x1c,0x1d,0x1f,0x20,0x25,0x26,0x27,0x28,0x32,0x33,0x34,0x35,0x36,0x37,0x5a};
 for(unsigned j=0;j<sizeof binary/sizeof *binary;++j)for(unsigned seed=0;seed<4;++seed){unsigned p=0x800;pushcode(screen,&p,seed==0?0:seed==1?UINT32_MAX:seed==2?0x7fffffff:0x80000000);pushcode(screen,&p,seed==3?1:7);screen[p++]=(uint8_t)binary[j];screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,seed,0))return 5;++checked;}
 unsigned unary[]={0x1e,0x21,0x22,0x24,0x2e,0x2f};
 for(unsigned j=0;j<sizeof unary/sizeof *unary;++j)for(unsigned seed=0;seed<4;++seed){unsigned p=0x800;pushcode(screen,&p,seed*0x55555555);screen[p++]=(uint8_t)unary[j];screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,seed,0))return 5;++checked;}
 for(unsigned op=0x3e;op<=0x3f;++op)for(unsigned cond=0;cond<2;++cond){unsigned p=0x800;pushcode(screen,&p,cond);pushcode(screen,&p,6);screen[p++]=(uint8_t)op;pushcode(screen,&p,99);screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,0,0))return 5;++checked;}
 {unsigned p=0x800;screen[p++]=0x43;put(screen+p,0x900);p+=4;screen[p++]=0x44;p=0x900;pushcode(screen,&p,17);screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,0,0))return 5;++checked;}
 for(unsigned mode=0;mode<3;++mode){unsigned p=0x800;pushcode(screen,&p,mode==0?0x81100600:mode==1?UINT32_MAX:0);screen[p++]=mode==0?0x18:mode==1?0x69:0x0d;screen[p++]=0x42;if(mode==2)screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,123,0))return 5;++checked;}
 unsigned float_binary[]={0x2a,0x2b,0x2c,0x2d,0x38,0x39,0x3a,0x3b,0x3c,0x3d};
 unsigned float_values[]={0,0x80000000,0x3f800000,0xbf800000,0x33d6bf95,0x3f800001};
 for(unsigned j=0;j<sizeof float_binary/sizeof *float_binary;++j)for(unsigned seed=0;seed<6;++seed){unsigned p=0x800;pushcode(screen,&p,float_values[seed]);pushcode(screen,&p,seed==4?0x33d6bf94:0x3f800000);screen[p++]=(uint8_t)float_binary[j];screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,seed,0))return 5;++checked;}
 for(unsigned op=0x7a;op<=0x7b;++op)for(unsigned capcase=0;capcase<3;++capcase)for(unsigned nullcase=0;nullcase<2;++nullcase){
  unsigned cap=capcase==0?0:capcase==1?1:16;put(screen+0x700,0);put(screen+0x704,cap);put(screen+0x708,0x81100740);
  const uint8_t bytes[16]={'A','z','Z','a',0,'B','c',0x80,'@','[','`','{','M','n',0xff,1};memcpy(screen+0x740,bytes,16);
  unsigned p=0x800;pushcode(screen,&p,nullcase?0:0x81100700);screen[p++]=(uint8_t)op;screen[p++]=0x44;
  if(!compare(m,ram,screen,0x800,1,0,0))return 5;++checked;
 }
 for(unsigned i=0;i<3;++i){unsigned p=0x800;pushcode(screen,&p,i==0?0:i==1?0x81100700:UINT32_MAX);screen[p++]=0x7d;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,0,0))return 5;++checked;}
 for(unsigned op=0x63;op<=0x64;++op)for(unsigned count=0;count<4;++count){unsigned p=0x800;
  for(unsigned i=0;i<3;++i)pushcode(screen,&p,0xdead0000u+i);screen[p++]=(uint8_t)op;put(screen+p,count);p+=4;
  unsigned remain=op==0x63?3+count:3-count;for(unsigned i=0;i<remain;++i)screen[p++]=0x42;screen[p++]=0x44;
  if(!compare(m,ram,screen,0x800,1,0,0))return 5;++checked;
 }
 for(unsigned op=0x5b;op<=0x62;++op){if(op==0x5f||op==0x60)continue;
  for(unsigned indexcase=0;indexcase<3;++indexcase){unsigned p=0x800,write=op==0x5c||op==0x5e||op==0x62,local=op==0x5b||op==0x5c,indirect=op==0x61||op==0x62;
   put(screen+0x700,2);put(screen+0x704,3);put(screen+0x708,2);for(unsigned i=0;i<6;++i)put(screen+0x70c+i*4,100+i);
   unsigned extra=local?9:indirect?1:0;
   if(local){pushcode(screen,&p,2);pushcode(screen,&p,3);pushcode(screen,&p,2);for(unsigned i=0;i<6;++i)pushcode(screen,&p,100+i);}else if(indirect)pushcode(screen,&p,0x81100700);
   pushcode(screen,&p,indexcase==0?0:indexcase==1?99:UINT32_MAX);pushcode(screen,&p,indexcase==0?1:0);
   if(write)pushcode(screen,&p,0xdeadbeefu);
   pushcode(screen,&p,local||indirect?(uint32_t)-(int32_t)(extra+2+write):0x81100700);screen[p++]=(uint8_t)op;
   for(unsigned i=0;i<extra+!write;++i)screen[p++]=0x42;screen[p++]=0x44;
   if(!compare(m,ram,screen,0x800,1,0,0))return 5;++checked;
  }
 }
 unsigned float_unary[]={0x12,0x13,0x23,0x29,0x30,0x31};
 for(unsigned j=0;j<sizeof float_unary/sizeof *float_unary;++j)for(unsigned seed=0;seed<6;++seed){unsigned p=0x800;pushcode(screen,&p,float_values[seed]);screen[p++]=(uint8_t)float_unary[j];screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,seed,0))return 5;++checked;}
 for(unsigned op=0x14;op<=0x15;++op)for(unsigned seed=0;seed<6;++seed){unsigned p=0x800;pushcode(screen,&p,float_values[seed]);pushcode(screen,&p,19);screen[p++]=(uint8_t)op;screen[p++]=0x42;screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,seed,0))return 5;++checked;}
 for(unsigned count=0;count<4;++count)for(unsigned seed=0;seed<4;++seed){unsigned p=0x800;for(unsigned j=0;j<3;++j)pushcode(screen,&p,seed*0x55555555+j);screen[p++]=0x65;put(screen+p,count);p+=4;for(unsigned j=0;j<3+count;++j)screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,seed,0))return 5;++checked;}
 for(unsigned op=0x5f;op<=0x60;++op)for(unsigned seed=0;seed<4;++seed){put(screen+0x700,2);put(screen+0x704,3);put(screen+0x708,2);for(unsigned j=0;j<6;++j)put(screen+0x70c+j*4,j+100);unsigned p=0x800;pushcode(screen,&p,0x81100000);pushcode(screen,&p,seed==3?UINT32_MAX:seed%2);pushcode(screen,&p,seed==2?99:seed);if(op==0x60)pushcode(screen,&p,0x12345678);screen[p++]=(uint8_t)op;put(screen+p,0x700);p+=4;screen[p++]=0xff;screen[p++]=op==0x60?0xfc:0xfd;if(op==0x5f)screen[p++]=0x42;screen[p++]=0x42;screen[p++]=0x44;if(!compare(m,ram,screen,0x800,1,seed,0))return 5;++checked;}
 unsigned array_indices[3][4][3]={
  {{0,0,0},{1,0,0},{2,0,0},{UINT32_MAX,0,0}},
  {{0,0,0},{2,1,0},{3,1,0},{UINT32_MAX,0,0}},
  {{0,0,0},{1,2,1},{2,2,1},{UINT32_MAX,0,1}}
 };
 for(unsigned dims=1;dims<=3;++dims)for(unsigned j=0;j<4;++j){
  if(!compare_array_address(m,ram,screen,dims,array_indices[dims-1][j]))return 5;++checked;
 }
 if(!array6e_guard(0x81100ffc,1,1,2)||!array6e_guard(0x81100700,9,9,2)||
    !array6e_guard(0x81100700,1,1,0)||!array6e_guard(0x81100700,2,1,2))return 6;
 unsigned array_guards=4;
 unsigned byte_reads[]={0,1,3,4,UINT32_MAX};
 for(unsigned i=0;i<sizeof byte_reads/sizeof *byte_reads;++i)if(!compare_byteop(m,ram,screen,0x4a,0,0,0,byte_reads[i],0x12345678))return 5;else ++checked;
 if(!compare_byteop(m,ram,screen,0x4a,0x81100700,0,0x81101000,0,0x12345678))return 5;++checked;
 for(unsigned i=0;i<sizeof byte_reads/sizeof *byte_reads;++i)if(!compare_byteop(m,ram,screen,0x4a,0x81100700,4,0x81100740,byte_reads[i],0x12345678))return 5;else ++checked;
 unsigned byte_writes[]={0,3,4,UINT32_MAX};for(unsigned i=0;i<sizeof byte_writes/sizeof *byte_writes;++i)if(!compare_byteop(m,ram,screen,0x4b,0,0,0,byte_writes[i],0x12345680+i))return 5;else ++checked;
 if(!compare_byteop(m,ram,screen,0x4b,0x81100700,0,0x81101000,0,0xabcdef12))return 5;++checked;
 for(unsigned i=0;i<sizeof byte_writes/sizeof *byte_writes;++i)if(!compare_byteop(m,ram,screen,0x4b,0x81100700,4,0x81100740,byte_writes[i],i?0x123456ff:0x12345680))return 5;else ++checked;
 if(!byteop_guard(0x4a,0x81100ffc,4,0x81100740,0,0)||!byteop_guard(0x4a,0x81100700,4,0x81101000,0,0)||!byteop_guard(0x4b,0x81100ffc,4,0x81100740,0,0)||!byteop_guard(0x4b,0x81100700,4,0x81101000,0,0))return 6;
 unsigned byte_guards=4;
 unsigned guards=0;for(unsigned mode=0;mode<3;++mode){uint8_t before[4096];memcpy(before,screen,4096);nflstreet_ui_script s={0},old;s.sp=1;s.pc=0x81100800;screen[0x800]=mode==0?0x0b:mode==1?0x42:0x40;if(mode==2)s.words[0]=UINT32_MAX;memcpy(before,screen,4096);old=s;if(nflstreet_ui_script_run(screen,4096,0x81100000,0x817c0000,&s,1)||memcmp(screen,before,4096)||memcmp(&s,&old,sizeof s))return 6;++guards;}
 for(unsigned guard=0;guard<4;++guard){unsigned p=0x800;uint8_t before[4096];nflstreet_ui_script state={0},prior;state.sp=1;state.pc=0x81100800;
  if(guard<2){screen[p++]=0x65;put(screen+p,guard?257:UINT32_MAX);p+=4;}else if(guard==2){pushcode(screen,&p,1);pushcode(screen,&p,0);screen[p++]=0x5a;}else{pushcode(screen,&p,0x81100000);pushcode(screen,&p,0);screen[p++]=0x5f;put(screen+p,0x700);p+=4;screen[p++]=0xff;screen[p++]=0xfe;put(screen+0x700,1);put(screen+0x704,0);}
  screen[p++]=0x44;memcpy(before,screen,4096);prior=state;if(nflstreet_ui_script_run(screen,4096,0x81100000,0x817c0000,&state,1000)||memcmp(before,screen,4096)||memcmp(&prior,&state,sizeof state))return 6;++guards;
 }
 printf("Original UI integer/finite-float/array-address/byte-descriptor script subset exact: %u full owned-file/256-word-stack/PC/flags comparisons (12 actual title helpers; 12 opcode 6E cases); %u atomic unsupported/budget/invalid-descriptor guards; service opcodes require explicit transactional handlers; no input mapping claim\n",checked,guards+byte_guards+array_guards);free(ram);FreeLibrary(h);return 0;}
