#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_devices.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define RAM_BASE 0x80000000u
#define R13 0x803a9a40u
#define STACK 0x817df000u
#define OUTPUT 0x817d0200u
#define MAP_ADDR 0x80282e34u

static nflstreet_team_device_record supplied[NFLSTREET_TEAM_DEVICE_MAX_ITERATOR_RECORDS];
static size_t supplied_count,iterator_index;
static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint32_t get32(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t rng=0x4a93c27du;
static uint32_t draw(void){rng=rng*1664525u+1013904223u;return rng;}

static int run_original(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const nflstreet_team_device_record*records,size_t count,uint32_t*result){
 supplied_count=count;iterator_index=0;if(count)memcpy(supplied,records,count*sizeof records[0]);
 CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
 c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=OUTPUT;c.pc=0x8010c078u;
 c.downcount=c.cycle_budget=10000000;
 for(unsigned step=0;c.pc&&step<5000;++step){uint32_t pc=c.pc;c.downcount=-1000000000;
  if(pc==0x80149a54u){c.pc=c.lr;}
  else if(pc==0x801490d0u){iterator_index=0;c.gpr[3]=supplied_count!=0;c.pc=c.lr;}
  else if(pc==0x80149aa0u){if(iterator_index>=supplied_count)return 0;c.gpr[3]=supplied[iterator_index].port_handle;c.pc=c.lr;}
  else if(pc==0x80149ac8u){if(iterator_index>=supplied_count)return 0;c.gpr[3]=supplied[iterator_index].side;c.pc=c.lr;}
  else if(pc==0x8014919cu){if(iterator_index<supplied_count)++iterator_index;c.gpr[3]=iterator_index<supplied_count;c.pc=c.lr;}
  else if(pc==0x80149a80u){c.pc=c.lr;}
  else m->dispatch(&c,pc);
  if(c.exception){fprintf(stderr,"Original device projection exception pc%08x ex%08x\n",c.pc,c.exception);return 0;}
 }
 if(c.pc||c.gpr[1]!=STACK)return 0;
 for(unsigned i=0;i<4;++i)result[i]=get32(ram+OUTPUT-RAM_BASE+i*4u);
 return 1;
}

static int one_case(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const nflstreet_team_device_record*records,size_t count,const uint32_t mapping[10],unsigned*cases){
 uint32_t expected[4],actual[4];memset(expected,0x5a,sizeof expected);memset(actual,0x5a,sizeof actual);
 int supported=nflstreet_team_devices_project(records,count,mapping,expected);
 if(!supported){for(unsigned i=0;i<4;++i)if(expected[i]!=0x5a5a5a5au)return 0;return 1;}
 for(unsigned i=0;i<10;++i)put32(ram+MAP_ADDR-RAM_BASE+i*4u,mapping[i]);
 for(unsigned i=0;i<4;++i)put32(ram+OUTPUT-RAM_BASE+i*4u,0xccccccccu);
 if(!run_original(m,ram,records,count,actual)||memcmp(expected,actual,sizeof expected)){
  fprintf(stderr,"8010C078 projection mismatch records%zu expected",count);for(unsigned i=0;i<4;++i)fprintf(stderr," %08x",expected[i]);
  fprintf(stderr," original");for(unsigned i=0;i<4;++i)fprintf(stderr," %08x",actual[i]);fputc('\n',stderr);return 0;}
 ++*cases;return 1;
}

int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"Usage: team-devices-original-check.exe <original-DLL> <snapshot.mem1>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 FILE*f=fopen(argv[2],"rb");uint8_t*ram=malloc(RAM_SIZE);if(!ram||!f||fread(ram,1,RAM_SIZE,f)!=RAM_SIZE)return 4;fclose(f);
 unsigned cases=0;uint32_t map[10];for(unsigned i=0;i<10;++i)map[i]=UINT32_MAX;
 nflstreet_team_device_record records[64];
 if(!one_case(m,ram,NULL,0,map,&cases))return 5;
 /* All four mapped sides, unsupported sides, remaps, and last-writer behavior. */
 for(unsigned i=0;i<10;++i)map[i]=i<4?i:UINT32_MAX;
 for(unsigned side=0;side<4;++side){records[0].port_handle=side;records[0].side=side&1u?1u:0u;if(!one_case(m,ram,records,1,map,&cases))return 6;}
 records[0]=(nflstreet_team_device_record){0,0};records[1]=(nflstreet_team_device_record){1,1};records[2]=(nflstreet_team_device_record){2,9};records[3]=(nflstreet_team_device_record){3,0};
 if(!one_case(m,ram,records,4,map,&cases))return 7;
 map[0]=2;map[1]=2;records[0]=(nflstreet_team_device_record){0,0};records[1]=(nflstreet_team_device_record){1,1};
 if(!one_case(m,ram,records,2,map,&cases))return 8;
 for(unsigned test=0;test<500;++test){for(unsigned i=0;i<10;++i){uint32_t v=draw()%7;map[i]=v<4?v:v<6?UINT32_MAX:3;}
  size_t count=draw()%25;for(size_t i=0;i<count;++i){records[i].port_handle=draw()%10;records[i].side=draw()%4;}
  if(!one_case(m,ram,records,count,map,&cases))return 9;}
 for(unsigned i=0;i<10;++i)map[i]=UINT32_MAX;map[0]=4;records[0]=(nflstreet_team_device_record){0,0};
 if(!one_case(m,ram,records,1,map,&cases))return 10;
 records[0].port_handle=10;if(!one_case(m,ram,records,1,map,&cases))return 11;
 records[0].port_handle=0;if(!one_case(m,ram,records,65,map,&cases))return 12;
 printf("8010C078 exact for %u supported supplied iterator/mapping cases; 80149a54/801490d0/80149aa0/80149ac8/8014919c/80149a80 iterator results were controlled inputs. No physical device discovery claimed.\n",cases);
 free(ram);FreeLibrary(h);return 0;
}
