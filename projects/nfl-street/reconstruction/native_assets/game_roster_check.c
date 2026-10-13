/* Match GAME PLAY tables built natively from the disc STAT PLAY/TEAM tables,
   then complete player records from them. The snapshot is fixture input only:
   it supplies the selected teams (live MTTS DIOT in UNLS order), the menu's
   lineup GILP values (the live STAT rows that differ from disc), the live GAME
   PLAY table to compare, and actor ids/slots/side inputs. A controlled corpus
   pairs every disc team with two partners, with lineup and DIOP overrides. */
#include "game_roster.h"
#include "player_record.h"
#include "db_fixture.h"
#define CAPTURED_PLAYERS 14u
#define PLAYER_RECORDS 0x802aaa60u
#define R13 0x803a9a40u
#define ACTORS (R13-0x7bbcu)
#define SIDE_FLAG (R13-0x6f68u)
#define SIDE_WORD 0x8030b1fcu
#define GAME_PLAY 0x804b1194u
#define STAT_PLAY 0x80452484u
#define SELECTED_TEAMS 0x804ae3a4u
#define TAG_PLAY 0x59414c50u
#define TAG_TEAM 0x4d414554u
#define TAG_LCGP 0x4c434750u
#define TAG_GEAR 0x52414547u
#define TAG_MTTS 0x4d545453u
#define TAG_UNLS 0x554e4c53u
#define TAG_DIOT 0x44494f54u
#define TAG_DIGT 0x44494754u
#define TAG_DIGP 0x44494750u
#define TAG_DIOP 0x44494f50u
#define TAG_GILP 0x47494c50u
#define MAX_OVERRIDES 32u
typedef struct {uint32_t teams[2];nflstreet_stat_override overrides[MAX_OVERRIDES];unsigned override_count;} roster_case;
#ifdef NFLSTREET_ROSTER_ORACLE
static unsigned oracle_cases;
static int initialize_roster_oracle(const char *dll,const char *snapshot,const nflstreet_team_table *stat_play_schema,
 const nflstreet_team_table *selected_schema,const nflstreet_team_table *game_schema);
static int original_roster(const roster_case *,const nflstreet_team_table *disc_play,const nflstreet_team_table *disc_teams,
 const nflstreet_stat_override *live_lineup,unsigned live_count,const nflstreet_game_roster *expected);
#endif
static nflstreet_gear_records records;
static int same_records(const nflstreet_game_roster *r,const nflstreet_team_table *live){
 return r->table.used==live->used&&r->table.record_bytes==live->record_bytes&&
  !memcmp(r->table.bytes+r->table.records_at,live->bytes+live->records_at,(size_t)live->used*live->record_bytes);}
int main(int argc,char **argv){
#ifdef NFLSTREET_ROSTER_ORACLE
 if(argc!=6)return 2;
#else
 if(argc!=5)return 2;
#endif
 size_t sizes[4];uint8_t *data[4];for(unsigned i=0;i<4;++i)if(!(data[i]=load_data(argv[1+i],sizes+i)))return 3;
 const uint8_t *ram=data[3];nflstreet_gear_texture_catalog catalog;
 nflstreet_team_table game_schema,selected_schema,play,thcd,teams,colors,gear,live_game,live_play,selected;
 if(sizes[3]!=RAM_BYTES||!nflstreet_gear_texture_catalog_open(data[0],sizes[0],&catalog)||
    !nflstreet_db_schema_open(data[1],sizes[1],TAG_PLAY,&game_schema)||!nflstreet_db_schema_open(data[1],sizes[1],TAG_MTTS,&selected_schema)||
    !nflstreet_team_table_open_tag(data[2],sizes[2],TAG_PLAY,&play)||!nflstreet_team_table_open_tag(data[2],sizes[2],0x54484344u,&thcd)||!nflstreet_team_table_open_tag(data[2],sizes[2],TAG_TEAM,&teams)||
    !nflstreet_team_table_open_tag(data[2],sizes[2],TAG_LCGP,&colors)||!nflstreet_team_table_open_tag(data[2],sizes[2],TAG_GEAR,&gear)||
    !nflstreet_gear_records_build(&gear,&catalog,&records)||!ram_table(ram,GAME_PLAY,TAG_PLAY,&game_schema,&live_game)||
    !ram_table(ram,STAT_PLAY,TAG_PLAY,&play,&live_play)||!ram_table(ram,SELECTED_TEAMS,TAG_MTTS,&selected_schema,&selected))return 3;
#ifdef NFLSTREET_ROSTER_ORACLE
 if(!initialize_roster_oracle(argv[5],argv[4],&play,&selected_schema,&game_schema))return 4;
#endif
 /* Live STAT PLAY = disc rows (same order) with the menu's GILP values, plus
    created-team rows appended. Only GILP may differ in the disc rows. */
 roster_case live={0};nflstreet_stat_override lineup[MAX_OVERRIDES];unsigned lineup_count=0;
 if(live_play.used<play.used)return 5;
 for(unsigned r=0;r<play.used;++r){const uint8_t *a=play.bytes+play.records_at+(size_t)r*play.record_bytes,*b=ram+live_play.records_at+(size_t)r*play.record_bytes;
  if(!memcmp(a,b,play.record_bytes))continue;uint32_t digp,value,disc;
  if(lineup_count>=MAX_OVERRIDES||!number_of(&play,r,TAG_DIGP,&digp)||!number_of(&live_play,r,TAG_GILP,&value)||!number_of(&play,r,TAG_GILP,&disc))return 5;
  lineup[lineup_count++]=(nflstreet_stat_override){digp,TAG_GILP,value};
  uint8_t copy[4096];memcpy(copy,a,play.record_bytes);
  if(!set_record_number(copy,&play,TAG_GILP,value)||memcmp(copy,b,play.record_bytes))return 5;}
 if(selected.used!=2)return 5;
 for(unsigned u=0;u<2;++u){unsigned row;uint32_t team;if(!find(&selected,TAG_UNLS,u,&row)||!number_of(&selected,row,TAG_DIOT,&team))return 5;live.teams[u]=team;}
 memcpy(live.overrides,lineup,sizeof lineup[0]*lineup_count);live.override_count=lineup_count;
 nflstreet_game_roster roster;
 if(!nflstreet_game_roster_build(&play,&thcd,&teams,&game_schema,live.teams,2,live.overrides,live.override_count,&roster))return 6;
 if(!same_records(&roster,&live_game)){fprintf(stderr,"live GAME PLAY mismatch\n");return 6;}
#ifdef NFLSTREET_ROSTER_ORACLE
 if(!original_roster(&live,&play,&teams,lineup,lineup_count,&roster))return 7;
#endif
 /* The captured players' complete records from the native GAME PLAY table. */
 const uint8_t *actors=ram_at(ram,rd32(ram_at(ram,ACTORS,4)),4*CAPTURED_PLAYERS),*flag=ram_at(ram,SIDE_FLAG,1),*word=ram_at(ram,SIDE_WORD,4);
 if(!actors||!flag||!word)return 5;unsigned live_records=0;
 for(unsigned p=0;p<CAPTURED_PLAYERS;++p){const uint8_t *object=ram_at(ram,rd32(actors+4*p),0x1364);unsigned row;if(!object)return 5;
  const uint8_t *captured=ram_at(ram,PLAYER_RECORDS+object[0x1363]*0x68u,0x68);
  if(!captured||!find(&roster.table,TAG_DIGP,rd32(object+0x135c),&row))return 5;uint8_t record[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];
  memcpy(record,captured,sizeof record);for(unsigned i=0;i<0x65;++i)if(i!=0x0b&&i!=0x3d)record[i]=0xa5;
  if(!nflstreet_player_record_build(&roster.table,row,&records,&teams,&colors,&catalog,nflstreet_player_side(p,*flag,rd32(word)),record)||
     memcmp(record,captured,sizeof record)){fprintf(stderr,"record from native GAME row mismatch player%u\n",p);return 6;}
  ++live_records;}
 unsigned live_rows=roster.table.used;nflstreet_game_roster_close(&roster);
 /* Controlled corpus over disc teams: partners +1 and +7 (team row order),
    the first seven rows by DIGP of each team as lineup, and a DIOP rewrite. */
 uint32_t digest=2166136261u;unsigned cases=0,rows=0,records_built=0,rejected=0;
 for(unsigned t=0;t<teams.used;++t)for(unsigned k=0;k<2;++k){roster_case c={0};
  if(!number_of(&teams,t,TAG_DIGT,c.teams)||!number_of(&teams,(t+(k?7u:1u))%teams.used,TAG_DIGT,c.teams+1))return 5;
  for(unsigned u=0;u<2;++u){uint32_t ids[64];unsigned n=0;
   for(unsigned r=0;r<play.used;++r){uint32_t team,digp;if(!number_of(&play,r,TAG_DIGT,&team)||!number_of(&play,r,TAG_DIGP,&digp))return 5;
    if(team!=c.teams[u])continue;if(n>=64)return 5;unsigned at=n++;while(at&&ids[at-1]>digp){ids[at]=ids[at-1];--at;}ids[at]=digp;}
   for(unsigned i=0;i<n&&i<7;++i)c.overrides[c.override_count++]=(nflstreet_stat_override){ids[i],TAG_GILP,u+1u};
   if(n&&!u)c.overrides[c.override_count++]=(nflstreet_stat_override){ids[n-1],TAG_DIOP,900u+t};}
  if(!nflstreet_game_roster_build(&play,&thcd,&teams,&game_schema,c.teams,2,c.overrides,c.override_count,&roster)){fprintf(stderr,"corpus roster failed %u/%u\n",t,k);return 6;}
#ifdef NFLSTREET_ROSTER_ORACLE
  if(!original_roster(&c,&play,&teams,lineup,lineup_count,&roster))return 7;
#endif
  for(unsigned r=0;r<roster.table.used;++r){uint8_t record[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]={0};uint32_t slot;
   if(!number_of(&roster.table,r,TAG_DIGT,&slot)||!nflstreet_player_record_build(&roster.table,r,&records,&teams,&colors,&catalog,slot,record))return 6;
   digest=fnv(digest,record,sizeof record);++records_built;}
  digest=fnv(digest,roster.table.bytes+roster.table.records_at,(size_t)roster.table.used*roster.table.record_bytes);
  rows+=roster.table.used;++cases;nflstreet_game_roster_close(&roster);
 }
 {const uint32_t missing[2]={live.teams[0],999u};const nflstreet_stat_override stray={0x7fffu,TAG_GILP,1};
  rejected+=!nflstreet_game_roster_build(&play,&thcd,&teams,&game_schema,missing,2,NULL,0,&roster);
  rejected+=!nflstreet_game_roster_build(&play,&thcd,&teams,&game_schema,live.teams,2,&stray,1,&roster);}
 printf("GAME rosters: live GAME PLAY table rebuilt byte-identical (%u rows, %u lineup values) and %u captured records from it; %u controlled team pairs (%u rows, %u player records); %u unmodeled inputs rejected; digest %08x.\n",
  live_rows,lineup_count,live_records,cases,rows,records_built,rejected,digest);
#ifdef NFLSTREET_ROSTER_ORACLE
 printf("Original GAME roster checks: %u 801E3828/801E3720/801E05A4 loads match every GAME PLAY row byte for byte.\n",oracle_cases);
#endif
 for(unsigned i=0;i<4;++i)free(data[i]);return live_records==CAPTURED_PLAYERS&&rejected==2?0:8;
}
