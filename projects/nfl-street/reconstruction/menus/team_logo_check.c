#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_screen.h"
#include "team_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define ARG_AT 0x817d2000u
#define OUT_AT 0x817d3000u
#define RET_AT 0x817d3010u
#define STACK_TOP 0x817df800u
#define STACK_BASE 0x81810000u
#define SENTINEL 0x8badc0deu

typedef struct {nflstreet_team_record record;uint8_t raw_type5;} logo_row;
static nflstreet_team_catalog catalog;
static logo_row rows[32];

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t*load_file(const char*path,size_t*size,size_t max){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(size_t)z>max||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}

static int load_catalog(const char*dir){
 for(unsigned i=0;i<32;++i){char path[4096];int z=snprintf(path,sizeof path,"%s/%05u.bin",dir,i+1);if(z<0||(size_t)z>=sizeof path)return 0;
  size_t n=0;uint8_t*b=load_file(path,&n,8u*1024u*1024u);nflstreet_team_table t;
  if(!b||!nflstreet_team_table_open(b,n,&t)||t.used!=1){free(b);return 0;}
  nflstreet_team_catalog one={0};if(!nflstreet_team_catalog_append(&one,&t)||one.count!=1){free(b);return 0;}
  rows[i].record=one.records[0];rows[i].raw_type5=(uint8_t)(one.records[0].raw_type==5);
  if(!nflstreet_team_catalog_append(&catalog,&t)){free(b);return 0;}free(b);
 }
 return catalog.count==32;
}

typedef struct {const nflstreet_team_record*row;uint32_t requested_id,mode;unsigned lookup_calls,pytt_calls,ctdl_calls,logo_calls,query_opens,query_closes,actor_calls;} oracle_data;

static int run_original(const ModernGekkoModuleDesc*m,CPUState*c,oracle_data*d){
 c->exception=0;c->downcount=c->cycle_budget=10000000;
 for(unsigned step=0;c->pc&&step<200000;++step){uint32_t pc=c->pc;
  if(pc==0x8010b60c){c->gpr[3]=d->mode;c->pc=c->lr;}
  else if(pc==0x80004668){++d->actor_calls;c->gpr[3]=0;c->pc=c->lr;}
  else if(pc==0x80150808){if(!c->gpr[3]||c->gpr[4]!=0){fprintf(stderr,"query open args %08x/%08x\n",c->gpr[3],c->gpr[4]);return 0;}++d->query_opens;c->pc=c->lr;}
  else if(pc==0x8014947c){if(!c->gpr[3]||c->gpr[4]!=0x44494754||c->gpr[5]!=d->requested_id||c->gpr[6]!=0||c->gpr[7]!=0){fprintf(stderr,"DIGT lookup args %08x/%08x/%08x/%08x/%08x\n",c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6],c->gpr[7]);return 0;}++d->lookup_calls;c->gpr[3]=d->row!=NULL;c->pc=c->lr;}
  else if(pc==0x80149614){if(c->gpr[4]==0x50595454){++d->pytt_calls;c->gpr[3]=d->row?d->row->raw_type:0;}
   else if(c->gpr[4]==0x4c445443){++d->ctdl_calls;c->gpr[3]=0;}
   else {fprintf(stderr,"Unexpected 80149614 tag %08x\n",c->gpr[4]);return 0;}c->pc=c->lr;}
  else if(pc==0x801495bc){if(c->gpr[4]!=0x4c474c54){fprintf(stderr,"Unexpected 801495BC tag %08x\n",c->gpr[4]);return 0;}++d->logo_calls;c->gpr[3]=d->row?d->row->logo_entry:0;c->pc=c->lr;}
  else if(pc==0x80149064){++d->query_closes;c->pc=c->lr;}
  else {c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Original logo/mode callback exception pc=%08x ex=%08x\n",c->pc,c->exception);return 0;}
 }
 if(c->pc){fprintf(stderr,"Original callback stopped pc=%08x lr=%08x\n",c->pc,c->lr);return 0;}
 return c->gpr[3]==1;
}

static int logo_case(const ModernGekkoModuleDesc*m,uint8_t*seed,const nflstreet_team_record*r,uint32_t id){
 uint8_t*actual=malloc(RAM_SIZE),*native_ram=malloc(RAM_SIZE);if(!actual||!native_ram)return 0;memcpy(actual,seed,RAM_SIZE);memcpy(native_ram,seed,RAM_SIZE);
 memset(actual+ARG_AT-0x80000000,0xa5,0x40);memset(native_ram+ARG_AT-0x80000000,0xa5,0x40);
 put(actual+ARG_AT-0x80000000,id);put(actual+ARG_AT-0x80000000+4,OUT_AT);put(actual+ARG_AT-0x80000000+8,OUT_AT+4);
 put(native_ram+ARG_AT-0x80000000,id);put(native_ram+ARG_AT-0x80000000+4,OUT_AT);put(native_ram+ARG_AT-0x80000000+8,OUT_AT+4);
 put(actual+OUT_AT-0x80000000,0xaaaaaaaa);put(actual+OUT_AT-0x80000000+4,0xbbbbbbbb);put(actual+RET_AT-0x80000000,SENTINEL);
 put(native_ram+OUT_AT-0x80000000,0xaaaaaaaa);put(native_ram+OUT_AT-0x80000000+4,0xbbbbbbbb);put(native_ram+RET_AT-0x80000000,SENTINEL);
 CPUState c={0};c.ram=actual;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK_TOP;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=8;c.gpr[4]=ARG_AT;c.gpr[5]=0;c.gpr[6]=RET_AT;c.pc=0x8010c3bc;
 oracle_data d={0};d.row=r;d.requested_id=id;d.mode=6;
 if(!run_original(m,&c,&d)||c.gpr[1]!=STACK_TOP||be(actual+RET_AT-0x80000000)!=SENTINEL){fprintf(stderr,"Original dispatch stack/result failure id%u\n",id);return 0;}
 uint32_t stack[256];for(unsigned i=0;i<256;++i)stack[i]=0x7e7e7e7e;
 nflstreet_team_callback state={0};state.catalog=catalog;state.query_mode=6;
 nflstreet_ui_service_request q={0};q.kind=1;q.property=8;q.argc=3;q.args[0]=id;q.args[1]=OUT_AT;q.args[2]=OUT_AT+4;uint32_t result=SENTINEL;
 if(!nflstreet_team_callback_memory(&state,native_ram,RAM_SIZE,0x80000000,&q,&result,STACK_BASE,stack)||result!=SENTINEL){fprintf(stderr,"Native logo callback rejected id%u\n",id);return 0;}
 uint32_t expected_logo=r?r->logo_entry:0;
 if(be(actual+OUT_AT-0x80000000)!=expected_logo||be(actual+OUT_AT-0x80000000+4)!=UINT32_MAX||
    be(native_ram+OUT_AT-0x80000000)!=expected_logo||be(native_ram+OUT_AT-0x80000000+4)!=UINT32_MAX){fprintf(stderr,"Logo/badge outputs differ id%u actual=%08x/%08x native=%08x/%08x expected=%08x/ffffffff\n",id,be(actual+OUT_AT-0x80000000),be(actual+OUT_AT-0x80000000+4),be(native_ram+OUT_AT-0x80000000),be(native_ram+OUT_AT-0x80000000+4),expected_logo);return 0;}
 if(d.lookup_calls!=1||d.query_opens!=1||d.query_closes!=1||d.actor_calls!=(r?2u:1u)||
    (r&&(d.pytt_calls!=1||d.ctdl_calls!=1||d.logo_calls!=1))||(!r&&(d.pytt_calls||d.ctdl_calls||d.logo_calls))){fprintf(stderr,"Original helper trace mismatch id%u lookups%u open%u close%u actor%u PYTT%u CTDL%u LGLT%u\n",id,d.lookup_calls,d.query_opens,d.query_closes,d.actor_calls,d.pytt_calls,d.ctdl_calls,d.logo_calls);return 0;}
 for(unsigned i=8;i<0x40;++i)if(actual[ARG_AT-0x80000000+i]!=native_ram[ARG_AT-0x80000000+i])return 0;
 for(unsigned i=0;i<256;++i)if(stack[i]!=0x7e7e7e7e)return 0;
 free(actual);free(native_ram);return 1;
}

static int mode_case(const ModernGekkoModuleDesc*m,uint8_t*seed,unsigned mode){
 uint8_t*actual=malloc(RAM_SIZE),*native_ram=malloc(RAM_SIZE);if(!actual||!native_ram)return 0;memcpy(actual,seed,RAM_SIZE);memcpy(native_ram,seed,RAM_SIZE);
 put(actual+ARG_AT-0x80000000,0);put(actual+RET_AT-0x80000000,SENTINEL);
 put(native_ram+RET_AT-0x80000000,SENTINEL);
 CPUState c={0};c.ram=actual;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK_TOP;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=0xe0;c.gpr[4]=ARG_AT;c.gpr[5]=0;c.gpr[6]=RET_AT;c.pc=0x8010c3bc;
 oracle_data d={0};d.mode=mode;if(!run_original(m,&c,&d)||c.gpr[1]!=STACK_TOP){fprintf(stderr,"Original E0 setup/return failed mode%u pc%08x r3%08x sp%08x value%08x\n",mode,c.pc,c.gpr[3],c.gpr[1],be(actual+RET_AT-0x80000000));return 0;}
 nflstreet_team_screen screen={0};screen.supplied_common_mode=mode;
 nflstreet_ui_service_request q={0};q.kind=1;q.property=0xe0;uint32_t output=SENTINEL;
 if(!nflstreet_team_screen_service(&screen,native_ram,RAM_SIZE,0x80000000,&q,&output))return 0;
 if(output!=be(actual+RET_AT-0x80000000)||output!=((mode>=3&&mode<=5)?1u:0u)){fprintf(stderr,"E0 result mismatch mode%u actual%08x native%08x\n",mode,be(actual+RET_AT-0x80000000),output);return 0;}
 free(actual);free(native_ram);return 1;
}

static int native_type5_guard(void){
 uint8_t*ram=calloc(1,RAM_SIZE);if(!ram)return 0;
 nflstreet_team_callback state={0},before={0};state.catalog=catalog;state.catalog.records[0].raw_type=5;before=state;
 put(ram+OUT_AT-0x80000000,0xaaaaaaaa);put(ram+OUT_AT-0x80000000+4,0xbbbbbbbb);
 uint8_t saved[8];memcpy(saved,ram+OUT_AT-0x80000000,sizeof saved);
 nflstreet_ui_service_request q={0};q.kind=1;q.property=8;q.argc=3;q.args[0]=catalog.records[0].id;q.args[1]=OUT_AT;q.args[2]=OUT_AT+4;
 uint32_t words[256],before_words[256],result=SENTINEL;for(unsigned i=0;i<256;++i)words[i]=before_words[i]=0x7e7e7e7e;
 int accepted=nflstreet_team_callback_memory(&state,ram,RAM_SIZE,0x80000000,&q,&result,STACK_BASE,words);
 int ok=!accepted&&result==SENTINEL&&!memcmp(&state,&before,sizeof state)&&!memcmp(words,before_words,sizeof words)&&!memcmp(ram+OUT_AT-0x80000000,saved,sizeof saved);
 if(!ok)fprintf(stderr,"Synthetic raw-type5 rejection guard changed state or accepted the request\n");free(ram);return ok;
}

int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"Usage: team-logo-check.exe <original-DLL> <db_teams-folder>\n");return 2;}
 if(!load_catalog(argv[2])){fprintf(stderr,"Could not parse all 32 actual team records\n");return 4;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*seed=calloc(1,RAM_SIZE);if(!seed)return 4;unsigned regular=0,type5=0;
 for(unsigned i=0;i<catalog.count;++i){const nflstreet_team_record*r=&catalog.records[i];if(r->resource_fields!=3)return 5;
  if(r->raw_type==5){++type5;continue;}if(!logo_case(m,seed,r,r->id))return 6;++regular;
 }
 if(!logo_case(m,seed,NULL,0xffffffffu))return 7;
 for(unsigned mode=0;mode<=8;++mode)if(!mode_case(m,seed,mode)){fprintf(stderr,"E0 query mode%u failed\n",mode);return 8;}
 if(!native_type5_guard())return 9;
 printf("Team logo callback original comparison passes: %u regular actual records and missing-ID case; %u actual type5 records were absent, synthetic native type5 rejection guard passes (no original type5 claim). Original 8010C3BC/8010BE60 and 8012A944 executed, supplied common mode6/actor-false and query open, DIGT lookup, PYTT/CTDL/LGLT field responses. Logo/badge words, result sentinel and SP retained; callback E0 modes0..8 match 8010B694.\n",regular,type5);
 free(seed);FreeLibrary(h);return 0;
}
