/* Original match-start GAME loads in the snapshot: per case the pristine RAM is
   restored, the STAT PLAY lineup/DIOP values and the two MTTS rows (DIGT =
   DIOT = IRDT = team, PYTT/DIGC/DIGD/NSID from the team, AMDT 0) are set, and
   the GAME PLAY/THCD tables are emptied (used = 0; the original clear before a
   fresh match is not traced). Then 801E3828(0x07000000), 801E3720(0x04000000)
   and 801E05A4(0x07000000, 0x03000000) run as in 801EA480. The db_teams.dat
   open (801E000C) is marked done because only teams missing from STAT use it;
   801E0110 (that load) fails the check. Synchronization returns. The GAME PLAY
   table must equal the native one byte for byte and MTTS DIGT the team slot. */
#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#define NFLSTREET_ROSTER_ORACLE
#include "game_roster_check.c"
#define ORACLE_STACK 0x810f0000u
#define GAME_THCD 0x804b2ca4u
#define DB_TEAMS_OPEN (R13-0x6354u)
static const ModernGekkoModuleDesc *roster_module;static uint8_t *roster_ram,*pristine;
static const nflstreet_team_table *stat_schema,*mtts_schema,*play_schema;
static int initialize_roster_oracle(const char *dll,const char *snapshot,const nflstreet_team_table *stat_play_schema,
 const nflstreet_team_table *selected_schema,const nflstreet_team_table *game_schema){
 HMODULE h=LoadLibraryA(dll);if(!h)return 0;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 0;
 roster_module=get();pristine=load(snapshot);roster_ram=malloc(RAM_SIZE);stat_schema=stat_play_schema;mtts_schema=selected_schema;play_schema=game_schema;
 return roster_module&&pristine&&roster_ram;}
static int run(uint32_t pc,uint32_t a,uint32_t b){
 CPUState c={0};c.ram=roster_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=ORACLE_STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 c.gpr[3]=a;c.gpr[4]=b;c.pc=pc;
 for(unsigned step=0;c.pc&&step<20000000;++step){uint32_t p=c.pc;
  if(p==0x801e0110u){fprintf(stderr,"db_teams load requested\n");return 0;}
  if(p==0x801af944u||p==0x801af9ccu){c.gpr[3]=0;c.pc=c.lr;continue;}
  if(p==0x801af57cu){c.pc=c.lr;continue;}
  c.downcount=-1000000000;roster_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 return !c.pc&&!c.gpr[3];
}
static int stat_row(const nflstreet_team_table *disc,uint32_t digp,unsigned *row){return find(disc,TAG_DIGP,digp,row);}
static int original_roster(const roster_case *c,const nflstreet_team_table *disc_play,const nflstreet_team_table *disc_teams,
 const nflstreet_stat_override *live_lineup,unsigned live_count,const nflstreet_game_roster *expected){
 memcpy(roster_ram,pristine,RAM_SIZE);nflstreet_team_table stat,mtts,game;unsigned row;uint32_t v;
 if(!ram_table(roster_ram,STAT_PLAY,TAG_PLAY,stat_schema,&stat)||!ram_table(roster_ram,SELECTED_TEAMS,TAG_MTTS,mtts_schema,&mtts))return 0;
 for(unsigned i=0;i<live_count;++i)if(!stat_row(disc_play,live_lineup[i].digp,&row)||!number_of(disc_play,row,TAG_GILP,&v)||
  !set_number(roster_ram,&stat,row,TAG_GILP,v))return 0;
 for(unsigned i=0;i<c->override_count;++i)if(!stat_row(disc_play,c->overrides[i].digp,&row)||
  !set_number(roster_ram,&stat,row,c->overrides[i].tag,c->overrides[i].value))return 0;
 static const uint32_t copied[4]={0x50595454u,0x44494743u,0x44494744u,0x4e534944u}; /* PYTT DIGC DIGD NSID */
 for(unsigned u=0;u<2;++u){unsigned team;if(!find(&mtts,TAG_UNLS,u,&row)||!find(disc_teams,TAG_DIGT,c->teams[u],&team))return 0;
  if(!set_number(roster_ram,&mtts,row,TAG_DIGT,c->teams[u])||!set_number(roster_ram,&mtts,row,TAG_DIOT,c->teams[u])||
     !set_number(roster_ram,&mtts,row,0x49524454u,c->teams[u])||!set_number(roster_ram,&mtts,row,0x414d4454u,0))return 0;
  for(unsigned k=0;k<4;++k)if(!number_of(disc_teams,team,copied[k],&v)||!set_number(roster_ram,&mtts,row,copied[k],v))return 0;}
 put(ptr(roster_ram,DB_TEAMS_OPEN,4),1);
 for(uint32_t header=GAME_PLAY;;header=GAME_THCD){uint8_t *used=ptr(roster_ram,header+0x5cu,2);used[0]=used[1]=0;if(header==GAME_THCD)break;}
 if(!run(0x801e3828u,0x07000000u,0)||!run(0x801e3720u,0x04000000u,0)||!run(0x801e05a4u,0x07000000u,0x03000000u)){
  fprintf(stderr,"original GAME load failed for teams %u/%u\n",c->teams[0],c->teams[1]);return 0;}
 if(!ram_table(roster_ram,GAME_PLAY,TAG_PLAY,play_schema,&game)||!same_records(expected,&game)){
  fprintf(stderr,"original GAME PLAY mismatch for teams %u/%u (%u vs %u rows)\n",c->teams[0],c->teams[1],game.used,expected->table.used);return 0;}
 for(unsigned u=0;u<2;++u)if(!find(&mtts,TAG_UNLS,u,&row)||!number_of(&mtts,row,TAG_DIGT,&v)||v!=u)return 0;
 ++oracle_cases;return 1;
}
