/* Diagnostic original score construction using a saved match context.
   Writes no files itself and never modifies the input snapshot or module. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_preview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE (24u*1024u*1024u)
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t*load(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(size_t)z>RAM_SIZE||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t z,n;uint8_t*ram=load(argv[2],&z),*db=load(argv[3],&n);uint32_t id,ids[20],count;if(!ram||z!=RAM_SIZE||!db||!nflstreet_team_preview_team_id_from_db(db,n,&id)||!nflstreet_team_preview_roster_from_db(db,n,id,ids,&count))return 4;
 for(unsigned i=0;i<count;++i)put(ram+0x17c0000+i*4,ids[i]);put(ram+0x17c0000+count*4,0x7fff);put(ram+0x17c0100,0);
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x817b0000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=0;c.gpr[4]=0x817c0000;c.gpr[5]=0x817c0100;c.pc=0x80042568;c.lr=0;c.cycle_budget=10000000;unsigned allocs=0,lookups=0;
 for(unsigned i=0;c.pc&&i<5000000;++i){if(c.pc==0x80004668){c.gpr[3]=0;c.pc=c.lr;}else if(c.pc==0x8014e800){uint32_t requested=c.gpr[4],ret=c.lr;c.lr=0;
   for(unsigned nested=0;c.pc&&nested<100000;++nested){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)break;}
   if(c.pc||c.exception||!c.gpr[3]){fprintf(stderr,"Original player lookup failed id%u pc%x ex%x result%u\n",requested,c.pc,c.exception,c.gpr[3]);return 7;}++lookups;c.pc=ret;c.lr=ret;
  }else if(c.pc==0x8018b7b8){if(c.gpr[3]!=count*0x44||allocs++){fprintf(stderr,"Unexpected allocation size%u\n",c.gpr[3]);return 5;}c.gpr[3]=0x817d0000;c.pc=c.lr;}else{c.downcount=-1000000000;m->dispatch(&c,c.pc);}if(c.exception)break;}
 if(c.pc||c.exception||allocs!=1||lookups<count||be(ram+0x17c0100)!=0x817d0000){fprintf(stderr,"Score construction incomplete pc%x ex%x allocations%u\n",c.pc,c.exception,allocs);return 6;}
 printf("Original weighted-score construction completed from match context: supplied actor-gatefalse/team%u formation0, %u rows; DB/profile/gear helpers executed, every intercepted player lookup succeeded, allocator supplied. This is not verified fresh-menu behavior.\n",id,count);
 for(unsigned i=0;i<count;++i){const uint8_t*r=ram+0x17d0000+i*0x44;printf("row %u id%u role%u flags%u scores",i,be(r),be(r+0x40),r[0x3c]);for(unsigned j=0;j<14;++j)printf(" %u",be(r+4+j*4));fputc('\n',stdout);}free(db);free(ram);FreeLibrary(h);return 0;
}
