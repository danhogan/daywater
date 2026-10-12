#include <windows.h>
#include "moderngekko/module_abi.h"
#include "order.h"
#include "dispatcher.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int compare(const ModernGekkoModuleDesc*m,uint8_t*ram,nflstreet_ui_order*s,unsigned id,int32_t delta){
 nflstreet_ui_order want=*s;if(!nflstreet_ui_order_move(&want,id,delta))return 0;
 memset(ram+0x17f0000,0,0x200);put(ram+0x17f0030,s->active);put(ram+0x17f0038,s->count);put(ram+0x17f003c,s->base);put(ram+0x17f0054,s->rate_count);put(ram+0x17f0058,0x817c0000);put(ram+0x17f0060,s->continuation_count);put(ram+0x17f0064,0x817d0000);
 for(unsigned i=0;i<16;++i)for(unsigned j=0;j<5;++j)put(ram+s->base-0x80000000+i*20+j*4,s->loaded[i][j]);
 for(unsigned i=0;i<64;++i)for(unsigned j=0;j<13;++j)put(ram+0x17c0000+i*52+j*4,s->rates[i][j]);
 for(unsigned i=0;i<8;++i)for(unsigned j=0;j<10;++j)put(ram+0x17d0000+i*40+j*4,s->continuations[i][j]);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=0x817f0000;c.gpr[4]=id>>16;c.gpr[5]=id&65535;c.gpr[6]=(unsigned)delta;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.pc=0x801d463c;c.cycle_budget=10000000;
 for(unsigned step=0;c.pc&&step<100000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;}if(c.pc)return 0;
 if(want.active!=be(ram+0x17f0030))return 0;
 for(unsigned i=0;i<16;++i)for(unsigned j=0;j<5;++j)if(want.loaded[i][j]!=be(ram+s->base-0x80000000+i*20+j*4))return 0;
 for(unsigned i=0;i<64;++i)for(unsigned j=0;j<13;++j)if(want.rates[i][j]!=be(ram+0x17c0000+i*52+j*4))return 0;
 for(unsigned i=0;i<8;++i)for(unsigned j=0;j<10;++j)if(want.continuations[i][j]!=be(ram+0x17d0000+i*40+j*4))return 0;
 *s=want;return 1;
}
static void seed(nflstreet_ui_order*s,unsigned count,unsigned active,unsigned refs,unsigned salt){
 memset(s,0,sizeof*s);s->base=0x817e0000;s->count=count;s->active=active;s->rate_count=refs?16:0;s->continuation_count=refs?8:0;
 for(unsigned i=0;i<16;++i)for(unsigned j=0;j<5;++j)s->loaded[i][j]=j==1?((i%3)<<16)|(i+5):0x12345678+i*73+j*19+salt;
 for(unsigned i=0;i<64;++i)for(unsigned j=0;j<13;++j)s->rates[i][j]=j==7&&i<16?(i<12?s->base+(i%16)*20:0x12340000+i):0xc0010000+i*29+j+salt;
 for(unsigned i=0;i<8;++i)for(unsigned j=0;j<10;++j)s->continuations[i][j]=j==6?(i<6?s->base+(i%16)*20:0x55550000+i):0xb0010000+i*31+j+salt;
}
int main(int argc,char**argv){if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 unsigned counts[]={0,1,4,16},active[]={0,1,3,UINT32_MAX},checked=0;int32_t deltas[]={-100,-2,-1,0,1,2,100,INT32_MIN,INT32_MAX};
 for(unsigned n=0;n<4;++n)for(unsigned a=0;a<4;++a)for(unsigned ref=0;ref<2;++ref)for(unsigned t=0;t<5;++t)for(unsigned d=0;d<9;++d){nflstreet_ui_order s;seed(&s,counts[n],active[a],ref,checked);unsigned id=t<4?s.loaded[t][1]:0xabcdefff;if(!compare(m,ram,&s,id,deltas[d])){fprintf(stderr,"Order mismatch count%u active%x refs%u target%x delta%d\n",counts[n],active[a],ref,id,deltas[d]);return 5;}++checked;}
 uint8_t file[1257];f=fopen(argv[3],"rb");if(!f||fread(file,1,sizeof file,f)!=sizeof file)return 4;fclose(f);unsigned relocated,flags=0;if(!nflstreet_ui_relocate(file,sizeof file,0x81100000,&relocated))return 6;nflstreet_ui_dispatcher dispatch={0};nflstreet_ui_order joined;seed(&joined,4,0,1,0);for(unsigned i=0;i<4;++i)joined.loaded[i][1]=i+5;
 unsigned events[]={0xfffe,1,1,2,2,1,1};unsigned joined_count=0;
 for(unsigned i=0;i<7;++i){unsigned first=dispatch.count;if(!nflstreet_ui_quick_setup_event(file,sizeof file,0x81100000,&flags,&dispatch,events[i],0x76,UINT32_MAX))return 6;for(unsigned j=first;j<dispatch.count;++j)if(dispatch.requests[j].property==8){nflstreet_ui_order applied=joined;if(!nflstreet_ui_order_request(&applied,&dispatch.requests[j])||!compare(m,ram,&joined,dispatch.requests[j].target,(int32_t)dispatch.requests[j].aux)||memcmp(&applied,&joined,sizeof joined))return 7;++joined_count;}}
 nflstreet_ui_order guard,before;seed(&guard,4,0,1,0);guard.rate_count=65;before=guard;if(nflstreet_ui_order_move(&guard,6,-2)||memcmp(&guard,&before,sizeof guard))return 8;seed(&guard,4,0,1,0);before=guard;nflstreet_ui_service_request invalid={0};invalid.kind=9;invalid.property=0;invalid.target=6;if(nflstreet_ui_order_request(&guard,&invalid)||memcmp(&guard,&before,sizeof guard))return 8;
 printf("Original loaded-screen ordering exact:%u complete record/rate/continuation comparisons,%u applied Quick Game source requests;active index and all opaque words preserved;bounds rollback passed;loaded records supplied,recipients not loaded\n",checked,joined_count);free(ram);FreeLibrary(h);return 0;}
