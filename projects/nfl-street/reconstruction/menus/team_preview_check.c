#ifdef _WIN32
#include <windows.h>
#include "moderngekko/module_abi.h"
#endif
#include "team_preview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE_R13 0x803a9a40u
#define COUNT_ADDRESS (BASE_R13-0x5a34u)
#define IDS_ADDRESS 0x802f50d4u
#define SIDE_STRIDE 0xf4u
#define RECORD_STRIDE NFLSTREET_TEAM_PREVIEW_QUERY_RECORD_STRIDE
#define CANARY 0xa5a5a5a5u

typedef struct {
 const uint32_t *ids;
 uint32_t count,index;
 uint32_t clear_slots[7],clear_count;
} supplied_play_query;

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}

static uint8_t*load_file(const char*path,size_t*size,size_t max){
 FILE*f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);
 if(z<=0||(size_t)z>max||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}
 if(fread(b,1,(size_t)z,f)!=(size_t)z||ferror(f)){free(b);fclose(f);return NULL;}
 fclose(f);*size=(size_t)z;return b;
}

static int load_team(const char*folder,unsigned entry,uint8_t**bytes,size_t*size){
 char path[4096];int z=snprintf(path,sizeof path,"%s/%05u.bin",folder,entry);
 if(z<0||(size_t)z>=sizeof path)return 0;
 *bytes=load_file(path,size,65536);return *bytes!=NULL;
}

static int native_probe(const char*folder){
 unsigned blocks=0,players=0;
 for(unsigned entry=1;entry<=32;++entry){uint8_t*b=NULL;size_t n=0;
  if(!load_team(folder,entry,&b,&n))return 3;uint32_t team=0,count=0,ids[20];
  if(!nflstreet_team_preview_team_id_from_db(b,n,&team)||
     !nflstreet_team_preview_roster_from_db(b,n,team,ids,&count))return 4;
  nflstreet_team_preview_roster state={0};
  for(unsigned side=0;side<2;++side){
   if(!nflstreet_team_preview_roster_select(&state,side,team,ids,count)||
      state.team_id!=team||state.count!=count||memcmp(state.player_ids,ids,count*4)||
      state.cursor!=count-1||state.cursor_mode!=1||state.clear_count!=7)return 5;
   for(unsigned i=0;i<7;++i)if(state.cleared_slots[i]!=side*7+i)return 5;
  }
  printf("Team entry%02u: TEAM/DIGT=%u; PLAY/DIGT rows=%u; ordered PLAY/DIGP IDs:",entry,team,count);
  for(unsigned i=0;i<count;++i)printf(" %u",ids[i]);putchar('\n');
  ++blocks;players+=count;free(b);
 }
 printf("Independent native team preview roster parser passes: %u private team blocks, %u PLAY/DIGP roster IDs; selected cursor=count-1, seven clear-slot requests per side. No renderer or visible actor claim.\n",blocks,players);
 return blocks==32?0:6;
}

#ifdef _WIN32
static uint8_t*ram_pointer(uint8_t*ram,uint32_t address){
 if(address<0x80000000u)return NULL;uint32_t offset=address-0x80000000u;
 return offset<RAM_SIZE?ram+offset:NULL;
}

static int run_original(const ModernGekkoModuleDesc*m,uint8_t*ram,
                        uint32_t side,const uint32_t*ids,uint32_t count,
                        nflstreet_team_preview_roster*expected){
 uint8_t*count_ptr=ram_pointer(ram,COUNT_ADDRESS+side*4u);
 uint8_t*list=ram_pointer(ram,IDS_ADDRESS+side*SIDE_STRIDE);
 if(!count_ptr||!list||!expected||!count||count>NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS)return 0;
 memset(count_ptr,0xa5,4);memset(list,0xa5,20*RECORD_STRIDE);
 supplied_play_query query={ids,count,0,{0},0};
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
 c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=BASE_R13;
 c.gpr[3]=side;c.pc=0x8011d628;c.lr=0;c.downcount=c.cycle_budget=100000000;
 for(unsigned step=0;c.pc&&step<100000;++step){uint32_t pc=c.pc;
  if(pc==0x8014909c){c.gpr[3]=query.count;c.pc=c.lr;}
  else if(pc==0x801490d0){query.index=0;c.gpr[3]=query.count!=0;c.pc=c.lr;}
  else if(pc==0x8014e760){if(query.index>=query.count)return 0;c.gpr[3]=query.ids[query.index];c.pc=c.lr;}
  else if(pc==0x8014919c){if(query.index<query.count)++query.index;c.gpr[3]=query.index<query.count;c.pc=c.lr;}
  else if(pc==0x80129068){if(c.gpr[4]!=0||query.clear_count>=7)return 0;query.clear_slots[query.clear_count++]=c.gpr[3];c.pc=c.lr;}
  else{c.downcount=-1000000000;m->dispatch(&c,pc);}
  if(c.exception)return 0;
 }
 if(c.pc||query.index!=count||query.clear_count!=7||be(count_ptr)!=count)return 0;
 for(unsigned i=0;i<count;++i)if(be(list+i*RECORD_STRIDE)!=expected->player_ids[i])return 0;
 for(unsigned i=0;i<7;++i)if(query.clear_slots[i]!=side*7+i||expected->cleared_slots[i]!=query.clear_slots[i])return 0;
 for(unsigned i=0;i<20*RECORD_STRIDE;++i){
  if(i%RECORD_STRIDE<4&&i/RECORD_STRIDE<count)continue;
  if(list[i]!=0xa5)return 0;
 }
 return 1;
}

static int original_probe(const char*dll,const char*snapshot,const char*folder){
 HMODULE h=LoadLibraryA(dll);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();
 if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
 size_t sn=0;uint8_t*ram=load_file(snapshot,&sn,RAM_SIZE);
 if(!ram||sn!=RAM_SIZE){free(ram);FreeLibrary(h);return 4;}
 unsigned checks=0,total_ids=0;
 for(unsigned entry=1;entry<=32;++entry){uint8_t*b=NULL;size_t n=0;
  if(!load_team(folder,entry,&b,&n)){free(ram);FreeLibrary(h);return 5;}
  uint32_t team=0,count=0,ids[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS];
  if(!nflstreet_team_preview_team_id_from_db(b,n,&team)||
     !nflstreet_team_preview_roster_from_db(b,n,team,ids,&count)){free(b);free(ram);FreeLibrary(h);return 6;}
  for(unsigned side=0;side<2;++side){nflstreet_team_preview_roster expected={0};
   if(!nflstreet_team_preview_roster_select(&expected,side,team,ids,count)||
      !run_original(m,ram,side,ids,count,&expected)){
    fprintf(stderr,"Original8011D628 roster projection differs entry%u team%u side%u count%u\n",entry,team,side,count);
    free(b);free(ram);FreeLibrary(h);return 7;
   }
   ++checks;total_ids+=count;
  }
  free(b);
 }
 printf("Original8011D628 roster projection exact: %u side/team comparisons, %u supplied PLAY/DIGP IDs from32 private blocks; original count/list writes and seven 80129068 clear requests match. DB iterator outputs supplied from TEAM/DIGT-filtered PLAY rows; class3 rendering not executed.\n",checks,total_ids);
 free(ram);FreeLibrary(h);return 0;
}
#endif

int main(int argc,char**argv){
#ifdef _WIN32
 if(argc==4)return original_probe(argv[1],argv[2],argv[3]);
#endif
 if(argc==2)return native_probe(argv[1]);
 fprintf(stderr,"Usage: team-preview-check.exe <db-teams-folder> | <original-DLL> <snapshot> <db-teams-folder>\n");return 2;
}
