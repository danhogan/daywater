#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_bootstrap_weights.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define RAM_BASE 0x80000000u
#define R13 0x803a9a40u
#define CONTEXT 0x817d0100u
#define OUTPUT 0x817d0200u
#define ATTRIBUTES 0x817d0300u
#define STACK 0x817df000u

static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t*load_file(const char*path,size_t*size,size_t max){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(size_t)z>max||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}

static int run_original(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t fn,uint32_t arg3,uint32_t arg4,uint32_t arg5,uint32_t*ret3){
 CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=arg3;c.gpr[4]=arg4;c.gpr[5]=arg5;c.pc=fn;c.downcount=c.cycle_budget=100000000;
 for(unsigned step=0;c.pc&&step<400000;++step){uint32_t pc=c.pc;c.downcount=-1000000000;m->dispatch(&c,pc);
  if(c.exception){fprintf(stderr,"Actual8014F2B0 exception pc%08x ex%08x step%u\n",c.pc,c.exception,step);return 0;}}
 if(c.pc||c.gpr[1]!=STACK){fprintf(stderr,"Actual8014F2B0 stopped pc%08x sp%08x\n",c.pc,c.gpr[1]);return 0;}
 if(ret3)*ret3=c.gpr[3];return 1;
}

static int run_actual_f2b0(const ModernGekkoModuleDesc*m,uint8_t*ram,const uint32_t ids[12],int16_t out[10]){
 memset(ram+CONTEXT-RAM_BASE,0,0x50);for(unsigned i=0;i<12;++i)put32(ram+CONTEXT-RAM_BASE+12+i*4,ids[i]);memset(ram+OUTPUT-RAM_BASE,0,32);
 if(!run_original(m,ram,0x8014f2b0u,CONTEXT,OUTPUT,0,NULL))return 0;
 for(unsigned i=0;i<10;++i){uint16_t bits=(uint16_t)((uint16_t)ram[OUTPUT-RAM_BASE+i*2]<<8|ram[OUTPUT-RAM_BASE+i*2+1]);memcpy(&out[i],&bits,2);}return 1;
}

static int check_curve_template(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const uint8_t*template_db,size_t template_size,unsigned*cases){
 nflstreet_team_rating_curve native,initialized;
 if(!nflstreet_team_bootstrap_curve_from_template(template_db,template_size,ram,RAM_SIZE,RAM_BASE,&native)||
    !nflstreet_team_rating_curve_from_ram(ram,RAM_SIZE,RAM_BASE,&initialized))return 0;
 for(unsigned role=0;role<14;++role){
  if(memcmp(&native.role_anchor[role],&initialized.role_anchor[role],4)||
     memcmp(&native.role_multiplier[role],&initialized.role_multiplier[role],4)||
     memcmp(&native.role_limit[role],&initialized.role_limit[role],4)||
     memcmp(native.coefficients[role],initialized.coefficients[role],sizeof native.coefficients[role])){
   fprintf(stderr,"ROBP initialized curve mismatch role%u anchor %.9g/%.9g multiplier %.9g/%.9g limit %.9g/%.9g\n",role,native.role_anchor[role],initialized.role_anchor[role],native.role_multiplier[role],initialized.role_multiplier[role],native.role_limit[role],initialized.role_limit[role]);
   for(unsigned col=0;col<10;++col)fprintf(stderr," coefficient%u %.9g/%.9g\n",col,native.coefficients[role][col],initialized.coefficients[role][col]);return 0;}}
 static const uint32_t values[4][10]={{1,1,1,1,1,1,1,1,1,1},{99,99,99,99,99,99,99,99,99,99},{32,46,57,61,13,27,84,93,5,72},{0,100,0,100,0,100,0,100,0,100}};
 for(unsigned role=0;role<14;++role)for(unsigned pattern=0;pattern<4;++pattern){uint32_t expected,actual;
  if(!nflstreet_team_rating_score(&native,role,values[pattern],0,&expected))return 0;
  for(unsigned i=0;i<10;++i)put32(ram+ATTRIBUTES-RAM_BASE+i*4,values[pattern][i]);
  if(!run_original(m,ram,0x801e0d00u,role,ATTRIBUTES,0,&actual)||expected!=actual){fprintf(stderr,"ROBP curve score mismatch role%u pattern%u native%08x original%08x\n",role,pattern,expected,actual);return 0;}++*cases;
 }
 return 1;
}

static int number_tag(const nflstreet_team_table*t,unsigned row,uint32_t tag,uint32_t*out){
 unsigned found=0,index=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(f.type!=3||++found!=1)return 0;index=i;}}
 return found==1&&nflstreet_team_table_number(t,row,index,out);
}

static int original_categories(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t categories[12]){
 for(unsigned i=0;i<12;++i)if(!run_original(m,ram,0x8014bc04u,i,0,0,&categories[i]))return 0;return 1;
}

static int check_original_bootstrap_row(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const uint8_t*template_db,size_t template_size,const char*team_db_path){
 size_t db_size=0;uint8_t*db=load_file(team_db_path,&db_size,8u*1024u*1024u);if(!db)return 0;
 nflstreet_team_table play;if(!nflstreet_team_table_open_tag(db,db_size,0x59414c50u,&play)){free(db);return 0;}
 uint32_t team,player;if(!number_tag(&play,0,0x44494754u,&team)||!number_tag(&play,0,0x44494750u,&player)){free(db);return 0;}
 nflstreet_team_weight_inputs inputs;if(!nflstreet_team_weights_from_db(db,db_size,team,player,&inputs)){free(db);return 0;}
 uint32_t categories[12];if(!original_categories(m,ram,categories)){free(db);return 0;}
 nflstreet_team_gear_response responses[12];if(!nflstreet_team_bootstrap_responses_from_template(template_db,template_size,&inputs,categories,0x17u,responses)){free(db);return 0;}
 nflstreet_team_rating_curve curve;if(!nflstreet_team_bootstrap_curve_from_template(template_db,template_size,ram,RAM_SIZE,RAM_BASE,&curve)){free(db);return 0;}
 uint32_t mode=0;nflstreet_team_bootstrap_weight_row native;if(!nflstreet_team_bootstrap_weights_build(db,db_size,template_db,template_size,&curve,mode,team,player,categories,responses,0x17u,&native)){fprintf(stderr,"Native weighted row build failed team%u player%u mode%u\n",team,player,mode);free(db);return 0;}
 memset(ram+CONTEXT-RAM_BASE,0,0x20);put32(ram+CONTEXT-RAM_BASE,player);put32(ram+CONTEXT-RAM_BASE+4,0x7fffu);put32(ram+OUTPUT-RAM_BASE,0);
 if(!run_original(m,ram,0x80042568u,mode,CONTEXT,OUTPUT,NULL)){fprintf(stderr,"Original 80042568 call failed mode%u player%u\n",mode,player);free(db);return 0;}
 uint32_t rows=be32(ram+OUTPUT-RAM_BASE);if(rows<RAM_BASE||(uint64_t)rows-RAM_BASE+0x44>RAM_SIZE){fprintf(stderr,"Original 80042568 output pointer invalid %08x\n",rows);free(db);return 0;}
 const uint8_t*p=ram+rows-RAM_BASE;for(unsigned i=0;i<14;++i){uint32_t original=be32(p+4+i*4);if(original!=native.weighted[i]){fprintf(stderr,"80042568 weighted mismatch team%u player%u role%u native%u original%u\n",team,player,i,native.weighted[i],original);free(db);return 0;}}
 uint32_t original_role=be32(p+0x40);if(original_role!=native.role){fprintf(stderr,"80042568 PBPO mismatch team%u player%u native%u original%u\n",team,player,native.role,original_role);free(db);return 0;}
 printf("80042568 native weighted row exact mode%u team%u player%u role%u\n",mode,team,player,native.role);free(db);return 1;
}


/* Whole original 80042568/8014FCCC arithmetic, with resolved raw database
   getters and allocation supplied. No original registration/bootstrap claim. */
static int controlled_weighted(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const uint8_t*template_db,size_t template_size,const char*folder){
 nflstreet_team_rating_curve curve;uint32_t categories[12];nflstreet_team_table tstr;
 if(!nflstreet_team_bootstrap_curve_from_template(template_db,template_size,ram,RAM_SIZE,RAM_BASE,&curve)||!original_categories(m,ram,categories)||!nflstreet_team_table_open_tag(template_db,template_size,0x52545354u,&tstr))return 0;
 unsigned cases=0;
 for(unsigned mode=0;mode<5;++mode){uint32_t modifiers[14]={0};unsigned matches=0;
  for(unsigned row=0;row<tstr.used;++row){uint32_t value;if(!number_tag(&tstr,row,0x59545453u,&value))return 0;if(value!=mode)continue;++matches;
   for(unsigned i=0;i<14;++i)if(!number_tag(&tstr,row,be32(ram+0x80285d2cu-RAM_BASE+i*4),&modifiers[i]))return 0;
  }if(matches!=1)return 0;
  for(unsigned entry=1;entry<=32;++entry){char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",folder,entry);size_t z;uint8_t*db=load_file(path,&z,8u*1024u*1024u);nflstreet_team_table play;
   if(!db||!nflstreet_team_table_open_tag(db,z,0x59414c50u,&play)){free(db);return 0;}
   for(unsigned row=0;row<play.used;++row){uint32_t team,id;if(!number_tag(&play,row,0x44494754u,&team)||!number_tag(&play,row,0x44494750u,&id)){free(db);return 0;}if(!team)continue;
    nflstreet_team_weight_inputs in;nflstreet_team_gear_response responses[12];nflstreet_team_bootstrap_weight_row expected;
    if(!nflstreet_team_weights_from_db(db,z,team,id,&in)||!nflstreet_team_bootstrap_responses_from_template(template_db,template_size,&in,categories,0x17,responses)||!nflstreet_team_bootstrap_weights_build(db,z,template_db,template_size,&curve,mode,team,id,categories,responses,0x17,&expected)){free(db);return 0;}
    put32(ram+CONTEXT-RAM_BASE,id);put32(ram+CONTEXT-RAM_BASE+4,0x7fff);put32(ram+OUTPUT-RAM_BASE,0);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=R13;c.gpr[3]=mode;c.gpr[4]=CONTEXT;c.gpr[5]=OUTPUT;c.pc=0x80042568u;
    unsigned ratings_calls=0,modifier_calls=0,role_calls=0,allocated=0;
    for(unsigned step=0;c.pc&&step<10000;++step){uint32_t pc=c.pc;int supplied=1;
     switch(pc){
      case 0x80150ed4u:case 0x80150f34u:case 0x8014e67cu:case 0x801abf1cu:case 0x8014e740u:case 0x80150f14u:break;
      case 0x80004668u:c.gpr[3]=0;break;
      case 0x8014e800u:if(c.gpr[4]!=id||c.gpr[5]){free(db);return 0;}c.gpr[3]=1;break;
      case 0x8018b7b8u:if(c.gpr[3]!=0x44){free(db);return 0;}c.gpr[3]=0x817d0400u;++allocated;break;
      case 0x8014ececu:c.gpr[3]=expected.role;++role_calls;break;
      case 0x8014f5c8u:if(c.gpr[4]<RAM_BASE||(uint64_t)c.gpr[4]-RAM_BASE+40>RAM_SIZE){free(db);return 0;}for(unsigned i=0;i<10;++i)put32(ram+c.gpr[4]-RAM_BASE+i*4,expected.adjusted_ratings[i]);++ratings_calls;break;
      case 0x80150f6cu:if(c.gpr[4]>=14){free(db);return 0;}c.gpr[3]=modifiers[c.gpr[4]];++modifier_calls;break;
      default:supplied=0;break;
     }
     if(supplied)c.pc=c.lr;else{c.downcount=-1000000000;m->dispatch(&c,pc);if(c.exception){free(db);return 0;}}
    }
    if(c.pc||c.gpr[1]!=STACK||allocated!=1||role_calls!=1||ratings_calls!=1||modifier_calls!=14){fprintf(stderr,"Controlled builder call framing failed pc%x allocation%u role%u rating%u mods%u\n",c.pc,allocated,role_calls,ratings_calls,modifier_calls);free(db);return 0;}
    const uint8_t*out=ram+0x817d0400u-RAM_BASE;
    if(be32(ram+OUTPUT-RAM_BASE)!=0x817d0400u||be32(out)!=id||out[0x3c]||be32(out+0x40)!=expected.role){free(db);return 0;}
    for(unsigned i=0;i<14;++i)if(be32(out+4+i*4)!=expected.weighted[i]){fprintf(stderr,"Controlled weighted mismatch team%u id%u mode%u role%u native%u original%u\n",team,id,mode,i,expected.weighted[i],be32(out+4+i*4));free(db);return 0;}
    ++cases;
   }free(db);
  }
 }
 printf("Controlled 80042568/8014FCCC weighted rows exact: %u authentic PLAY/mode cases; resolved DB getters/allocation supplied, original scoring and unsigned weighting executed\n",cases);return cases==2010;
}

static int raw_one_query(const uint8_t*template_db,size_t template_size,
 const nflstreet_team_weight_inputs*in,const uint32_t categories[12],unsigned slot,
 int16_t values[10],int*found){
 if(slot>=12||slot==7)return 0;unsigned query_slot=slot;if(slot==6&&in->query_ids[6]==0)query_slot=7;
 nflstreet_team_gear_lookup lookup;if(!nflstreet_team_gear_lookup_from_template(template_db,template_size,in->query_ids[query_slot],categories[slot],&lookup))return 0;
 memset(values,0,10*sizeof *values);*found=lookup.found;if(lookup.found)memcpy(values,lookup.values,sizeof lookup.values);return 1;
}

static void raw_join_sum(const uint8_t*template_db,size_t template_size,
 const nflstreet_team_weight_inputs*in,const uint32_t categories[12],int16_t sums[10],unsigned*joined,unsigned*missed){
 memset(sums,0,10*sizeof *sums);*joined=*missed=0;
 for(unsigned slot=0;slot<12;++slot){if(slot==7)continue;
  int16_t values[10];int found;if(!raw_one_query(template_db,template_size,in,categories,slot,values,&found))continue;
  if(!found){++*missed;continue;}++*joined;
  for(unsigned i=0;i<10;++i){uint16_t bits;memcpy(&bits,&sums[i],2);bits=(uint16_t)(bits+(uint16_t)values[i]);memcpy(&sums[i],&bits,2);}
 }
}

static int check_authentic_query_aggregates(const ModernGekkoModuleDesc*m,uint8_t*ram,
 const char*db_folder,const uint8_t*template_db,size_t template_size,unsigned*players,unsigned*queries){
 uint32_t categories[12];if(!original_categories(m,ram,categories))return 0;unsigned count=0,query_count=0,total_joined=0,total_missed=0;
 for(unsigned entry=1;entry<=32;++entry){char path[4096];int n=snprintf(path,sizeof path,"%s/%05u.bin",db_folder,entry);if(n<0||(size_t)n>=sizeof path)return 0;
  size_t size=0;uint8_t*db=load_file(path,&size,8u*1024u*1024u);if(!db)return 0;nflstreet_team_table play;
  if(!nflstreet_team_table_open_tag(db,size,0x59414c50u,&play)){free(db);return 0;}
  for(unsigned row=0;row<play.used;++row){uint32_t team,player;if(!number_tag(&play,row,0x44494754u,&team)||!number_tag(&play,row,0x44494750u,&player)){free(db);return 0;}
   nflstreet_team_weight_inputs in;if(!nflstreet_team_weights_from_db(db,size,team,player,&in)){free(db);return 0;}
   int16_t actual[10],expected[10];unsigned joined=0,missed=0;
   if(!run_actual_f2b0(m,ram,in.query_ids,actual)){fprintf(stderr,"Actual 8014F2B0 failed team%u player%u entry%u row%u\n",team,player,entry,row);free(db);return 0;}
   raw_join_sum(template_db,template_size,&in,categories,expected,&joined,&missed);
   if(memcmp(actual,expected,sizeof actual)){fprintf(stderr,"Actual query aggregate differs from bounded raw join team%u player%u entry%u row%u joined%u missing%u actual",team,player,entry,row,joined,missed);for(unsigned i=0;i<10;++i)fprintf(stderr," %d",actual[i]);fprintf(stderr," expected");for(unsigned i=0;i<10;++i)fprintf(stderr," %d",expected[i]);fputc('\n',stderr);free(db);return 0;}
   for(unsigned slot=0;slot<12;++slot){if(slot==7)continue;uint32_t ids[12]={0};unsigned response_slot=slot;if(slot==6&&in.query_ids[6]==0)response_slot=7;ids[response_slot]=in.query_ids[response_slot];
    int16_t exact[10],want[10];int found;if(!raw_one_query(template_db,template_size,&in,categories,slot,want,&found)||!run_actual_f2b0(m,ram,ids,exact)||memcmp(exact,want,sizeof want)){
     fprintf(stderr,"Isolated original query differs from raw join team%u player%u entry%u row%u slot%u response%u joined%d\n",team,player,entry,row,slot,response_slot,found);free(db);return 0;}
   }
   total_joined+=joined;total_missed+=missed;++count;query_count+=11;
  }
  free(db);
 }
 *players=count;*queries=query_count;printf("untouched 8014F2B0 matched raw signed GEAR/GRIP aggregate for %u PLAY rows / %u isolated query slots; joined requests %u, non-joined requests %u\n",count,query_count,total_joined,total_missed);return count>0;
}

int main(int argc,char**argv){
 if(argc!=5&&argc!=6){fprintf(stderr,"Usage: team-bootstrap-weights-original-check.exe <original-DLL> <snapshot.mem1> <db_teams-folder> <template-00002.bin>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t ram_size=0,template_size=0;uint8_t*ram=load_file(argv[2],&ram_size,RAM_SIZE),*template_db=load_file(argv[4],&template_size,8u*1024u*1024u);if(!ram||ram_size!=RAM_SIZE||!template_db)return 4;
 unsigned curve_cases=0;if(!check_curve_template(m,ram,template_db,template_size,&curve_cases)){fprintf(stderr,"ROBP template curve validation failed\n");return 5;}
 unsigned players=0,queries=0;if(!check_authentic_query_aggregates(m,ram,argv[3],template_db,template_size,&players,&queries))return 6;
 if(!controlled_weighted(m,ram,template_db,template_size,argv[3]))return 7;
 if(argc==6){char sample_path[4096];int pn=snprintf(sample_path,sizeof sample_path,"%s/%05u.bin",argv[3],16u);if(pn<0||(size_t)pn>=sizeof sample_path||!check_original_bootstrap_row(m,ram,template_db,template_size,sample_path))return 8;}
 printf("ROBP native template curve matched initialized curve and original 801E0D00 for %u score cases\n",curve_cases);
 FreeLibrary(h);free(ram);free(template_db);return 0;
}
