#include <windows.h>
#include "moderngekko/module_abi.h"
#include "field_cache.h"
#include "field_catalog.h"
#include "team_catalog.h"
#include "field_bootstrap.h"
#include "field_stat_text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define RECORDS 0x817e0000u
#define DEST0 0x817e1000u
#define DEST1 0x817e1100u
#define DEST2 0x817e1200u
#define DEST3 0x817e1300u
#define VALUE 0x817e1400u
#define R13 0x817f0000u
#define STACK 0x817df000u

static void put32(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t*at(uint8_t*ram,uint32_t a){if(a<0x80000000u||(size_t)(a-0x80000000u)>=RAM_SIZE)return NULL;return ram+a-0x80000000u;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c){c->pc=0x8011f1f0;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=100000000;for(unsigned i=0;c->pc&&i<10000;++i){m->dispatch(c,c->pc);if(c->exception)return 0;}return !c->pc&&!c->exception;}
static int read_all(const char*path,uint8_t**out,size_t*size){FILE*f=fopen(path,"rb");if(!f)return 0;if(fseek(f,0,SEEK_END)){fclose(f);return 0;}long n=ftell(f);if(n<0||n>16*1024*1024){fclose(f);return 0;}rewind(f);uint8_t*b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return 0;}fclose(f);*out=b;*size=(size_t)n;return 1;}

/* Execute the original primitive DB readers on actual RVNE record bytes.
   Scratch RAM is supplied; no original query/open/profile service is replaced. */
static int compare_raw_fields(const ModernGekkoModuleDesc*m,uint8_t*ram,
                              const uint8_t*bytes,size_t size){
 nflstreet_team_table table;
 if(!nflstreet_team_table_open_tag(bytes,size,0x52564e45u,&table)||table.used!=8||table.record_bytes>4096)return 0;
 unsigned numbers=0,texts=0;
 for(unsigned row=0;row<table.used;++row){
  memcpy(at(ram,RECORDS),bytes+table.records_at+(size_t)row*table.record_bytes,table.record_bytes);
  memset(at(ram,RECORDS)+table.record_bytes,0,8);
  for(unsigned i=0;i<table.field_count;++i){
   nflstreet_team_field f;
   if(!nflstreet_team_table_field(&table,i,&f))return 0;
   /* Compare only the eight fields consumed by field_catalog.c. Other RVNE
      types (including type4) are outside its contract. */
   if(f.tag!=0x44495645u&&f.tag!=0x4c545645u&&f.tag!=0x524f5645u&&
      f.tag!=0x53445645u&&f.tag!=0x544f5645u&&f.tag!=0x4d4e5645u&&
      f.tag!=0x4f4c5645u&&f.tag!=0x45445645u)continue;
   if(f.width>2040||(f.type!=0&&f.type!=3)){fprintf(stderr,"unsupported consumed RVNE field row%u tag%08x type%u width%u\n",row,f.tag,f.type,f.width);return 0;}
   memset(at(ram,DEST0),0xa5,256);
   CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
   c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=R13;
   c.gpr[3]=RECORDS;c.gpr[4]=f.bit_offset;c.gpr[5]=f.width;
   c.gpr[6]=DEST0;c.gpr[7]=f.type==0?f.width/8:0;
   c.pc=f.type==0?0x801c4388:0x801b94c8;c.cycle_budget=10000000;
   for(unsigned step=0;c.pc&&step<10000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"original RVNE reader exception row%u tag%08x pc%08x ex%08x\n",row,f.tag,c.pc,c.exception);return 0;}}
   if(c.pc){fprintf(stderr,"original RVNE reader unfinished row%u tag%08x pc%08x\n",row,f.tag,c.pc);return 0;}
   size_t written;
   if(f.type==3){
    uint32_t value;if(!nflstreet_team_table_number(&table,row,i,&value)||value!=be32(at(ram,DEST0))){fprintf(stderr,"RVNE number differs row%u tag%08x\n",row,f.tag);return 0;}
    ++numbers;written=4;
   }else{
    char text[256];written=f.width/8;
    if(!nflstreet_team_table_text(&table,row,i,text,sizeof text)||memcmp(text,at(ram,DEST0),written)){fprintf(stderr,"RVNE text differs row%u tag%08x\n",row,f.tag);return 0;}
    ++texts;
   }
   for(size_t k=written;k<256;++k)if(at(ram,DEST0)[k]!=0xa5)return 0;
  }
 }
 printf("Actual RVNE consumed-field extraction matched original801B94C8/801C4388: %u numeric and %u text fields across eight rows; output tails preserved; other RVNE fields excluded.\n",numbers,texts);
 return numbers==40&&texts==24;
}

static int compare_slot_order(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const uint8_t*dol,size_t size,const nflstreet_field_catalog_entry catalog[8]){
 nflstreet_field_bootstrap native;
 if(!nflstreet_field_bootstrap_from_dol(dol,size,catalog,&native))return 0;
 for(unsigned i=0;i<18;++i){uint32_t off=be32(dol+i*4),address=be32(dol+0x48+i*4),length=be32(dol+0x90+i*4);if(length)memcpy(at(ram,address),dol+off,length);}
 const uint32_t original_r13=0x803a9a40u;
 const int32_t values[]={0,1,999,1000,1001,999999,1000000,2147483647,-1,-999,-1000,-1001,-2147483647};
 for(unsigned value=0;value<sizeof values/sizeof values[0];++value){
  char native_text[33];if(!nflstreet_field_stat_text(values[value],native_text))return 0;
  memset(at(ram,DEST3),0xa5,128);
  CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=original_r13;
  c.gpr[3]=(uint32_t)values[value];c.gpr[4]=DEST3;c.gpr[5]=32;c.pc=0x80129b24;c.cycle_budget=100000000;
  for(unsigned step=0;c.pc&&step<1000000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;}
  size_t length=strlen(native_text)+1;
  if(c.pc||memcmp(at(ram,DEST3),native_text,length)){fprintf(stderr,"Original field stat text differs value%d pc%08x\n",values[value],c.pc);return 0;}
  for(size_t i=length;i<128;++i)if(at(ram,DEST3)[i]!=0xa5)return 0;
 }
 puts("Original80129B24 numeric field text matches13 signed/thousand-boundary cases; statistic values supplied, INT32_MIN excluded.");
 for(unsigned slot=0;slot<8;++slot){
  memset(at(ram,RECORDS),0,8*NFLSTREET_FIELD_CACHE_RECORD_SIZE);
  put32(at(ram,original_r13-0x6e08u),RECORDS);
  CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=original_r13;
  c.gpr[3]=slot;c.gpr[4]=DEST0;c.gpr[5]=32;c.gpr[6]=DEST1;c.gpr[7]=32;c.gpr[8]=DEST2;c.gpr[9]=32;c.gpr[10]=VALUE;
  put32(at(ram,STACK+8),DEST3);put32(at(ram,STACK+12),101);
  c.pc=0x8011f1f0;c.cycle_budget=10000000;
  for(unsigned step=0;c.pc!=0x8014b4f0&&step<10000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(!c.pc||c.exception)return 0;}
  if(c.pc!=0x8014b4f0||c.gpr[3]!=native.keys[slot])return 0;
  uint8_t*record=native.records+slot*NFLSTREET_FIELD_CACHE_RECORD_SIZE;
  const uint8_t*expected=catalog[native.keys[slot]].record;
  if(memcmp(record,expected,99)||memcmp(record+201,expected+201,4))return 0;
  if(native.keys[slot]){if(memcmp(record+99,expected+99,102))return 0;}
  else{const uint8_t*description=at(ram,0x80256e40u);const uint8_t*end=memchr(description,0,102);if(!end||memcmp(record+99,description,(size_t)(end-description)+1))return 0;}
 }
 nflstreet_field_bootstrap guard,before;memset(&guard,0xa5,sizeof guard);before=guard;
 if(nflstreet_field_bootstrap_from_dol(dol,0xe3,catalog,&guard)||memcmp(&guard,&before,sizeof guard))return 0;
 puts("Original8011F1F0 cache-miss key selection matches DOL-backed native slot order for eight slots; key0 description bytes retained from DOL. Query/profile/cache-fill execution excluded.");
 return 1;
}

int main(int argc,char**argv){
 if(argc!=3&&argc!=4){fprintf(stderr,"usage: field-catalog-original-check <original.dll> <private-template-00002.bin> [original-main.dol]\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*template_bytes=NULL;size_t template_size=0;if(!read_all(argv[2],&template_bytes,&template_size))return 4;
 char text42[8][33];uint8_t c9[8];for(unsigned i=0;i<8;++i){memset(text42[i],0,sizeof text42[i]);snprintf(text42[i],sizeof text42[i],"supplied-value-%u",i);c9[i]=(uint8_t)(i&1u);}
 nflstreet_field_catalog_entry entries[8];memset(entries,0xa5,sizeof entries);
 if(!nflstreet_field_catalog_from_template(template_bytes,template_size,text42,c9,entries)){fprintf(stderr,"RVNE catalog parse failed\n");return 5;}
 nflstreet_field_catalog_entry guarded[8],guard_before[8];memset(guarded,0xa5,sizeof guarded);memcpy(guard_before,guarded,sizeof guarded);
 if(nflstreet_field_catalog_from_template(template_bytes,template_size-1,text42,c9,guarded)||memcmp(guarded,guard_before,sizeof guarded)){fprintf(stderr,"short-template atomic guard failed\n");return 5;}
 uint8_t cache_records[8*NFLSTREET_FIELD_CACHE_RECORD_SIZE];for(unsigned i=0;i<8;++i)memcpy(cache_records+i*NFLSTREET_FIELD_CACHE_RECORD_SIZE,entries[i].record,NFLSTREET_FIELD_CACHE_RECORD_SIZE);
 uint8_t*ram=calloc(1,RAM_SIZE);if(!ram)return 6;CPUState cpu={0};unsigned checks=0;
 if(!compare_raw_fields(m,ram,template_bytes,template_size)){fprintf(stderr,"RVNE primitive extraction comparison failed\n");return 6;}
 if(argc==4){uint8_t*dol=NULL;size_t size=0;if(!read_all(argv[3],&dol,&size)||!compare_slot_order(m,ram,dol,size,entries)){fprintf(stderr,"Original field slot order comparison failed\n");return 6;}free(dol);}
 for(unsigned key=0;key<8;++key){
  if(entries[key].DIVE!=key||entries[key].record[0xca]!=1||entries[key].record[0xc9]!=c9[key])return 7;
  nflstreet_field_cache_output native={0};memset(native.text0,0x11,sizeof native.text0);memset(native.text21,0x22,sizeof native.text21);memset(native.text42,0x33,sizeof native.text42);memset(native.text99,0x44,sizeof native.text99);native.value_c9=0xa5;native.selected_index=0xfeedbeefu;
  if(!nflstreet_field_cache_read(cache_records,sizeof cache_records,8,key,&native))return 7;
  uint8_t*records=at(ram,RECORDS);memcpy(records,cache_records,sizeof cache_records);
  put32(at(ram,R13-0x6e08u),RECORDS);put32(at(ram,R13-0x6e04u),0xdeadbeefu);
  memset(at(ram,DEST0),0x11,96);memset(at(ram,DEST1),0x22,96);memset(at(ram,DEST2),0x33,96);memset(at(ram,DEST3),0x44,128);memset(at(ram,VALUE),0x55,8);
  uint8_t tails[4][128];memcpy(tails[0],at(ram,DEST0),96);memcpy(tails[1],at(ram,DEST1),96);memcpy(tails[2],at(ram,DEST2),96);memcpy(tails[3],at(ram,DEST3),128);
  memset(&cpu,0,sizeof cpu);cpu.ram=ram;cpu.ram_size=RAM_SIZE;cpu.gpr[1]=STACK;cpu.gpr[2]=0x803b9a40;cpu.gpr[13]=R13;cpu.gpr[3]=key;cpu.gpr[4]=DEST0;cpu.gpr[5]=32;cpu.gpr[6]=DEST1;cpu.gpr[7]=32;cpu.gpr[8]=DEST2;cpu.gpr[9]=32;cpu.gpr[10]=VALUE;put32(at(ram,STACK+8),DEST3);put32(at(ram,STACK+12),101);
  if(!call(m,&cpu)){fprintf(stderr,"original cached reader failed DIVE=%u pc=%08x ex=%08x\n",key,cpu.pc,cpu.exception);return 8;}
  if(be32(at(ram,R13-0x6e04u))!=key||be32(at(ram,VALUE))!=c9[key]||memcmp(native.text0,at(ram,DEST0),33)||memcmp(native.text21,at(ram,DEST1),33)||memcmp(native.text42,at(ram,DEST2),33)||memcmp(native.text99,at(ram,DEST3),102)){fprintf(stderr,"cmp key%u sel=%08x val=%08x c9=%02x d0=%02x/%02x d21=%02x/%02x d42=%02x/%02x d99=%02x/%02x\n",key,be32(at(ram,R13-0x6e04u)),be32(at(ram,VALUE)),c9[key],native.text0[0],at(ram,DEST0)[0],native.text21[0],at(ram,DEST1)[0],native.text42[0],at(ram,DEST2)[0],native.text99[0],at(ram,DEST3)[0]);return 9;}
  if(memcmp(tails[0]+33,at(ram,DEST0)+33,63)||memcmp(tails[1]+33,at(ram,DEST1)+33,63)||memcmp(tails[2]+33,at(ram,DEST2)+33,63)||memcmp(tails[3]+102,at(ram,DEST3)+102,26))return 10;
  checks++;
 }
 printf("RVNE field catalog -> native205-byte cache records -> untouched original8011F1F0 cached reader matched for %u raw DIVE keys. MNVE/OLVE/EDVE byte spans and DIVE key come from supplied private template; offset0x42 text and C9 value are explicit caller inputs. No DIVE-name meanings, formatted-text source, profile/unlock query, or original cache-miss initialization is claimed.\n",checks);
 free(ram);free(template_bytes);FreeLibrary(h);return 0;
}
