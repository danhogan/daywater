#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_weights.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define R13 0x803a9a40u
#define OBJECT 0x817d0000u
#define CONTEXT 0x817d0100u
#define OUTPUT 0x817d0200u
#define ATTRIBUTES 0x817d0300u
#define STACK 0x817df000u
#define TABLE_STAT 0x54415453u
#define STATUS_NO_ROW 0x17u

static const uint32_t base_tags[10]={0x49474150u,0x52574150u,0x4b544250u,0x48544350u,0x44505350u,0x4b415450u,0x53535050u,0x4b4c4250u,0x564f4350u,0x54464450u};

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void put16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static uint8_t*load_file(const char*path,size_t*size,size_t max){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(size_t)z>max||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}
static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}

typedef struct {const nflstreet_team_weight_inputs*inputs;const nflstreet_team_gear_response*responses;const uint32_t*categories;unsigned slot,calls,field_queries;} query_boundary;

static int base_for_tag(uint32_t tag,const nflstreet_team_weight_inputs*in,uint32_t*out){for(unsigned i=0;i<10;++i)if(base_tags[i]==tag){*out=in->base_values[i];return 1;}return 0;}
static int ram_word(uint8_t*ram,uint32_t address,uint32_t*out){if(address<BASE||(uint64_t)address-BASE+4u>RAM_SIZE)return 0;*out=be(ram+address-BASE);return 1;}
static nflstreet_team_gear_response fixture_response(uint32_t player,unsigned slot){
 nflstreet_team_gear_response r={0};r.status=((player+slot)%5u==0)?STATUS_NO_ROW:0;
 for(unsigned i=0;i<10;++i){int v=((int)(player%7u)-3)*(int)(slot+1u)+((int)i*3-11);r.values[i]=(int16_t)v;}
 return r;
}

static int execute(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,
 uint32_t function,uint32_t arg3,uint32_t arg4,uint32_t arg5,query_boundary*boundary){
 memset(c,0,sizeof *c);c->ram=ram;c->ram_size=RAM_SIZE;c->msr=0x2000;c->gpr[1]=STACK;c->gpr[2]=0x803b9a40u;c->gpr[13]=R13;c->gpr[3]=arg3;c->gpr[4]=arg4;c->gpr[5]=arg5;c->pc=function;c->lr=0;c->downcount=c->cycle_budget=100000000;
 if(boundary){boundary->slot=0;boundary->calls=0;boundary->field_queries=0;}
 for(unsigned step=0;c->pc&&step<150000;++step){uint32_t pc=c->pc;
  if(pc==0x801b1ac8){if(!boundary||!boundary->inputs)return 0;uint32_t list=c->gpr[6];
   for(unsigned i=0;i<10;++i){uint32_t tag=be(ram+list-BASE+i*16+8),value;if(!base_for_tag(tag,boundary->inputs,&value)){fprintf(stderr,"Unmapped801B1AC8 field tag=%08x\n",tag);return 0;}put(ram+list-BASE+i*16,value);}
   ++boundary->field_queries;c->gpr[3]=0;c->pc=c->lr;
  }else if(pc==0x801b1b30){if(!boundary||!boundary->responses||!boundary->categories||c->gpr[3]!=TABLE_STAT)return 0;unsigned slot=boundary->slot;
   unsigned response_slot=slot;if(slot==6&&boundary->inputs->query_ids[6]==0)response_slot=7;else if(slot==7){response_slot=8;slot=7;}if(response_slot>=12)return 0;
   uint32_t request=c->gpr[4],conditions=0,geid_container=0,grti_node=0,geid_node=0,table_tag=0,geid_tag=0,geid_value=0,grti_tag=0,grti_category=0;
   unsigned category_slot=(slot==6&&boundary->inputs->query_ids[6]==0)?6u:response_slot;
   if(!ram_word(ram,request,&table_tag)||table_tag!=0x52414547u||!ram_word(ram,request+8,&conditions)||
      !ram_word(ram,conditions+8,&geid_container)||!ram_word(ram,conditions+24,&grti_node)){
    fprintf(stderr,"801B1B30 metadata structure invalid category%u request%08x conditions%08x\n",category_slot,request,conditions);return 0;}
   geid_node=geid_container+12;grti_node+=8;
   if(!ram_word(ram,geid_node,&geid_tag)||geid_tag!=0x44494547u||!ram_word(ram,geid_node+12,&geid_value)||
      !ram_word(ram,grti_node+4,&grti_tag)||grti_tag!=0x49545247u||!ram_word(ram,grti_node+16,&grti_category)){
    fprintf(stderr,"801B1B30 nested metadata invalid GEIDnode%08x GRTInode%08x\n",geid_node,grti_node);return 0;}
   if(geid_value!=boundary->inputs->query_ids[response_slot]||grti_category!=boundary->categories[category_slot]){
    fprintf(stderr,"801B1B30 request mismatch response%u expectedGEID%08x got%08x category%u expectedGRTI%08x got%08x\n",response_slot,boundary->inputs->query_ids[response_slot],geid_value,category_slot,boundary->categories[category_slot],grti_category);return 0;}
   uint32_t result=c->gpr[6];
   for(unsigned i=0;i<10;++i)put16(ram+result-BASE+2+i*16,(uint16_t)boundary->responses[response_slot].values[i]);
   c->gpr[3]=boundary->responses[response_slot].status;boundary->slot=(slot==6&&boundary->inputs->query_ids[6]==0)?8u:(slot==7?9u:slot+1u);++boundary->calls;c->pc=c->lr;
  }else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Original rating exception fn=%08x pc=%08x ex=%08x\n",function,c->pc,c->exception);return 0;}
 }
 if(c->pc||c->gpr[1]!=STACK){fprintf(stderr,"Original rating stopped fn=%08x pc=%08x sp=%08x\n",function,c->pc,c->gpr[1]);return 0;}return 1;
}

static int run_original_base(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const nflstreet_team_weight_inputs*inputs,uint32_t out[10]){
 memset(ram+OBJECT-BASE,0,0x20);put(ram+OBJECT-BASE,inputs->player_id);put(ram+OBJECT-BASE+8,inputs->team_id);memset(ram+OUTPUT-BASE,0,40);
 query_boundary boundary={.inputs=inputs};CPUState c;
 if(!execute(m,&c,ram,0x8014f180,OBJECT,OUTPUT,0,&boundary)||boundary.field_queries!=1)return 0;
 for(unsigned i=0;i<10;++i)out[i]=be(ram+OUTPUT-BASE+i*4);return 1;
}

static int run_original_build(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const nflstreet_team_weight_inputs*inputs,const nflstreet_team_gear_response responses[12],const uint32_t categories[14],uint32_t out[10],unsigned*queries){
 memset(ram+OBJECT-BASE,0,0x20);put(ram+OBJECT-BASE,inputs->player_id);put(ram+OBJECT-BASE+8,inputs->team_id);
 memset(ram+CONTEXT-BASE,0,80);for(unsigned i=0;i<12;++i)put(ram+CONTEXT-BASE+12+i*4,inputs->query_ids[i]);memset(ram+OUTPUT-BASE,0,40);
 query_boundary boundary={.inputs=inputs,.responses=responses,.categories=categories};CPUState c;
 if(!execute(m,&c,ram,0x8014f504,OBJECT,CONTEXT,OUTPUT,&boundary)||boundary.field_queries!=1)return 0;
 for(unsigned i=0;i<10;++i)out[i]=be(ram+OUTPUT-BASE+i*4);if(queries)*queries=boundary.calls;return 1;
}

static int run_original_score(const ModernGekkoModuleDesc*m,uint8_t*ram,
 uint32_t role,const uint32_t values[10],uint32_t clamp_mode,uint32_t*out){
 for(unsigned i=0;i<10;++i)put(ram+ATTRIBUTES-BASE+i*4,values[i]);CPUState c;
 if(!execute(m,&c,ram,0x801e0d00,role,ATTRIBUTES,clamp_mode,NULL))return 0;*out=c.gpr[3];return 1;
}

static int table_number_tag(const nflstreet_team_table*t,unsigned row,uint32_t tag,uint32_t*out);

static int run_original_signed9(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t raw,
 uint32_t*high,uint32_t*low){
 if(raw>0x1ffu||!high||!low)return 0;CPUState c;
 /* 801b94c8 left-aligns a field in a 64-bit window before 80213bd8 extracts
    its signed width. A 9-bit value at bit 55 has high word raw<<23. */
 if(!execute(m,&c,ram,0x80213bd8u,raw<<23,0,55,NULL))return 0;*high=c.gpr[3];*low=c.gpr[4];return 1;
}

static int check_original_signed_decoder(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const uint8_t*template_db,size_t template_size,unsigned*actual_fields){
 static const uint32_t edge_raw[]={0,255,256,511};unsigned count=0;
 for(unsigned i=0;i<sizeof edge_raw/sizeof edge_raw[0];++i){uint32_t high=0,low=0,want_high=(edge_raw[i]&0x100u)?0xffffffffu:0u;
  uint32_t want_low=(edge_raw[i]&0x100u)?(edge_raw[i]|0xfffffe00u):edge_raw[i];
  if(!run_original_signed9(m,ram,edge_raw[i],&high,&low)||high!=want_high||low!=want_low){fprintf(stderr,"80213bd8 edge mismatch raw%u high%08x low%08x expected%08x/%08x\n",edge_raw[i],high,low,want_high,want_low);return 0;}}
 nflstreet_team_table gear;if(!nflstreet_team_table_open_tag(template_db,template_size,0x52414547u,&gear))return 0;
 uint8_t seen[64]={0};unsigned unique_grips=0;
 for(unsigned row=0;row<gear.used;++row){uint32_t id,category;
  if(!table_number_tag(&gear,row,0x44494547u,&id)||!table_number_tag(&gear,row,0x49545247u,&category))return 0;
  nflstreet_team_gear_lookup lookup;if(!nflstreet_team_gear_lookup_from_template(template_db,template_size,id,category,&lookup))return 0;
  if(!lookup.found)continue;if(lookup.grip_row>=sizeof seen)return 0;if(seen[lookup.grip_row])continue;seen[lookup.grip_row]=1;++unique_grips;
  for(unsigned col=0;col<10;++col){uint32_t raw=(uint16_t)lookup.values[col]&0x1ffu,high,low;
   uint32_t want_high=(raw&0x100u)?0xffffffffu:0u,want_low=(raw&0x100u)?(raw|0xfffffe00u):raw;
   if(!run_original_signed9(m,ram,raw,&high,&low)||high!=want_high||low!=want_low){fprintf(stderr,"80213bd8 signed 9-bit mismatch GEARrow%u GRIProw%u field%u raw%u high%08x low%08x expected%08x/%08x\n",row,lookup.grip_row,col,raw,high,low,want_high,want_low);return 0;}++count;
  }
 }
 if(unique_grips!=30||count!=300){fprintf(stderr,"80213bd8 coverage mismatch grips%u fields%u\n",unique_grips,count);return 0;}*actual_fields=count;return 1;
}

static int run_original_category(const ModernGekkoModuleDesc*m,uint8_t*ram,unsigned slot,uint32_t*out){
 CPUState c;if(!execute(m,&c,ram,0x8014bc04,slot,0,0,NULL))return 0;*out=c.gpr[3];return 1;
}

static int table_number_tag(const nflstreet_team_table*t,unsigned row,uint32_t tag,uint32_t*out){
 unsigned found=0,field=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;if(f.tag==tag){if(f.type!=3||++found!=1)return 0;field=i;}}
 return found==1&&nflstreet_team_table_number(t,row,field,out);
}

static int check_raw_gear_table(const uint8_t*template_db,size_t template_size,unsigned*gear_rows,unsigned*joined_rows){
 nflstreet_team_table gear;if(!nflstreet_team_table_open_tag(template_db,template_size,0x52414547u,&gear))return 0;
 unsigned rows=0,joined=0;
 for(unsigned row=0;row<gear.used;++row){uint32_t id,category,key;
  if(!table_number_tag(&gear,row,0x44494547u,&id)||!table_number_tag(&gear,row,0x49545247u,&category)||!table_number_tag(&gear,row,0x594b5047u,&key))return 0;
  nflstreet_team_gear_lookup lookup;if(!nflstreet_team_gear_lookup_from_template(template_db,template_size,id,category,&lookup))return 0;
  if(lookup.found){if(lookup.gear_row!=row||lookup.gear_key!=key)return 0;++joined;}++rows;
 }
 *gear_rows=rows;*joined_rows=joined;return 1;
}

static int check_rating_case(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const nflstreet_team_rating_curve*curve,unsigned*cases){
 static const uint32_t pattern[8][10]={
  {1,1,1,1,1,1,1,1,1,1},{99,99,99,99,99,99,99,99,99,99},
  {1,99,1,99,1,99,1,99,1,99},{99,1,99,1,99,1,99,1,99,1},
  {0,100,0,100,0,100,0,100,0,100},{100,0,100,0,100,0,100,0,100,0},
  {32,46,57,61,13,27,84,93,5,72},{75,62,51,44,91,11,38,6,88,23}
 };
 for(unsigned role=0;role<14;++role)for(unsigned mode=0;mode<2;++mode)for(unsigned k=0;k<8;++k){uint32_t expected=0,actual=0;
  if(!nflstreet_team_rating_score(curve,role,pattern[k],mode,&expected)||!run_original_score(m,ram,role,pattern[k],mode,&actual))return 0;
  if(actual!=expected){fprintf(stderr,"801E0D00 mismatch role%u mode%u pattern%u original=%08x native=%08x\n",role,mode,k,actual,expected);return 0;}++*cases;
 }
 return 1;
}

static int check_team_file(const ModernGekkoModuleDesc*m,uint8_t*ram,const char*folder,const uint8_t*template_db,size_t template_size,const uint32_t categories[14],
 unsigned entry,unsigned*players,unsigned*builds){
 char path[4096];int len=snprintf(path,sizeof path,"%s/%05u.bin",folder,entry);if(len<0||(size_t)len>=sizeof path)return 0;size_t size=0;uint8_t*db=load_file(path,&size,8u*1024u*1024u);if(!db)return 0;
 nflstreet_team_table play;unsigned id_field=0,team_field=0;if(!nflstreet_team_table_open_tag(db,size,0x59414c50u,&play)){free(db);return 0;}
 if(!table_number_tag(&play,0,0x44494750u,&id_field)||!table_number_tag(&play,0,0x44494754u,&team_field)){free(db);return 0;}
 uint32_t first_team=0,first_player=0;int got_first=0;
 for(unsigned row=0;row<play.used;++row){uint32_t team=0,player=0;if(!table_number_tag(&play,row,0x44494750u,&player)||!table_number_tag(&play,row,0x44494754u,&team)){free(db);return 0;}
  nflstreet_team_weight_inputs in;if(!nflstreet_team_weights_from_db(db,size,team,player,&in)){fprintf(stderr,"PLAY field extraction failed team%u player%u entry%u row%u\n",team,player,entry,row);free(db);return 0;}
  if(in.team_id!=team||in.player_id!=player){free(db);return 0;}
  uint32_t original[10];if(!run_original_base(m,ram,&in,original)){fprintf(stderr,"8014F180 raw-field comparison failed team%u player%u\n",team,player);free(db);return 0;}
  if(memcmp(original,in.base_values,sizeof original)){fprintf(stderr,"8014F180 values differ team%u player%u\n",team,player);free(db);return 0;}
  if(!got_first){first_team=team;first_player=player;got_first=1;}++*players;
 }
 if(!got_first){free(db);return 0;}
 nflstreet_team_weight_inputs in;if(!nflstreet_team_weights_from_db(db,size,first_team,first_player,&in)){free(db);return 0;}
 for(unsigned variant=0;variant<3;++variant){nflstreet_team_weight_inputs sample=in;if(variant==1)sample.query_ids[6]=0;
  nflstreet_team_gear_response responses[12];for(unsigned slot=0;slot<12;++slot)responses[slot]=fixture_response(sample.player_id,slot);
  unsigned raw_hits=0;if(variant==2){for(unsigned slot=0;slot<12;++slot){if(slot==7)continue;unsigned response_slot=slot,category_slot=slot;if(slot==6&&sample.query_ids[6]==0)response_slot=7;
    nflstreet_team_gear_lookup lookup;if(!nflstreet_team_gear_lookup_from_template(template_db,template_size,sample.query_ids[response_slot],categories[category_slot],&lookup)){free(db);return 0;}
    if(lookup.found){responses[response_slot].status=0;memcpy(responses[response_slot].values,lookup.values,sizeof lookup.values);++raw_hits;}}
  }
  uint32_t expected[10],actual[10];unsigned original_queries=0;
  if(!nflstreet_team_weights_build(&sample,responses,STATUS_NO_ROW,expected)||!run_original_build(m,ram,&sample,responses,categories,actual,&original_queries)||memcmp(expected,actual,sizeof expected)){
   fprintf(stderr,"8014F504 base+gear arithmetic mismatch team%u player%u variant%u query6=%u calls%u expected",first_team,first_player,variant,sample.query_ids[6],original_queries);for(unsigned j=0;j<10;++j)fprintf(stderr," %u",expected[j]);fprintf(stderr," original");for(unsigned j=0;j<10;++j)fprintf(stderr," %u",actual[j]);fputc('\n',stderr);free(db);return 0;}
  unsigned expected_queries=11u;if(original_queries!=expected_queries){fprintf(stderr,"8014F2B0 query count differs team%u player%u got%u expected%u\n",first_team,first_player,original_queries,expected_queries);free(db);return 0;}++*builds;
  if(variant==2)printf("entry%u team%u player%u: %u of 11 joined GEAR/GRIP vectors supplied to the controlled original 8014F504 response hook; missing rows and every status remain explicit harness inputs\n",entry,first_team,first_player,raw_hits);
 }
 free(db);return 1;
}

int main(int argc,char**argv){
 if(argc!=5){fprintf(stderr,"Usage: team-weights-original-check.exe <original-DLL> <position-neutral-folder> <db_teams-folder> <template-00002.bin>\n");return 2;}
 size_t template_size=0;uint8_t*template_db=load_file(argv[4],&template_size,8u*1024u*1024u);if(!template_db)return 2;
 unsigned raw_gear_rows=0,raw_joined_rows=0;if(!check_raw_gear_table(template_db,template_size,&raw_gear_rows,&raw_joined_rows)){free(template_db);fprintf(stderr,"Raw GEAR/GRIP table join validation failed\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 unsigned snapshots[]={14,15,16};for(unsigned s=0;s<3;++s){char path[4096];int n=snprintf(path,sizeof path,"%s/position-neutral-%02u.mem1",argv[2],snapshots[s]);if(n<0||(size_t)n>=sizeof path)return 4;size_t ram_size=0;uint8_t*baseline=load_file(path,&ram_size,RAM_SIZE);if(!baseline||ram_size!=RAM_SIZE)return 4;uint64_t hash=hash_bytes(baseline,ram_size);uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 4;memcpy(ram,baseline,RAM_SIZE);
  nflstreet_team_rating_curve curve;if(!nflstreet_team_rating_curve_from_ram(ram,RAM_SIZE,BASE,&curve)){fprintf(stderr,"Rating curve extraction failed snapshot%u\n",snapshots[s]);return 5;}
  unsigned score_cases=0,players=0,builds=0,signed_fields=0;uint32_t categories[14];
  if(!check_rating_case(m,ram,&curve,&score_cases)){fprintf(stderr,"801E0D00 score compare failed snapshot%u\n",snapshots[s]);return 6;}
  if(!check_original_signed_decoder(m,ram,template_db,template_size,&signed_fields)){fprintf(stderr,"Original 80213bd8 signed field decoder check failed snapshot%u\n",snapshots[s]);return 6;}
  for(unsigned role=0;role<14;++role)if(!run_original_category(m,ram,role,&categories[role])){fprintf(stderr,"8014BC04 category lookup failed role%u snapshot%u\n",role,snapshots[s]);return 6;}
  for(unsigned entry=1;entry<=32;++entry)if(!check_team_file(m,ram,argv[3],template_db,template_size,categories,entry,&players,&builds)){fprintf(stderr,"Team rating inputs failed entry%u snapshot%u\n",entry,snapshots[s]);return 7;}
  if(hash_bytes(baseline,ram_size)!=hash){free(ram);free(baseline);return 8;}
  printf("snapshot%u: 801E0D00 exact %u role/value/clamp cases; 8014F180 exact raw tag values across %u PLAY rows; 8014F504 exact supplied-response builds %u including slot6 zero; 80213bd8 signed decoder exact on %u joined GRIP fields plus 4 edge values; 801B1B30 GEID/GRTI request metadata exact. Baseline FNV64 %016llx unchanged.\n",snapshots[s],score_cases,players,builds,signed_fields,(unsigned long long)hash);
  free(ram);free(baseline);
 }
 FreeLibrary(h);free(template_db);puts("Raw PLAY extraction, GEAR/GRIP table join, supplied-response 8014F504 arithmetic, and 801E0D00 score conversion pass. The comparator injects joined payloads and supplied statuses; runtime database registration and STAT query result behavior are not claimed.");return 0;
}
