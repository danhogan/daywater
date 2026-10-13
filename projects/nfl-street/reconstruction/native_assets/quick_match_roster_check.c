#include "quick_match_roster.h"
#include "db_fixture.h"
#ifdef NFLSTREET_QUICK_ROSTER_ORACLE
static int flag_oracle(uint32_t,unsigned,int,uint32_t);
static int initialize_flag_oracle(const char *,const char *);
#endif
int main(int argc,char **argv){
#ifdef NFLSTREET_QUICK_ROSTER_ORACLE
 if(argc!=7||!initialize_flag_oracle(argv[3],argv[4]))return 2;
#else
 if(argc!=5)return 2;
#endif
 size_t sizes[2];uint8_t *data[2]={load_data(argv[1],sizes),load_data(argv[2],sizes+1)};
 nflstreet_team_table play,teams,game_schema;if(!data[0]||!data[1]||!nflstreet_team_table_open_tag(data[0],sizes[0],0x59414c50u,&play)||
  !nflstreet_team_table_open_tag(data[0],sizes[0],0x4d414554u,&teams)||!nflstreet_db_schema_open(data[1],sizes[1],0x59414c50u,&game_schema))return 3;
 uint8_t *unchanged=malloc(sizes[0]);if(!unchanged)return 3;memcpy(unchanged,data[0],sizes[0]);
 unsigned cases=0,players=0,positions=0,rejections=0,flags_checked=0;uint32_t random=0x12345678u;
 for(unsigned i=0;i<1024;++i){random=random*1664525u+1013904223u;uint32_t prior=i<4?(uint32_t[]){0,3,0xffffffffu,0x80000000u}[i]:random;
  for(unsigned side=0;side<4;++side)for(unsigned selected=0;selected<2;++selected){uint32_t expected=side?(selected?prior|2u:prior&~2u):(selected?prior|1u:prior&~1u);
   if(nflstreet_lineup_flag(prior,side,(int)selected)!=expected)return 4;
#ifdef NFLSTREET_QUICK_ROSTER_ORACLE
   if(!flag_oracle(prior,side,(int)selected,expected))return 4;
#endif
   ++flags_checked;}}
 for(unsigned t=0;t<teams.used;++t)for(unsigned partner=0;partner<2;++partner)for(unsigned variant=0;variant<3;++variant){
  uint32_t ids[2];if(!number_of(&teams,t,0x44494754u,ids)||!number_of(&teams,(t+(partner?7u:1u))%teams.used,0x44494754u,ids+1))return 5;
  nflstreet_team_activation activation={0};nflstreet_field_bootstrap fields={0};nflstreet_quick_match_selection selection;
  fields.keys[0]=3;fields.records[0xca]=1;
  for(unsigned side=0;side<2;++side){uint32_t roster[64];unsigned count=0;
   for(unsigned row=0;row<play.used;++row){uint32_t team,id;if(!number_of(&play,row,0x44494754u,&team)||!number_of(&play,row,0x44494750u,&id))return 5;
    if(team==ids[side]){if(count>=64)return 5;roster[count++]=id;}}
   if(count<7)return 5;
   nflstreet_team_activation_side *a=activation.sides+side;a->selected_count=a->formation_count=7;
   for(unsigned p=0;p<7;++p){unsigned at=variant==1?count-7+p:variant==2?(p+3)%7:p;a->selected_ids[p]=roster[at];}
   for(unsigned p=0;p<7;++p){a->active_formation[p]=a->selected_ids[(p+2)%7];a->active_formation[7+p]=a->selected_ids[6-p];}
  }
  if(!nflstreet_quick_match_selection_build(ids,&activation,&fields,0,&selection))return 5;
  nflstreet_quick_match_roster r={0};if(!nflstreet_quick_match_roster_build(data[0],sizes[0],&game_schema,&selection,&r))return 6;
  if(memcmp(&r.selection,&selection,sizeof selection))return 6;
  for(unsigned row=0;row<r.game.table.used;++row){uint32_t side,id,original_id,prior,actual;
   if(!number_of(&r.game.table,row,0x44494754u,&side)||!number_of(&r.game.table,row,0x44494750u,&id)||side>1||
      !number_of(&play,r.game.source_rows[row],0x44494750u,&original_id)||!number_of(&play,r.game.source_rows[row],0x47494c50u,&prior)||
      !number_of(&r.game.table,row,0x47494c50u,&actual))return 6;
   int chosen=0;for(unsigned p=0;p<7;++p)chosen|=selection.selected_players[side][p]==original_id;
   uint32_t expected=nflstreet_lineup_flag(prior,side,chosen);if(actual!=expected)return 6;
  }
  for(unsigned side=0;side<2;++side){for(unsigned p=0;p<7;++p){unsigned row=r.selected_rows[side][p];uint32_t id,original_id;
    if(!number_of(&r.game.table,row,0x44494750u,&id)||id!=r.selected_game_ids[side][p]||
       !number_of(&play,r.game.source_rows[row],0x44494750u,&original_id)||original_id!=selection.selected_players[side][p])return 6;++players;}
   for(unsigned p=0;p<14;++p){unsigned row=r.formation_rows[side][p];uint32_t id,original_id;
    if(!number_of(&r.game.table,row,0x44494750u,&id)||id!=r.formation_game_ids[side][p]||
       !number_of(&play,r.game.source_rows[row],0x44494750u,&original_id)||original_id!=selection.formation_players[side][p])return 6;++positions;}}
  nflstreet_quick_match_roster before=r;if(nflstreet_quick_match_roster_build(data[0],sizes[0],&game_schema,&selection,&r)||memcmp(&r,&before,sizeof r))return 7;++rejections;
  if(!cases)for(unsigned fault=0;fault<5;++fault){nflstreet_quick_match_selection bad=selection;
   if(fault==0)bad.selected_players[0][1]=bad.selected_players[0][0];if(fault==1)bad.selected_players[0][0]=bad.selected_players[1][0];
   if(fault==2)bad.formation_players[0][1]=bad.formation_players[0][0];if(fault==3)bad.field_key=8;if(fault==4)bad.team_ids[1]=bad.team_ids[0];
   nflstreet_quick_match_roster out={0},saved=out;out.selection.field_key=saved.selection.field_key=0xa5;
   if(nflstreet_quick_match_roster_build(data[0],sizes[0],&game_schema,&bad,&out)||memcmp(&out,&saved,sizeof out))return 7;++rejections;}
  nflstreet_quick_match_roster_close(&r);nflstreet_quick_match_roster_close(&r);++cases;
 }
 if(memcmp(data[0],unchanged,sizes[0]))return 8;free(unchanged);

#ifdef NFLSTREET_QUICK_ROSTER_ORACLE
 const char *capture=argv[5],*expected_text=argv[6];
#else
 const char *capture=argv[3],*expected_text=argv[4];
#endif
 size_t capture_size;uint8_t *capture_bytes=load_data(capture,&capture_size);
 if(!capture_bytes||capture_size!=192||memcmp(capture_bytes,"QMS1",4)||rd32(capture_bytes+4)!=1)return 8;
 nflstreet_quick_match_selection captured={0};unsigned at=8;
 for(unsigned i=0;i<2;++i,at+=4)captured.team_ids[i]=rd32(capture_bytes+at);
 for(unsigned side=0;side<2;++side)for(unsigned p=0;p<7;++p,at+=4)captured.selected_players[side][p]=rd32(capture_bytes+at);
 for(unsigned side=0;side<2;++side)for(unsigned p=0;p<14;++p,at+=4)captured.formation_players[side][p]=rd32(capture_bytes+at);
 captured.field_slot=rd32(capture_bytes+at);at+=4;uint32_t key=rd32(capture_bytes+at);if(key>255)return 8;captured.field_key=(uint8_t)key;
 nflstreet_quick_match_roster rejected={0},saved=rejected;
 /* Current captured native picker chose mirrored teams, an explicitly
    unsupported route; it must not silently produce a mismatched match. */
 if(captured.team_ids[0]!=captured.team_ids[1]||nflstreet_quick_match_roster_build(data[0],sizes[0],&game_schema,&captured,&rejected)||memcmp(&rejected,&saved,sizeof saved))return 8;
 unsigned matched=0;
 for(unsigned side=0;side<2;++side)for(unsigned p=0;p<7;++p){unsigned found=0;
  for(unsigned row=0;row<play.used;++row){uint32_t team,id;if(!number_of(&play,row,0x44494754u,&team)||!number_of(&play,row,0x44494750u,&id))return 8;
   found|=team==captured.team_ids[side]&&id==captured.selected_players[side][p];}matched+=found;}
 char *end;unsigned long expected_match=strtoul(expected_text,&end,10);
 if(!*expected_text||*end||expected_match>14||matched!=expected_match)return 8;
 free(capture_bytes);++rejections;
 free(data[0]);free(data[1]);printf("Quick-match rosters: %u menu selections, %u selected GAME rows, %u formation mappings; %u rejection checks; %u lineup-bit cases. STAT unchanged; no actors created.\n",cases,players,positions,rejections,flags_checked);printf("Retained native menu snapshot: %u/14 ids match STAT team membership; mirrored route rejects without creating a roster.\n",matched);return 0;
}
