#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_start.h"
#include "team_callback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE (24u*1024u*1024u)
#define DATA 0x803a9a40u
#define TABLE 0x817e3000u
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint8_t*read_file(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<0||z>(long)RAM_SIZE){fclose(f);return NULL;}rewind(f);uint8_t*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}
static int load_catalog(const char*directory,nflstreet_team_catalog*out){
 memset(out,0,sizeof *out);for(unsigned i=1;i<=32;++i){char path[4096];int z=snprintf(path,sizeof path,"%s/%05u.bin",directory,i);if(z<0||z>=(int)sizeof path)return 0;
  size_t n=0;uint8_t*b=read_file(path,&n);nflstreet_team_table t;int ok=b&&nflstreet_team_table_open(b,n,&t)&&nflstreet_team_catalog_append(out,&t);free(b);if(!ok)return 0;
 }return out->count==32;
}
static int side(uint32_t cursor){return cursor==0x802f507cu?0:cursor==0x802f50a8u?1:-1;}
typedef struct {
 const nflstreet_team_catalog *catalog;
 uint32_t cursor[2],saved[2],query_mode;
 uint32_t configs[2][3];
 unsigned opens,allocations,refreshes,acquires,resets,callback_calls,predicates;
 nflstreet_team_start_effect effects[NFLSTREET_TEAM_START_EFFECT_CAPACITY];
 unsigned effect_count;
} oracle;
static int effect(oracle*x,uint32_t kind,uint32_t side_id,uint32_t a,uint32_t b,uint32_t c){
 if(x->effect_count>=NFLSTREET_TEAM_START_EFFECT_CAPACITY)return 0;
 x->effects[x->effect_count++]=(nflstreet_team_start_effect){kind,side_id,a,b,c};return 1;
}
static int dispatch(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,oracle*x,unsigned wrapper){
 c->pc=0x8011c414;c->lr=0;c->exception=0;c->cycle_budget=10000000;
 if(wrapper){c->pc=0x8011e9d4;c->gpr[3]=0x80000001u;c->gpr[4]=0x817c0400u;c->gpr[5]=0;c->gpr[6]=0x817c0800u;put(ram+0x17c0800,0xabcddcbau);}
 for(unsigned step=0;c->pc&&step<200000;++step){uint32_t pc=c->pc;
  if(pc==0x80127900){if(c->gpr[3]>1)return 0;c->gpr[3]=UINT32_MAX;c->pc=c->lr;}
  else if(pc==0x8010c390){c->gpr[3]=x->query_mode;c->pc=c->lr;}
  else if(pc==0x8010b60c){c->gpr[3]=6;c->pc=c->lr;}
  else if(pc==0x80149af0){if(c->gpr[3]>1)return 0;c->gpr[3]=1;c->pc=c->lr;}
  else if(pc==0x8014aafc){if(!effect(x,NFLSTREET_TEAM_START_REFRESH,UINT32_MAX,pc,0,0))return 0;++x->refreshes;c->pc=c->lr;}
  else if(pc==0x8011c3a0){int s=side(c->gpr[3]);if(s<0||c->gpr[4]!=(uint32_t)s||!effect(x,NFLSTREET_TEAM_START_CALLBACK,(uint32_t)s,pc,(uint32_t)s,0))return 0;++x->callback_calls;c->downcount=-1000000000;m->dispatch(c,pc);}
  else if(pc==0x80150808){int s=side(c->gpr[3]);uint32_t at=c->gpr[4]-0x80000000;if(s<0||c->gpr[4]<0x80000000||at>RAM_SIZE-12)return 0;
   for(unsigned i=0;i<3;++i)x->configs[s][i]=be(ram+at+i*4);
   for(unsigned i=0;i<4;++i)if(be(ram+c->gpr[3]-0x80000000+i*4)!=(i<2?0u:UINT32_MAX))return 0;
   if(be(ram+c->gpr[3]-0x80000000+40)!=UINT32_MAX)return 0;
   if(!effect(x,NFLSTREET_TEAM_START_QUERY_OPEN,(uint32_t)s,x->configs[s][0],x->configs[s][1],x->configs[s][2]))return 0;
   x->cursor[s]=UINT32_MAX;++x->opens;c->pc=c->lr;
  }else if(pc==0x80150a00){if(side(c->gpr[3])<0||c->gpr[4]>1||c->gpr[5])return 0;c->gpr[3]=0;c->pc=c->lr;}
  else if(pc==0x801490d0){int s=side(c->gpr[3]);if(s<0)return 0;x->cursor[s]=0;c->gpr[3]=1;c->pc=c->lr;}
  else if(pc==0x8014919c){int s=side(c->gpr[3]);if(s<0||x->cursor[s]>=x->catalog->count)return 0;
   ++x->cursor[s];c->gpr[3]=x->cursor[s]<x->catalog->count;if(!c->gpr[3])x->cursor[s]=UINT32_MAX;c->pc=c->lr;
  }else if(pc==0x801509c8){int s=side(c->gpr[3]);if(s<0||c->gpr[5])return 0;uint32_t i=0;while(i<x->catalog->count&&x->catalog->records[i].id!=c->gpr[4])++i;
   c->gpr[3]=i<x->catalog->count;if(c->gpr[3])x->cursor[s]=i;c->pc=c->lr;
  }else if(pc==0x80150978){int s=side(c->gpr[3]);if(s<0||x->cursor[s]>=x->catalog->count)return 0;c->gpr[3]=x->catalog->records[x->cursor[s]].id;c->pc=c->lr;}
  else if(pc==0x8011c328||pc==0x8011c364){int s=side(c->gpr[3]);if(s<0||x->cursor[s]>=x->catalog->count)return 0;
   uint32_t count=x->catalog->count;x->cursor[s]=(x->cursor[s]+(pc==0x8011c328?count-1:1))%count;c->pc=c->lr;
  }else if(pc==0x801490c8){int s=side(c->gpr[3]);if(s<0)return 0;x->saved[s]=x->cursor[s];c->gpr[3]=x->cursor[s];c->pc=c->lr;}
  else if(pc==0x8014928c){int s=side(c->gpr[3]);if(s<0||c->gpr[4]!=x->saved[s])return 0;x->cursor[s]=c->gpr[4];c->pc=c->lr;}
  else if(pc==0x801495bc){int s=side(c->gpr[3]);if(s<0||c->gpr[4]!=0x53495654||x->cursor[s]>=x->catalog->count)return 0;
   c->gpr[3]=x->catalog->records[x->cursor[s]].sivt;++x->predicates;c->pc=c->lr;
  }else if(pc==0x801e3c00){if(c->gpr[3])return 0;c->gpr[3]=0;c->pc=c->lr;}
  else if(pc==0x8018b7b8){unsigned size=x->allocations?12:8;if(x->allocations>=3||c->gpr[3]!=size||c->gpr[4])return 0;
   if(!effect(x,NFLSTREET_TEAM_START_ALLOC,UINT32_MAX,size,0,0))return 0;
   c->gpr[3]=x->allocations==0?TABLE:x->allocations==1?0x817e1000:0x817e1020;++x->allocations;c->pc=c->lr;
  }else if(pc==0x801d5c24){if(c->gpr[3]!=1||!effect(x,NFLSTREET_TEAM_START_ARCHIVE_OPEN,UINT32_MAX,1,0,0))return 0;c->gpr[3]=0x817f0000;c->pc=c->lr;}
  else if(pc==0x801d54a4){if(c->gpr[3]!=0x817f0000||c->gpr[4]!=13||c->gpr[5]!=0x3e||!effect(x,NFLSTREET_TEAM_START_ARCHIVE_READ,UINT32_MAX,13,0x3e,0))return 0;++x->acquires;c->pc=c->lr;}
  else if(pc==0x8003f824){if(!effect(x,NFLSTREET_TEAM_START_RESET,UINT32_MAX,pc,0,0))return 0;++x->resets;c->pc=c->lr;}
  else if(pc==0x80004668){c->gpr[3]=0;c->pc=c->lr;}
  else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Startup exception pc%08x\n",c->pc);return 0;}
 }
 if(c->pc){fprintf(stderr,"Startup stopped pc%08x lr%08x\n",c->pc,c->lr);return 0;}return wrapper?(c->gpr[3]==1&&be(ram+0x17c0800)==0):c->gpr[3]==0;
}
static const uint32_t flag_offsets[8]={0x6e44,0x6e43,0x6e3f,0x6e34,0x6e33,0x6e30,0x6e2f,0x6e40};
static void seed(uint8_t*ram,const nflstreet_team_start_state*s){
 for(unsigned i=0;i<2;++i){put(ram+DATA-0x80000000-0x6e3c+i*4,s->prior_profile[i]);put(ram+DATA-0x80000000-0x6e4c+i*4,s->selected_id[i]);ram[DATA-0x80000000-0x6e50+i]=(uint8_t)s->eligibility_count[i];}
 for(unsigned i=0;i<8;++i)ram[DATA-0x80000000-flag_offsets[i]]=s->opaque_flags[i];
 const uint32_t cursors[4]={0x802f507c,0x802f50a8,0x802f52cc,0x802f52f8};
 for(unsigned i=0;i<4;++i)memset(ram+cursors[i]-0x80000000,0xa5,44);
 memset(ram+0x17e0000,0x5a,0x4000);
 put(ram+DATA-0x80000000-0x6e18,s->zero_words[0]);put(ram+DATA-0x80000000-0x6e14,s->zero_words[1]);put(ram+DATA-0x80000000-0x6e1c,s->zero_words[2]);
}
static int compare(uint8_t*ram,const nflstreet_team_start_state*s,const oracle*x){
 for(unsigned i=0;i<2;++i){if(s->prior_profile[i]!=be(ram+DATA-0x80000000-0x6e3c+i*4)||s->selected_id[i]!=be(ram+DATA-0x80000000-0x6e4c+i*4)||s->eligibility_count[i]!=ram[DATA-0x80000000-0x6e50+i]||s->profile_changed[i]!=ram[DATA-0x80000000-0x6e20+i]||s->carousel.catalogs[i].cursor!=x->cursor[i]){fprintf(stderr,"Startup side%u profile/default/cursor mismatch nativecursor%u original%u\n",i,s->carousel.catalogs[i].cursor,x->cursor[i]);return 0;}
  uint32_t at=i?0x17e1020:0x17e1000;for(unsigned j=0;j<3;++j)if(s->carousel.windows[i][j]!=be(ram+at+j*4)){fprintf(stderr,"Startup side%u window%u differs\n",i,j);return 0;}
 }
 if(memcmp(s->query_configs,x->configs,sizeof x->configs)||s->refresh_calls!=x->refreshes||s->allocation_calls!=x->allocations||s->query_open_calls!=x->opens||s->archive_reads!=x->acquires||s->reset_calls!=x->resets||s->callback_calls!=x->callback_calls||s->effect_count!=x->effect_count||memcmp(s->effects,x->effects,sizeof x->effects)){fprintf(stderr,"Startup ordered service effects differ native%u original%u\n",s->effect_count,x->effect_count);return 0;}
 for(unsigned i=0;i<8;++i)if(s->opaque_flags[i]!=ram[DATA-0x80000000-flag_offsets[i]]){fprintf(stderr,"Startup flag%u differs %u/%u\n",i,s->opaque_flags[i],ram[DATA-0x80000000-flag_offsets[i]]);return 0;}
 if(be(ram+DATA-0x80000000-0x6e18)||be(ram+DATA-0x80000000-0x6e14)||be(ram+DATA-0x80000000-0x6e1c)||s->zero_words[0]||s->zero_words[1]||s->zero_words[2])return 0;
 const uint32_t cursors[4]={0x802f507c,0x802f50a8,0x802f52cc,0x802f52f8};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<11;++j){uint32_t expected=j<2?0u:j<4||j==10?UINT32_MAX:0xa5a5a5a5u;if(be(ram+cursors[i]-0x80000000+j*4)!=expected)return 0;}
 if(be(ram+DATA-0x80000000-0x5a38)!=TABLE||be(ram+TABLE-0x80000000)!=0x817e1000||be(ram+TABLE-0x80000000+4)!=0x817e1020)return 0;
 for(unsigned i=0;i<0x4000;++i){int written=(i>=0x1000&&i<0x100c)||(i>=0x1020&&i<0x102c)||(i>=0x3000&&i<0x3008);if(!written&&ram[0x17e0000+i]!=0x5a)return 0;}
 return 1;
}
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t z=0;uint8_t*ram=read_file(argv[2],&z);nflstreet_team_catalog actual;if(!ram||z!=RAM_SIZE||!load_catalog(argv[3],&actual))return 4;
 unsigned cases=0;
 for(unsigned catalog_case=0;catalog_case<3;++catalog_case){nflstreet_team_catalog cat=actual;
  if(catalog_case==1)for(unsigned i=0;i<cat.count;++i)cat.records[i].sivt=(i%3)==0;
  if(catalog_case==2){cat.count=1;cat.records[0].sivt=0;}
  for(unsigned mode=0;mode<2;++mode)for(unsigned test=0;test<8;++test)for(unsigned wrapper=0;wrapper<2;++wrapper){nflstreet_team_start_state state={0};
   state.prior_profile[0]=test&1?123:UINT32_MAX;state.prior_profile[1]=test&2?456:UINT32_MAX;
   const uint32_t selected[4]={UINT32_MAX,cat.records[0].id,cat.records[cat.count-1].id,0xf00d0000};
   state.selected_id[0]=selected[test%4];state.selected_id[1]=selected[(test+1)%4];state.eligibility_count[0]=13;state.eligibility_count[1]=27;memset(state.opaque_flags,0x73,sizeof state.opaque_flags);for(unsigned i=0;i<3;++i)state.zero_words[i]=0xa5a5a5a5;
   seed(ram,&state);
   if(wrapper){nflstreet_team_callback bridge={0};bridge.catalog=cat;bridge.startup=state;bridge.query_mode=mode;
    uint8_t owned[4]={0};uint32_t stack[256]={0},result=0xabcddcbau;nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000001u;
    if(!nflstreet_team_callback_memory(&bridge,owned,sizeof owned,0x81100000,&q,&result,0x817c0000,stack)||result)return 5;
    state=bridge.startup;
   }else if(!nflstreet_team_start_apply(&cat,mode,&state))return 5;
   oracle x={0};x.catalog=&cat;x.query_mode=mode;CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=DATA;
   if(!dispatch(m,&c,ram,&x,wrapper)||!compare(ram,&state,&x)){fprintf(stderr,"Startup comparison failed catalogcase%u mode%u test%u pc%08x\n",catalog_case,mode,test,c.pc);return 6;}
   if(!wrapper)++cases;
  }
 }
 printf("Team startup original8011C414 exact:%u direct comparisons plus48 original8011E9D4/80000001 callback comparisons; actual32-team records, mixed eligibility and one-item catalogs; query modes0/1, stale profiles, valid/missing stored IDs, full windows/cursors, flags, cursor initialization and ordered resource requests. DB/profile/random/actor boundaries supplied; full source-screen activation not claimed.\n",cases);
 free(ram);FreeLibrary(h);return 0;
}
