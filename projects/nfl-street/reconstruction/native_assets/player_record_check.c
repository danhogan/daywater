/* Complete 800DE000 player records from the match GAME PLAY rows. The snapshot
   is fixture input only: it supplies the GAME PLAY table (built at match setup,
   not reconstructed), the save's created STAT TEAM row, the actor player ids
   (+135C) and slots (+1363) passed by 800DF868, and the 800DE6C8 side inputs.
   TEAM, LCGP and GEAR rows come from the on-disc template DB. Each captured
   record is rebuilt from a copy whose written bytes are poisoned. A controlled
   corpus then rewrites the ITGT and 1AHP fields of GAME rows. */
#include "player_record.h"
#include "game_roster.h"
#include "db_fixture.h"
#define CAPTURED_PLAYERS 14u
#define GAME_ROWS 23u
#define PLAYER_RECORDS 0x802aaa60u
#define R13 0x803a9a40u
#define ACTORS (R13-0x7bbcu)
#define SIDE_FLAG (R13-0x6f68u)
#define SIDE_WORD 0x8030b1fcu
#define GAME_PLAY 0x804b1194u
#define STAT_TEAM 0x80482fb4u
#define TAG_PLAY 0x59414c50u
#define TAG_TEAM 0x4d414554u
#define TAG_LCGP 0x4c434750u
#define TAG_GEAR 0x52414547u
#define TAG_ITGT 0x49544754u
#define TAG_DIGT 0x44494754u
#define TAG_DIGP 0x44494750u
#define TAG_1AHP 0x31414850u
#define POISON 0xa5u
/* Every byte 800DE000 writes; +0B, +3D and +65..+67 are retained. */
static void poison(uint8_t r[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]){
 for(unsigned i=0;i<0x65;++i)if(i!=0x0b&&i!=0x3d)r[i]=POISON;}
typedef struct {unsigned player,slot,side;uint32_t id;uint8_t r13_6f68;uint32_t word;} call_args;
#ifdef NFLSTREET_PLAYER_ORACLE
static unsigned oracle_calls;
static int initialize_player_oracle(const char *dll,const char *snapshot);
static int original_player_record(const nflstreet_team_table *game,unsigned row,uint32_t tag,uint32_t value,const call_args *,
 const uint8_t before[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],const uint8_t expected[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]);
#endif
static nflstreet_gear_records records;
int main(int argc,char **argv){
#ifdef NFLSTREET_PLAYER_ORACLE
 if(argc!=6)return 2;
#else
 if(argc!=5)return 2;
#endif
 size_t sizes[4];uint8_t *data[4];for(unsigned i=0;i<4;++i)if(!(data[i]=load_data(argv[1+i],sizes+i)))return 3;
 uint8_t *ram=data[3];nflstreet_gear_texture_catalog catalog;
 nflstreet_team_table gear,teams,colors,game_schema,game,live_teams;
 if(sizes[3]!=RAM_BYTES||!nflstreet_gear_texture_catalog_open(data[0],sizes[0],&catalog)||
    !nflstreet_db_schema_open(data[1],sizes[1],TAG_PLAY,&game_schema)||!nflstreet_team_table_open_tag(data[2],sizes[2],TAG_GEAR,&gear)||
    !nflstreet_team_table_open_tag(data[2],sizes[2],TAG_TEAM,&teams)||!nflstreet_team_table_open_tag(data[2],sizes[2],TAG_LCGP,&colors)||
    !nflstreet_gear_records_build(&gear,&catalog,&records)||!ram_table(ram,GAME_PLAY,TAG_PLAY,&game_schema,&game)||
    !ram_table(ram,STAT_TEAM,TAG_TEAM,&teams,&live_teams)||game.used!=GAME_ROWS)return 3;
#ifdef NFLSTREET_PLAYER_ORACLE
 if(!initialize_player_oracle(argv[5],argv[4]))return 4;
#endif
 /* The live STAT TEAM rows are the disc rows plus the save's created team(s). */
 if(live_teams.used<=teams.used||memcmp(ram+live_teams.records_at,teams.bytes+teams.records_at,(size_t)teams.used*teams.record_bytes))return 5;
 const uint8_t *actors=ram_at(ram,rd32(ram_at(ram,ACTORS,4)),4*CAPTURED_PLAYERS),*flag=ram_at(ram,SIDE_FLAG,1),*word=ram_at(ram,SIDE_WORD,4);
 if(!actors||!flag||!word)return 5;
 uint32_t digest=2166136261u;unsigned live=0,cases=0,rejected=0,both=0,sides[2]={0};
 for(unsigned p=0;p<CAPTURED_PLAYERS;++p){const uint8_t *object=ram_at(ram,rd32(actors+4*p),0x1364);if(!object)return 5;
  call_args a={p,object[0x1363],0,rd32(object+0x135c),*flag,rd32(word)};a.side=nflstreet_player_side(p,a.r13_6f68,a.word);
  const uint8_t *captured=ram_at(ram,PLAYER_RECORDS+a.slot*0x68u,0x68);unsigned row;
  if(a.slot!=p||!captured||!find(&game,TAG_DIGP,a.id,&row))return 5;
  uint8_t before[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],built[2][NFLSTREET_GEAR_PLAYER_RECORD_BYTES];memcpy(before,captured,sizeof before);poison(before);
  for(unsigned v=0;v<2;++v){memcpy(built[v],before,sizeof before);
   if(!nflstreet_player_record_build(&game,row,&records,v?&live_teams:&teams,&colors,&catalog,a.side,built[v])){fprintf(stderr,"live record failed %u\n",p);return 6;}}
  if(memcmp(built[0],captured,0x68)||memcmp(built[1],captured,0x68)){fprintf(stderr,"live record mismatch player%u\n",p);return 6;}
#ifdef NFLSTREET_PLAYER_ORACLE
  if(!original_player_record(&game,row,0,0,&a,before,built[0]))return 7;
#endif
  digest=fnv(digest,built[0],sizeof built[0]);++live;
 }
 /* Controlled corpus: every GAME row with every live STAT team id and four
    800DE6C8 input pairs, then a 1AHP sweep. The GAME RAM row is edited. */
 static const uint8_t flags[4]={0,0,1,1};static const uint32_t words[4]={0,1,0,0x100};
 for(unsigned row=0;row<GAME_ROWS;++row)for(unsigned team=0;team<live_teams.used;++team)for(unsigned k=0;k<4;++k){
  uint32_t id,old;unsigned disc;
  call_args a={(row+team)%CAPTURED_PLAYERS,0,0,0,flags[k],words[k]};a.slot=a.player;a.side=nflstreet_player_side(a.player,a.r13_6f68,a.word);
  if(!number_of(&live_teams,team,TAG_DIGT,&id)||!number_of(&game,row,TAG_ITGT,&old)||!number_of(&game,row,TAG_DIGP,&a.id)||
     !set_number(ram,&game,row,TAG_ITGT,id))return 5;
  uint8_t before[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],built[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];
  memcpy(before,ram_at(ram,PLAYER_RECORDS+a.slot*0x68u,0x68),sizeof before);poison(before);memcpy(built,before,sizeof built);
  if(!nflstreet_player_record_build(&game,row,&records,&live_teams,&colors,&catalog,a.side,built)){fprintf(stderr,"corpus failed %u/%u\n",row,team);return 6;}
  if(find(&teams,TAG_DIGT,id,&disc)){uint8_t again[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];memcpy(again,before,sizeof again);
   if(!nflstreet_player_record_build(&game,row,&records,&teams,&colors,&catalog,a.side,again)||memcmp(again,built,sizeof built))return 6;++both;}
#ifdef NFLSTREET_PLAYER_ORACLE
  if(!original_player_record(&game,row,TAG_ITGT,id,&a,before,built))return 7;
#endif
  if(!set_number(ram,&game,row,TAG_ITGT,old))return 5;
  digest=fnv(digest,built,sizeof built);++cases;++sides[a.side];
 }
 for(uint32_t value=0;value<128;++value)for(unsigned k=0;k<2;++k){const unsigned row=value%GAME_ROWS;uint32_t old;
  call_args a={value%CAPTURED_PLAYERS,value%CAPTURED_PLAYERS,0,0,(uint8_t)k,0};a.side=nflstreet_player_side(a.player,a.r13_6f68,a.word);
  if(!number_of(&game,row,TAG_1AHP,&old)||!number_of(&game,row,TAG_DIGP,&a.id)||!set_number(ram,&game,row,TAG_1AHP,value))return 5;
  uint8_t before[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],built[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];
  memcpy(before,ram_at(ram,PLAYER_RECORDS+a.slot*0x68u,0x68),sizeof before);poison(before);memcpy(built,before,sizeof built);
  if(!nflstreet_player_record_build(&game,row,&records,&live_teams,&colors,&catalog,a.side,built)||built[0x5e]!=(uint8_t)(value+0x80u))return 6;
#ifdef NFLSTREET_PLAYER_ORACLE
  if(!original_player_record(&game,row,TAG_1AHP,value,&a,before,built))return 7;
#endif
  if(!set_number(ram,&game,row,TAG_1AHP,old))return 5;
  digest=fnv(digest,built,sizeof built);++cases;++sides[a.side];
 }
 /* Unmodeled inputs reject: a TEAM id with no row and an LCGP row for the player. */
 {uint8_t r[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]={0};uint32_t id,old;
  if(!number_of(&game,0,TAG_ITGT,&old)||!set_number(ram,&game,0,TAG_ITGT,999))return 5;
  rejected+=!nflstreet_player_record_build(&game,0,&records,&live_teams,&colors,&catalog,0,r);
  if(!set_number(ram,&game,0,TAG_ITGT,old))return 5;
  if(!number_of(&game,0,TAG_DIGP,&id)||!set_number(data[2],&colors,0,TAG_DIGP,id))return 5;
  rejected+=!nflstreet_player_record_build(&game,0,&records,&live_teams,&colors,&catalog,0,r);}
 printf("Player records: %u captured records rebuilt byte-identical from poisoned copies (GAME PLAY rows, disc and live STAT TEAM tables); %u controlled records (%u GAME rows x %u STAT teams x4 side inputs, 256 1AHP cases; side0/1 %u/%u; %u disc-team cases agree); %u unmodeled inputs rejected; digest %08x.\n",
  live,cases,GAME_ROWS,live_teams.used,sides[0],sides[1],both,rejected,digest);
#ifdef NFLSTREET_PLAYER_ORACLE
 printf("Original player record checks: %u 800DE6C8 calls on the live GAME/STAT databases match all 104 record bytes.\n",oracle_calls);
#endif
 for(unsigned i=0;i<4;++i)free(data[i]);return live==CAPTURED_PLAYERS&&rejected==2?0:8;
}
