#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_carousel.h"
#include "team_selection.h"
#include "team_callback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define SIDE_TABLE 0x817e3000u
#define WINDOW0 0x817e1000u
#define WINDOW1 0x817e1020u
#define OUT_VALUES 0x817e2000u
#define OUT_FLAGS 0x817e2020u
#define CURSOR0 0x802f507cu
#define CURSOR1 0x802f50a8u

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}

typedef struct {
 const nflstreet_team_carousel_catalog *catalogs;
 uint32_t cursor[2];
 uint32_t saved_cursor[2];
 unsigned seeks,reads,nexts,prevs,predicates,false_queries;
} original_services;

static int side_for_cursor(uint32_t address){
 if(address==CURSOR0)return 0;
 if(address==CURSOR1)return 1;
 return -1;
}

static int find_id(const nflstreet_team_carousel_catalog*c,uint32_t id,uint32_t*index){
 for(uint32_t i=0;i<c->count;++i)if(c->entries[i].id==id){*index=i;return 1;}
 return 0;
}

static int dispatch_original(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,
                             original_services*s,uint32_t operation,uint32_t side,unsigned wrapper){
 (void)ram;
 c->pc=0x8011ce64;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=10000000;
 c->gpr[3]=side;c->gpr[4]=operation;c->gpr[5]=OUT_VALUES;c->gpr[6]=OUT_FLAGS;
 if(wrapper){
  put(ram+OUT_VALUES-0x80000000-(1u+(side+operation)%3u)*4u,(side+operation)%3u);put(ram+OUT_FLAGS-0x80000000-(1u+(side+operation)%3u)*4u,(side+operation)%3u);
  uint8_t*a=ram+0x17e2100;put(a,side);put(a+4,operation);put(a+8,OUT_VALUES-(1u+(side+operation)%3u)*4u);put(a+12,OUT_FLAGS-(1u+(side+operation)%3u)*4u);
  put(ram+0x17e2180,0xabcddcbau);
  c->pc=0x8011e9d4;c->gpr[3]=0x80000004u;c->gpr[4]=0x817e2100u;c->gpr[5]=0;c->gpr[6]=0x817e2180u;
 }
 for(unsigned step=0;c->pc&&step<10000;++step){
  uint32_t pc=c->pc;
  if(pc==0x80009e5c||pc==0x80009e6c){++s->false_queries;c->gpr[3]=0;c->pc=c->lr;}
  else if(pc==0x801509c8){
   int which=side_for_cursor(c->gpr[3]);uint32_t index;
   if(which<0||c->gpr[5]!=0||!find_id(&s->catalogs[which],c->gpr[4],&index))return 0;
   s->cursor[which]=index;++s->seeks;c->pc=c->lr;
  }else if(pc==0x80150978){
   int which=side_for_cursor(c->gpr[3]);
   if(which<0||s->cursor[which]>=s->catalogs[which].count)return 0;
   c->gpr[3]=s->catalogs[which].entries[s->cursor[which]].id;++s->reads;c->pc=c->lr;
  }else if(pc==0x8011c364||pc==0x8011c328){
   int which=side_for_cursor(c->gpr[3]);
   if(which<0||s->cursor[which]>=s->catalogs[which].count)return 0;
   uint32_t count=s->catalogs[which].count;
   if(pc==0x8011c364){s->cursor[which]=(s->cursor[which]+1u)%count;++s->nexts;}
   else{s->cursor[which]=(s->cursor[which]+count-1u)%count;++s->prevs;}
   c->pc=c->lr;
  }else if(pc==0x80127900){
   /* Supply the no-profile local branch for both profile queries. */
   c->gpr[3]=0xffffffffu;c->pc=c->lr;
  }else if(pc==0x801490c8){
   int which=side_for_cursor(c->gpr[3]);
   if(which<0||s->cursor[which]>=s->catalogs[which].count)return 0;
   s->saved_cursor[which]=s->cursor[which];c->gpr[3]=s->saved_cursor[which];c->pc=c->lr;
  }else if(pc==0x801495bc){
   int which=side_for_cursor(c->gpr[3]);
   if(which<0||c->gpr[4]!=0x53495654u||s->cursor[which]>=s->catalogs[which].count)return 0;
   c->gpr[3]=s->catalogs[which].entries[s->cursor[which]].d060_result==0?1u:0u;
   ++s->predicates;c->pc=c->lr;
  }else if(pc==0x8014928c){
   int which=side_for_cursor(c->gpr[3]);
   if(which<0||s->saved_cursor[which]>=s->catalogs[which].count)return 0;
   s->cursor[which]=s->saved_cursor[which];c->pc=c->lr;
  }else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception)return 0;
 }
 return c->pc==0&&!c->exception&&(!wrapper||(c->gpr[3]==1&&be(ram+0x17e2180)==0xabcddcbau));
}

static int compare_case(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,
                        const nflstreet_team_carousel*seed,uint32_t side,uint32_t operation,
                        unsigned*comparisons,unsigned*guards,unsigned wrapper){
 nflstreet_team_carousel native=*seed;
 nflstreet_team_carousel_output expected={{0,0,0},{0,0,0}};
 int native_ok=nflstreet_team_carousel_apply(&native,side,operation,&expected);
 if(wrapper){
  nflstreet_team_callback bridge={0};bridge.startup.carousel=*seed;
  uint8_t owned[64];memset(owned,0xa5,sizeof owned);uint32_t dims=(side+operation)%3u;put(owned+16-(dims+1)*4,dims);put(owned+48-(dims+1)*4,dims);
  uint32_t stack[256]={0},result=0xabcddcbau;
  nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000004u;q.argc=4;
  q.args[0]=side;q.args[1]=operation;q.args[2]=OUT_VALUES-(1u+(side+operation)%3u)*4u;q.args[3]=OUT_FLAGS-(1u+(side+operation)%3u)*4u;
  int accepted=nflstreet_team_callback_memory(&bridge,owned,sizeof owned,OUT_VALUES-16,&q,&result,0x817c0000,stack);
  if(accepted!=native_ok||result!=0xabcddcbau||memcmp(&bridge.startup.carousel,&native,sizeof native))return 0;
  if(accepted)for(unsigned i=0;i<3;++i)if(be(owned+16+i*4)!=expected.values[i]||be(owned+48+i*4)!=expected.flags[i])return 0;
 }
 memset(ram+0x17e0000,0x5a,0x4000);
 put(ram+0x3a4008,SIDE_TABLE);
 put(ram+SIDE_TABLE-0x80000000,WINDOW0);
 put(ram+SIDE_TABLE-0x80000000+4,WINDOW1);
 for(unsigned s=0;s<2;++s){uint32_t address=s?WINDOW1:WINDOW0;
  for(unsigned i=0;i<3;++i)put(ram+address-0x80000000+i*4,seed->windows[s][i]);
 }
 for(unsigned i=0;i<3;++i){put(ram+OUT_VALUES-0x80000000+i*4,0xa1000000u+i);put(ram+OUT_FLAGS-0x80000000+i*4,0xb2000000u+i);}
 if(wrapper){
  put(ram+OUT_VALUES-0x80000000-(1u+(side+operation)%3u)*4u,(side+operation)%3u);put(ram+OUT_FLAGS-0x80000000-(1u+(side+operation)%3u)*4u,(side+operation)%3u);
  uint8_t*a=ram+0x17e2100;put(a,side);put(a+4,operation);put(a+8,OUT_VALUES-(1u+(side+operation)%3u)*4u);put(a+12,OUT_FLAGS-(1u+(side+operation)%3u)*4u);put(ram+0x17e2180,0xabcddcbau);
 }
 uint8_t before_window[24],before_output[24];
 memcpy(before_window,ram+WINDOW0-0x80000000,12);memcpy(before_window+12,ram+WINDOW1-0x80000000,12);
 memcpy(before_output,ram+OUT_VALUES-0x80000000,12);memcpy(before_output+12,ram+OUT_FLAGS-0x80000000,12);
 uint8_t*before_guard=malloc(0x4000);if(!before_guard)return 0;memcpy(before_guard,ram+0x17e0000,0x4000);
 original_services services={0};services.catalogs=seed->catalogs;services.cursor[0]=seed->catalogs[0].cursor;services.cursor[1]=seed->catalogs[1].cursor;
 if(!dispatch_original(m,c,ram,&services,operation,side,wrapper)){free(before_guard);return 0;}
 int oracle_ok=1;
 if(!native_ok){
  if(memcmp(&native,seed,sizeof native)||memcmp(before_window,ram+WINDOW0-0x80000000,12)||
     memcmp(before_window+12,ram+WINDOW1-0x80000000,12)||
     memcmp(before_output,ram+OUT_VALUES-0x80000000,12)||
     memcmp(before_output+12,ram+OUT_FLAGS-0x80000000,12))oracle_ok=0;
 }else{
  for(unsigned s=0;s<2;++s)for(unsigned i=0;i<3;++i){uint32_t address=s?WINDOW1:WINDOW0;
   if(native.windows[s][i]!=be(ram+address-0x80000000+i*4))oracle_ok=0;
  }
  for(unsigned i=0;i<3;++i){
   if(expected.values[i]!=be(ram+OUT_VALUES-0x80000000+i*4)||expected.flags[i]!=be(ram+OUT_FLAGS-0x80000000+i*4))oracle_ok=0;
  }
  for(unsigned s=0;s<2;++s)if(native.catalogs[s].cursor!=services.cursor[s])oracle_ok=0;
  for(unsigned i=0;i<0x4000;++i){
   int writable=(i>=0x1000&&i<0x100c)||(i>=0x1020&&i<0x102c)||
                (i>=0x2000&&i<0x200c)||(i>=0x2020&&i<0x202c);
   if(!writable&&before_guard[i]!=ram[0x17e0000+i])oracle_ok=0;
  }
 }
 free(before_guard);
 if(!oracle_ok){fprintf(stderr,"Differential mismatch side%u op%u native_accept%d seek%u read%u next%u prev%u d060%u false%u\n",side,operation,native_ok,services.seeks,services.reads,services.nexts,services.prevs,services.predicates,services.false_queries);for(unsigned s=0;s<2;++s){fprintf(stderr," side%u native cursor%u oracle cursor%u windows",s,native.catalogs[s].cursor,services.cursor[s]);for(unsigned i=0;i<3;++i)fprintf(stderr," %08x/%08x",native.windows[s][i],be(ram+(s?WINDOW1:WINDOW0)-0x80000000+i*4));fputc('\n',stderr);}for(unsigned i=0;i<3;++i)fprintf(stderr," out%u %08x/%08x flag%08x/%08x\n",i,expected.values[i],be(ram+OUT_VALUES-0x80000000+i*4),expected.flags[i],be(ram+OUT_FLAGS-0x80000000+i*4));return 0;}
 ++*comparisons;if(!native_ok)++*guards;return 1;
}

static int make_case(nflstreet_team_carousel*s,unsigned n,unsigned test){
 memset(s,0,sizeof *s);
 for(unsigned side=0;side<2;++side){
  unsigned count=side?65u-n:n;if(!count||count>NFLSTREET_TEAM_CAROUSEL_MAX_IDS)return 0;
  nflstreet_team_carousel_catalog*c=&s->catalogs[side];c->count=count;c->cursor=(test+side*3u)%count;
  for(unsigned i=0;i<count;++i){c->entries[i].id=0x61000000u+side*0x1000u+i*0x10u;c->entries[i].d060_result=(i+side+test)&1u;}
 }
 return 1;
}

static int load_bytes(const char*path,uint8_t**bytes,size_t*size){
 FILE*f=fopen(path,"rb");if(!f)return 0;
 if(fseek(f,0,SEEK_END)){fclose(f);return 0;}long length=ftell(f);
 if(length<=0||length>0x1000000L||fseek(f,0,SEEK_SET)){fclose(f);return 0;}
 uint8_t*b=malloc((size_t)length);if(!b){fclose(f);return 0;}
 int ok=fread(b,1,(size_t)length,f)==(size_t)length&&!ferror(f);fclose(f);
 if(!ok){free(b);return 0;}*bytes=b;*size=(size_t)length;return 1;
}

static int load_private_catalog(const char*directory,nflstreet_team_catalog*out){
 memset(out,0,sizeof *out);
 for(unsigned i=1;i<=32;++i){char path[1024];
  int written=snprintf(path,sizeof path,"%s\\%05u.bin",directory,i);
  if(written<0||(size_t)written>=sizeof path)return 0;
  uint8_t*bytes=NULL;size_t size=0;nflstreet_team_table table;
  if(!load_bytes(path,&bytes,&size))return 0;
  int ok=nflstreet_team_table_open(bytes,size,&table)&&
         nflstreet_team_catalog_append(out,&table);
  free(bytes);if(!ok)return 0;
 }
 return out->count==32;
}

int main(int argc,char**argv){
 if(argc!=3&&argc!=4)return 2;
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(RAM_SIZE);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,RAM_SIZE,f)!=RAM_SIZE)return 4;fclose(f);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
 unsigned comparisons=0,guards=0;
 for(unsigned count0=1;count0<=NFLSTREET_TEAM_CAROUSEL_MAX_IDS;++count0){
  for(unsigned side=0;side<2;++side)for(unsigned opcase=0;opcase<3;++opcase){
   uint32_t operation=opcase==0?1u:opcase==1?3u:2u;
   nflstreet_team_carousel seed;if(!make_case(&seed,count0,count0*7u+side))return 5;
   uint32_t count=seed.catalogs[side].count,start;
   if(operation==1)start=count>2?count-3u:0u;
   else if(operation==3)start=0;
   else start=(count0+side)%count;
   for(unsigned i=0;i<3;++i)seed.windows[side][i]=seed.catalogs[side].entries[(start+i)%count].id;
   for(unsigned other=0;other<2;++other){uint32_t n=seed.catalogs[other].count,s=(count0+other)%n;
    if(other!=side)for(unsigned i=0;i<3;++i)seed.windows[other][i]=seed.catalogs[other].entries[(s+i)%n].id;
   }
   for(unsigned wrapper=0;wrapper<2;++wrapper)if(!compare_case(m,&c,ram,&seed,side,operation,&comparisons,&guards,wrapper)){free(ram);FreeLibrary(h);return 6;}
  }
 }

 unsigned database_comparisons=0;
 if(argc==4){
  nflstreet_team_catalog catalog;
  if(!load_private_catalog(argv[3],&catalog)){fprintf(stderr,"Could not load exactly32 validated private MAET blocks.\n");free(ram);FreeLibrary(h);return 8;}
  uint32_t centers[2]={catalog.records[0].id,catalog.records[1].id};
  nflstreet_team_carousel seed;
  if(!nflstreet_team_selection_seed(&catalog,centers,&seed)){fprintf(stderr,"Could not seed the supplied two-center catalog.\n");free(ram);FreeLibrary(h);return 8;}
  for(unsigned side=0;side<2;++side)for(unsigned opcase=0;opcase<3;++opcase){
   uint32_t operation=opcase==0?1u:opcase==1?3u:2u;
   nflstreet_team_carousel actual=seed;uint32_t count=actual.catalogs[side].count;
   uint32_t start=operation==1?count-3u:operation==3?0u:count/2u;
   for(unsigned i=0;i<3;++i)actual.windows[side][i]=actual.catalogs[side].entries[(start+i)%count].id;
   for(unsigned wrapper=0;wrapper<2;++wrapper)if(!compare_case(m,&c,ram,&actual,side,operation,&comparisons,&guards,wrapper)){fprintf(stderr,"Private catalog differential failed side%u op%u\n",side,operation);free(ram);FreeLibrary(h);return 9;}
   ++database_comparisons;
  }
 }

 /* Invalid owned states reject without touching state or output. */
 nflstreet_team_carousel seed;if(!make_case(&seed,4,7))return 5;
 for(unsigned i=0;i<3;++i)seed.windows[0][i]=seed.catalogs[0].entries[i].id;
 for(unsigned i=0;i<3;++i)seed.windows[1][i]=seed.catalogs[1].entries[i].id;
 uint32_t sentinel=0xa5a5a5a5u; nflstreet_team_carousel_output out;
 memset(&out,(int)(sentinel&0xff),sizeof out);
 nflstreet_team_carousel prior=seed;out=(nflstreet_team_carousel_output){{sentinel,sentinel,sentinel},{sentinel,sentinel,sentinel}};
 if(nflstreet_team_carousel_apply(&seed,2,1,&out)||memcmp(&seed,&prior,sizeof seed)||out.values[0]!=sentinel||out.flags[2]!=sentinel)return 7;
 ++guards;
 seed.catalogs[0].count=0;prior=seed;out=(nflstreet_team_carousel_output){{sentinel,sentinel,sentinel},{sentinel,sentinel,sentinel}};
 if(nflstreet_team_carousel_apply(&seed,0,3,&out)||memcmp(&seed,&prior,sizeof seed)||out.values[0]!=sentinel||out.flags[2]!=sentinel)return 7;
 ++guards;
 seed=prior;seed.catalogs[0].count=4;seed.windows[0][0]=0xdeadbeefu;prior=seed;out=(nflstreet_team_carousel_output){{sentinel,sentinel,sentinel},{sentinel,sentinel,sentinel}};
 if(nflstreet_team_carousel_apply(&seed,0,2,&out)||memcmp(&seed,&prior,sizeof seed)||out.values[0]!=sentinel||out.flags[2]!=sentinel)return 7;
 ++guards;
 printf("Original 8011CE64 controlled carousel exact: %u differential comparisons of direct8011CE64 and callback8011E9D4/80000004 across both sides, operations1/3/other, catalog lengths1..64, and wrap boundaries; %u invalid native state guards; %u private MAET catalog checks. Supplied catalog IDs, no-profile responses, raw SIVT query results feeding original 8011D060, and false80009E5C/6C responses; no full team-screen startup or field meanings claimed.\n",comparisons,guards,database_comparisons);
 free(ram);FreeLibrary(h);return 0;
}
