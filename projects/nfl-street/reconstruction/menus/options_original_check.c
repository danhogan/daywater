#include <windows.h>
#include "moderngekko/module_abi.h"
#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int run(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_ui_options*s){
 unsigned fetched=0;c->lr=0;c->pc=0x8012a414;c->exception=0;c->cycle_budget=100000000;
 for(unsigned i=0;c->pc&&i<100000;++i){unsigned fn=c->pc,argc=99,value=0;
 switch(fn){case 0x80004668:argc=0;value=s->query_4668;break;case 0x8014e29c:argc=1;value=s->query_7;break;case 0x801a6178:argc=2;value=s->sound_handle;break;case 0x801440f0:argc=1;value=s->query_440f0;break;case 0x80143fc0:case 0x80143ee0:argc=1;break;case 0x801441f4:argc=1;if(fetched>=2)return 0;value=s->query_441f4[fetched++];break;case 0x801415b0:argc=2;break;}
 if(argc!=99){if(s->count>=64)return 0;s->requests[s->count++]=(nflstreet_ui_options_request){fn,argc,{argc?c->gpr[3]:0,argc>1?c->gpr[4]:0}};c->gpr[3]=value;c->pc=c->lr;}
 else{c->downcount=-1000000000;m->dispatch(c,c->pc);}if(c->exception)return 0;
 }return !c->pc&&fetched==2;
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 for(unsigned pass=0;pass<32;++pass){nflstreet_ui_options native={0},original;native.query_7=pass%4;native.sound_handle=0xa0000000+pass;native.query_440f0=pass%3;native.query_441f4[0]=0x12340000+pass;native.query_441f4[1]=0x56780000+pass;native.word_6da8=0x11111111;native.byte_6db0=0xa5;native.word_6da0=0x22222222;native.word_6dac=0x33333333;native.word_6d9c=0x44444444;native.byte_59f8=0x5a;original=native;
 unsigned r=0x3a9a40;put(ram+r-0x6da8,native.word_6da8);ram[r-0x6db0]=native.byte_6db0;put(ram+r-0x6da0,native.word_6da0);put(ram+r-0x6dac,native.word_6dac);put(ram+r-0x6d9c,native.word_6d9c);ram[r-0x59f8]=native.byte_59f8;put(ram+0x17c0000,0x11223344);
 CPUState c={0};c.ram=ram;c.ram_size=24*1024*1024;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.msr=0x2000;c.gpr[3]=0x80000001;c.gpr[4]=5;c.gpr[5]=0;c.gpr[6]=0;c.gpr[7]=0x817c0010;c.gpr[8]=0x817c0000;
 nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000001;uint32_t result=0x11223344;
 if(!nflstreet_ui_options_service(&native,NULL,0,0,&q,&result)||!run(m,&c,&original))return 5;
 original.word_6da8=be(ram+r-0x6da8);original.byte_6db0=ram[r-0x6db0];original.word_6da0=be(ram+r-0x6da0);original.word_6dac=be(ram+r-0x6dac);original.word_6d9c=be(ram+r-0x6d9c);original.byte_59f8=ram[r-0x59f8];
 if(memcmp(&native,&original,sizeof native)||be(ram+0x17c0000)!=result||native.count!=10){fprintf(stderr,"Options initialization mismatch pass=%u count=%u/%u\n",pass,native.count,original.count);return 6;}}
 for(unsigned guard=0;guard<3;++guard){nflstreet_ui_options s={0},prior;if(guard==0)s.query_4668=1;else if(guard==1)s.count=55;else s.count=65;prior=s;if(nflstreet_ui_options_initialize(&s)||memcmp(&s,&prior,sizeof s))return 7;}
 puts("Original group5/entry0 initialization callback exact:32 controlled cases,6 owned fields,320 ordered service requests,unchanged callback result;query4668=0 only;external service outputs supplied;late capacity/nonzero mode guards atomic");free(ram);FreeLibrary(h);return 0;}
