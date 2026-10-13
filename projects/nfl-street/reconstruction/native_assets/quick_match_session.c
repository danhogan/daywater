#include "quick_match_session.h"
#include <stdlib.h>
#include <string.h>
void nflstreet_quick_match_session_close(nflstreet_quick_match_session *s){
 if(s){nflstreet_quick_match_roster_close(&s->roster);free(s->stat_bytes);memset(s,0,sizeof *s);}
}
int nflstreet_quick_match_session_open(const uint8_t *stat,size_t stat_size,
 const uint8_t *schema,size_t schema_size,const nflstreet_quick_match_selection *selection,
 nflstreet_quick_match_session *out){
 if(!stat||!stat_size||!schema||!selection||!out||out->stat_bytes||out->roster.game.bytes)return 0;
 nflstreet_team_table game_schema;nflstreet_quick_match_session s={0};
 if(!nflstreet_db_schema_open(schema,schema_size,0x59414c50u,&game_schema))return 0;
 s.stat_bytes=malloc(stat_size);if(!s.stat_bytes)return 0;
 memcpy(s.stat_bytes,stat,stat_size);s.stat_size=stat_size;
 if(!nflstreet_quick_match_roster_build(s.stat_bytes,s.stat_size,&game_schema,selection,&s.roster)){
  nflstreet_quick_match_session_close(&s);return 0;}
 *out=s;return 1;
}
