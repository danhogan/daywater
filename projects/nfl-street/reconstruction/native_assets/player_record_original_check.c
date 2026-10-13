/* Original 800DE6C8 -> 800DE000 against the live databases in the snapshot:
   GAME PLAY, STAT TEAM/LCGP and GEAR are read by the original query code
   itself (801B1AC8 and friends execute). Only 801AF944/9CC/57C synchronization
   returns. The same GAME field edits and side inputs are applied to the RAM
   image before each call, and all 104 record bytes are compared. */
#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#define NFLSTREET_PLAYER_ORACLE
#include "player_record_check.c"
#define ORACLE_STACK 0x810f0000u
static const ModernGekkoModuleDesc *player_module;static uint8_t *player_ram;
static int initialize_player_oracle(const char *dll,const char *snapshot){HMODULE h=LoadLibraryA(dll);if(!h)return 0;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 0;
 player_module=get();player_ram=load(snapshot);return player_module&&player_ram;}
static int run_side(const call_args *a){
 CPUState c={0};c.ram=player_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=ORACLE_STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 c.gpr[3]=a->player;c.gpr[4]=a->slot;c.gpr[5]=0;c.gpr[6]=a->id;c.pc=0x800de6c8u;
 for(unsigned step=0;c.pc&&step<5000000;++step){uint32_t pc=c.pc;
  if(pc==0x801af944u||pc==0x801af9ccu){c.gpr[3]=0;c.pc=c.lr;continue;}
  if(pc==0x801af57cu){c.pc=c.lr;continue;}
  c.downcount=-1000000000;player_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 return !c.pc;
}
static int original_player_record(const nflstreet_team_table *game,unsigned row,uint32_t tag,uint32_t value,const call_args *a,
 const uint8_t before[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],const uint8_t expected[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]){
 uint8_t *record=ptr(player_ram,PLAYER_RECORDS+a->slot*0x68u,0x68),*flag=ptr(player_ram,SIDE_FLAG,1),*word=ptr(player_ram,SIDE_WORD,4);
 uint32_t old=0;uint8_t saved_flag;uint8_t saved_word[4];if(!record||!flag||!word)return 0;
 nflstreet_team_table view=*game;view.bytes=player_ram; /* same layout in the oracle's RAM image */
 if(tag&&(!number_of(&view,row,tag,&old)||!set_number(player_ram,&view,row,tag,value)))return 0;
 saved_flag=*flag;memcpy(saved_word,word,4);*flag=a->r13_6f68;put(word,a->word);memcpy(record,before,0x68);
 int ok=run_side(a);
 if(ok&&memcmp(record,expected,0x68)){unsigned i=0;while(record[i]==expected[i])++i;
  fprintf(stderr,"original record mismatch player%u row%u tag%08x=%u at +%02x: %02x vs %02x\n",a->player,row,tag,value,i,record[i],expected[i]);ok=0;}
 *flag=saved_flag;memcpy(word,saved_word,4);
 if(tag&&!set_number(player_ram,&view,row,tag,old))return 0;
 if(!ok)return 0;++oracle_calls;return 1;
}
