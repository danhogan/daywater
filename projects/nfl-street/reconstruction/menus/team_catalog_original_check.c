#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE (24u*1024u*1024u)
#define ROW 0x817e0000u
#define OUTPUT 0x817e2000u
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t *load(const char*path,size_t *size){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<=0||n>(long)RAM_SIZE){fclose(f);return NULL;}rewind(f);uint8_t*b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return b;}
static int original(const ModernGekkoModuleDesc*m,uint8_t*ram,nflstreet_team_field f){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;
 c.gpr[3]=ROW;c.gpr[4]=f.bit_offset;c.gpr[5]=f.width;c.gpr[6]=OUTPUT;c.gpr[7]=f.type==0?f.width/8:0;
 c.pc=f.type==0?0x801c4388:0x801b94c8;c.cycle_budget=10000000;
 for(unsigned step=0;c.pc&&step<10000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;}
 return c.pc==0;
}
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t z=0;uint8_t*ram=load(argv[2],&z);if(!ram||z!=RAM_SIZE)return 4;
 unsigned numbers=0,texts=0;uint32_t ids[32]={0};
 for(unsigned entry=1;entry<=32;++entry){char path[4096];if(snprintf(path,sizeof path,"%s/%05u.bin",argv[3],entry)>=(int)sizeof path)return 4;
  size_t n=0;uint8_t*b=load(path,&n);nflstreet_team_table t;if(!b||!nflstreet_team_table_open(b,n,&t)||t.used!=1||t.record_bytes>4096)return 5;
  memcpy(ram+ROW-0x80000000,b+t.records_at,t.record_bytes);memset(ram+ROW-0x80000000+t.record_bytes,0,8);
  char team[256]={0};uint32_t id=UINT32_MAX;
  for(unsigned i=0;i<t.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&t,i,&f)||f.width>2040)return 5;
   memset(ram+OUTPUT-0x80000000,0xa5,256);if(!original(m,ram,f)){fprintf(stderr,"Original field dispatch failed entry%u field%u\n",entry,i);return 6;}
   if(f.type==3){uint32_t value;if(!nflstreet_team_table_number(&t,0,i,&value)||value!=be(ram+OUTPUT-0x80000000)){fprintf(stderr,"Numeric field differs entry%u tag%08x\n",entry,f.tag);return 7;}++numbers;if(f.tag==0x44494754)id=value;
    for(unsigned k=4;k<256;++k)if(ram[OUTPUT-0x80000000+k]!=0xa5)return 7;
   }else if(f.type==0){char text[256];size_t len=f.width/8;if(!nflstreet_team_table_text(&t,0,i,text,sizeof text)||memcmp(text,ram+OUTPUT-0x80000000,len))return 7;++texts;
    for(size_t k=len;k<256;++k)if(ram[OUTPUT-0x80000000+k]!=0xa5)return 7;
    if(f.tag==0x414e4454)memcpy(team,text,len+1);
   }else return 5;
  }
  if(id==UINT32_MAX||!team[0])return 8;for(unsigned i=0;i<entry-1;++i)if(ids[i]==id)return 8;ids[entry-1]=id;
  printf("Team archive entry%u: DIGT=%u ANDT=%s\n",entry,id,team);
  /* Truncation, unsupported version and impossible field widths reject. */
  nflstreet_team_table rejected={0};if(nflstreet_team_table_open(b,n-1,&rejected))return 9;
  uint8_t version=b[3];b[3]=9;if(nflstreet_team_table_open(b,n,&rejected))return 9;b[3]=version;
  uint8_t saved[4];memcpy(saved,b+t.fields_at+12,4);memset(b+t.fields_at+12,0xff,4);if(nflstreet_team_table_open(b,n,&rejected))return 9;memcpy(b+t.fields_at+12,saved,4);
  free(b);
 }
 printf("Team catalog exact:32 original private DB blocks; %u numeric and %u text fields match original801B94C8/801C4388; bounded malformed-block rejection. No archive loading, sorting, profile filtering or team-screen startup claimed.\n",numbers,texts);
 free(ram);FreeLibrary(h);return 0;
}
