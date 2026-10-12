/* Controlled oracle for the original preview body's TBYP -> pool-key path. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <moderngekko/module_abi.h>

#define RAM_SIZE (24u * 1024u * 1024u)
#define ACTOR 0x81700000u
#define STACK 0x817df000u
#define R13 0x803a9a40u
#define TYPE0_TABLE 0x802ab1e8u
static uint32_t get32(const uint8_t *p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static uint8_t *at(uint8_t *ram, uint32_t a) { return a < 0x80000000u || a-0x80000000u >= RAM_SIZE ? NULL : ram+a-0x80000000u; }
static void put32(uint8_t *p,uint32_t x){p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;}

int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: body-route-check <original-module.so> <private-tbyp-values.txt>\n");return 2;}
 void*lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);if(!lib){fprintf(stderr,"dlopen failed\n");return 3;}
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)dlsym(lib,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=calloc(1,RAM_SIZE);if(!ram)return 4;CPUState c={0};
 FILE*f=fopen(argv[2],"rb");if(!f)return 4;char*line=NULL;size_t cap=0;ssize_t n;unsigned cases=0,redirected7=0;
 while((n=getline(&line,&cap,f))>=0){char*end=NULL;long value=strtol(line,&end,10);if(end==line||value<0||value>65535)return 5;
  uint8_t*actor=at(ram,ACTOR);memset(actor,0,0x500);memset(&c,0,sizeof c);c.ram=ram;c.ram_size=RAM_SIZE;c.pc=0x8004221cu;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=0;c.gpr[4]=(uint32_t)value;c.downcount=c.cycle_budget=100000000;
  /* 80041E18(slot) reads the active object-pointer array from r13-0x7BBC. */
  put32(at(ram,R13-0x7bbcu),0x81701000u);put32(at(ram,0x81701000u),ACTOR);
  for(unsigned step=0;c.pc&&step<10000;step++){c.downcount=c.cycle_budget=100000000;m->dispatch(&c,c.pc);if(c.exception)return 6;}
  uint32_t expected=(uint32_t)value==7?2u:(uint32_t)value;uint8_t*key=at(ram,ACTOR+0x418);uint8_t*record=at(ram,ACTOR+0x41c);uint32_t expected_record=TYPE0_TABLE+expected*0x14u;
  if(c.pc||!key||!record||get32(key)!=expected||get32(record)!=expected_record){fprintf(stderr,"original body route mismatch case=%u input=%ld actual_key=%08x actual_record=%08x expected_key=%08x expected_record=%08x pc=%08x exception=%08x\n",cases,value,key?get32(key):0,record?get32(record):0,expected,expected_record,c.pc,c.exception);return 7;}
  if(value==7)redirected7++;cases++;
 }
 fclose(f);free(line);if(!cases||!redirected7)return 8;
 printf("Untouched DOL 8004221C body route passed: %u supplied PLAY TBYP values; verified actor+418 key and actor+41C type-0 record pointer; TBYP 7 override observed.\n",cases);
 free(ram);dlclose(lib);return 0;
}
