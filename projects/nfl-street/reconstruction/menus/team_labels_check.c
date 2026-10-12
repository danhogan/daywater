#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_callback.h"
#include "team_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define ARG_AT 0x817d2000u
#define DESC_AT 0x817d3000u
#define DEST_AT 0x817d4000u
#define OUT_AT 0x817d4100u
#define ROW_AT 0x817d5000u
#define RESULT_AT 0x817d6000u
#define ORIG_STACK 0x817df800u
#define STACK 0x81801000u
#define SENTINEL 0xabcddcbau

typedef struct {nflstreet_team_record record;uint8_t row[4096];size_t row_size;nflstreet_team_field name;} raw_team;
static raw_team teams[32];

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t*load_file(const char*path,size_t*size,size_t max){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(size_t)z>max||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}
static int load_teams(const char*dir){
 for(unsigned entry=1;entry<=32;++entry){char path[4096];int n=snprintf(path,sizeof path,"%s/%05u.bin",dir,entry);if(n<0||n>=(int)sizeof path)return 0;
  size_t z=0;uint8_t*b=load_file(path,&z,8u*1024u*1024u);nflstreet_team_table t;if(!b||!nflstreet_team_table_open(b,z,&t)||t.used!=1||t.record_bytes>sizeof teams[0].row){free(b);return 0;}
  raw_team*r=&teams[entry-1];r->row_size=t.record_bytes;memcpy(r->row,b+t.records_at,t.record_bytes);int has_name=0;
  for(unsigned i=0;i<t.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&t,i,&f)){free(b);return 0;}if(f.tag==0x414e4454){if(f.type!=0||(f.bit_offset&7)||(f.width&7)){free(b);return 0;}r->name=f;has_name=1;}}
  nflstreet_team_catalog one={0};if(!nflstreet_team_catalog_append(&one,&t)||one.count!=1||!has_name){free(b);return 0;}r->record=one.records[0];free(b);
 }
 return 1;
}
typedef struct {const raw_team*team;uint32_t requested_id;unsigned numeric_calls,name_calls;} supplied_db;
static int dispatch_original(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,supplied_db*db){
 c->pc=0x8011e9d4;c->lr=0;c->exception=0;c->cycle_budget=10000000;
 c->gpr[3]=0x80000003u;c->gpr[4]=ARG_AT;c->gpr[5]=0;c->gpr[6]=RESULT_AT;
 for(unsigned step=0;c->pc&&step<200000;++step){uint32_t pc=c->pc;
  if(pc==0x80150808){if(c->gpr[4]!=0||!c->gpr[3]){fprintf(stderr,"Query open args %08x %08x\n",c->gpr[3],c->gpr[4]);return 0;}c->pc=c->lr;}
  else if(pc==0x8014947c){if(c->gpr[4]!=0x44494754||c->gpr[5]!=db->requested_id||c->gpr[6]!=0||!c->gpr[3]){fprintf(stderr,"Team lookup args %08x %08x %08x %08x %08x id%u\n",c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6],c->gpr[7],db->requested_id);return 0;}c->gpr[3]=1;c->pc=c->lr;}
  else if(pc==0x801496c4){if(c->gpr[4]!=0x414e4454||!c->gpr[5]||c->gpr[6]>4096){fprintf(stderr,"Name query args %08x %08x %08x %08x\n",c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6]);return 0;}
   uint32_t ret=c->lr,oldpc=c->pc;uint32_t oldlr=c->lr;db->name_calls++;
   c->gpr[3]=ROW_AT;c->gpr[4]=db->team->name.bit_offset;c->gpr[5]=db->team->name.width;c->gpr[6]=c->gpr[5];
   /* Restore the caller's destination/capacity after retaining them above. */
   uint32_t dest=be(ram+0x17d3008),cap=be(ram+0x17d3004);c->gpr[6]=dest;c->gpr[7]=cap;c->lr=0;c->pc=0x801c4388;
   for(unsigned nested=0;c->pc&&nested<10000;++nested){c->downcount=-1000000000;m->dispatch(c,c->pc);if(c->exception)return 0;}
   if(c->pc)return 0;c->pc=ret;c->lr=oldlr;(void)oldpc;
  }
  else if(pc==0x80149614){if(c->gpr[4]!=0x464f5254&&c->gpr[4]!=0x45445254&&c->gpr[4]!=0x564f5254)return 0;
   uint32_t index=c->gpr[4]==0x464f5254?0:c->gpr[4]==0x45445254?1:2;c->gpr[3]=db->team->record.fields[index];++db->numeric_calls;c->pc=c->lr;}
  else if(pc==0x80149064){c->pc=c->lr;}
  else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Original team callback exception pc=%08x ex=%08x\n",c->pc,c->exception);return 0;}
 }
 if(c->pc){fprintf(stderr,"Original team callback stopped pc=%08x lr=%08x\n",c->pc,c->lr);return 0;}
 if(c->gpr[3]!=1||db->name_calls!=1||db->numeric_calls!=3){fprintf(stderr,"Callback terminal r3=%08x name=%u numeric=%u\n",c->gpr[3],db->name_calls,db->numeric_calls);return 0;}return 1;
}

static void seed_callback(uint8_t*ram,const raw_team*t,unsigned cap){
 memset(ram+0x17d2000,0xa5,0x5000);memcpy(ram+0x17d5000,t->row,t->row_size);
 put(ram+0x17d2000,t->record.id);put(ram+0x17d2004,DESC_AT);put(ram+0x17d2008,OUT_AT);put(ram+0x17d200c,OUT_AT+4);put(ram+0x17d2010,OUT_AT+8);
 put(ram+0x17d3000,0);put(ram+0x17d3004,cap);put(ram+0x17d3008,DEST_AT);
 put(ram+0x17d6000,SENTINEL);for(unsigned i=0;i<3;++i)put(ram+0x17d4100+i*4,0xa5a5a5a5u);
}
static int compare_case(const ModernGekkoModuleDesc*m,uint8_t*baseline,const raw_team*t,unsigned cap){
 uint8_t*actual=malloc(RAM_SIZE),*native_ram=malloc(RAM_SIZE);if(!actual||!native_ram)return 0;memcpy(actual,baseline,RAM_SIZE);memcpy(native_ram,baseline,RAM_SIZE);
 seed_callback(actual,t,cap);seed_callback(native_ram,t,cap);
 CPUState c={0};c.ram=actual;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=ORIG_STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;
 supplied_db db={t,t->record.id,0,0};if(!dispatch_original(m,&c,actual,&db)){fprintf(stderr,"Original callback unsupported team%u cap%u\n",t->record.id,cap);free(actual);free(native_ram);return 0;}
 nflstreet_team_callback state={0};state.catalog.count=1;state.catalog.records[0]=t->record;
 nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000003u;q.argc=5;q.args[0]=t->record.id;q.args[1]=DESC_AT;q.args[2]=OUT_AT;q.args[3]=OUT_AT+4;q.args[4]=OUT_AT+8;
 uint32_t stack_words[256];for(unsigned i=0;i<256;++i)stack_words[i]=0xa5a5a5a5u;uint32_t result=SENTINEL;
 if(!nflstreet_team_callback_memory(&state,native_ram,RAM_SIZE,0x80000000u,&q,&result,STACK,stack_words)||result!=SENTINEL){fprintf(stderr,"Native callback rejected team%u cap%u\n",t->record.id,cap);free(actual);free(native_ram);return 0;}
 for(unsigned i=0;i<256;++i)if(stack_words[i]!=0xa5a5a5a5u){fprintf(stderr,"Native stack sentinel changed team%u cap%u word%u\n",t->record.id,cap,i);free(actual);free(native_ram);return 0;}
 if(memcmp(actual+0x17d2000,native_ram+0x17d2000,0x220)){fprintf(stderr,"Input/output region differs team%u cap%u\n",t->record.id,cap);free(actual);free(native_ram);return 0;}
 if(c.gpr[1]!=ORIG_STACK||be(actual+0x17d6000)!=SENTINEL||be(native_ram+0x17d6000)!=SENTINEL){fprintf(stderr,"Stack/result retention differs team%u cap%u\n",t->record.id,cap);free(actual);free(native_ram);return 0;}
 for(unsigned i=0;i<cap;++i){uint8_t expected=i<(t->name.width/8)?t->row[t->name.bit_offset/8+i]:0;if(actual[DEST_AT-0x80000000+i]!=expected||native_ram[DEST_AT-0x80000000+i]!=expected){fprintf(stderr,"Name output mismatch team%u cap%u byte%u actual%02x native%02x expected%02x\n",t->record.id,cap,i,actual[DEST_AT-0x80000000+i],native_ram[DEST_AT-0x80000000+i],expected);free(actual);free(native_ram);return 0;}}
 for(unsigned i=cap;i<128;++i)if(actual[DEST_AT-0x80000000+i]!=0xa5||native_ram[DEST_AT-0x80000000+i]!=0xa5){fprintf(stderr,"Destination guard changed team%u cap%u byte%u\n",t->record.id,cap,i);free(actual);free(native_ram);return 0;}
 free(actual);free(native_ram);return 1;
}

static int native_mixed_case(const raw_team*t){
 uint8_t*ram=calloc(1,RAM_SIZE);if(!ram)return 0;nflstreet_team_callback state={0};state.catalog.count=1;state.catalog.records[0]=t->record;
 put(ram+0x17d3004,64);put(ram+0x17d3008,DEST_AT);nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000003u;q.argc=5;q.args[0]=t->record.id;q.args[1]=DESC_AT;q.args[2]=STACK;q.args[3]=STACK+4;q.args[4]=STACK+8;
 uint32_t words[256]={0},result=SENTINEL;int ok=nflstreet_team_callback_memory(&state,ram,RAM_SIZE,0x80000000u,&q,&result,STACK,words);
 if(ok){if(words[0]!=t->record.fields[0]||words[1]!=t->record.fields[1]||words[2]!=t->record.fields[2])ok=0;for(unsigned i=0;i<64;++i){uint8_t expected=i<t->record.name_bytes?(uint8_t)t->record.name[i]:0;if(ram[DEST_AT-0x80000000+i]!=expected)ok=0;}}
 free(ram);return ok;
}

static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}
int main(int argc,char**argv){
 if(argc!=4){fprintf(stderr,"Usage: team-labels-check.exe <original-DLL> <snapshot.mem1> <db_teams-folder>\n");return 2;}
 if(!load_teams(argv[3])){fprintf(stderr,"Could not load 32 private team records\n");return 4;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t z=0;uint8_t*baseline=load_file(argv[2],&z,RAM_SIZE);if(!baseline||z!=RAM_SIZE)return 4;uint64_t before=hash_bytes(baseline,z);unsigned cases=0;
 for(unsigned i=0;i<32;++i){const raw_team*t=&teams[i];unsigned width=t->name.width/8;unsigned caps[7]={0,1,width?width-1:0,width,width+1,64,64};
  for(unsigned j=0;j<7;++j){if(caps[j]>128)return 5;if(!compare_case(m,baseline,t,caps[j]))return 6;++cases;}
 }
 if(!native_mixed_case(&teams[0])){fprintf(stderr,"Native mixed RAM/stack output case failed\n");return 7;}
 size_t after_size=0;uint8_t*after=load_file(argv[2],&after_size,RAM_SIZE);if(!after||after_size!=z||hash_bytes(after,after_size)!=before){fprintf(stderr,"Snapshot baseline changed during verification\n");return 8;}
 printf("Team label original callback exact: %u cases across 32 actual records; 8011E9D4/8011CDB4/80150944/80150C7C/CA4/CCC executed, original 801C4388 copied actual ANDT bitfields; raw archive query/numeric field responses supplied. Input descriptor/arguments, full destination and guards, numeric outputs, result sentinel and SP retained. Native mixed RAM/stack outputs pass separately. Baseline FNV64 %016llx unchanged. Full screen integration not claimed.\n",cases,(unsigned long long)before);
 free(after);free(baseline);FreeLibrary(h);return 0;
}
