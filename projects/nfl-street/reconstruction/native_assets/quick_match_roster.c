#include "quick_match_roster.h"
#include <stdlib.h>
#include <string.h>
#define DIGT 0x44494754u
#define DIGP 0x44494750u
#define GILP 0x47494c50u
uint32_t nflstreet_lineup_flag(uint32_t v,unsigned side,int selected){uint32_t bit=side?2u:1u;return selected?v|bit:v&~bit;}
void nflstreet_quick_match_roster_close(nflstreet_quick_match_roster *r){if(r){nflstreet_game_roster_close(&r->game);memset(r,0,sizeof *r);}}
static int field(const nflstreet_team_table *t,uint32_t tag,unsigned *out){unsigned found=0;
 for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;if(f.tag==tag){if(found++)return 0;*out=i;}}return found==1;}
int nflstreet_quick_match_roster_build(const uint8_t *db,size_t size,const nflstreet_team_table *schema,
 const nflstreet_quick_match_selection *s,nflstreet_quick_match_roster *out){
 if(!s||!schema||!out||out->game.bytes||s->field_slot>=8||s->field_key>=8)return 0;
 nflstreet_team_table play,thcd,teams;unsigned team_field,id_field,flag_field;
 if(!nflstreet_team_table_open_tag(db,size,0x59414c50u,&play)||!nflstreet_team_table_open_tag(db,size,0x54484344u,&thcd)||
    !nflstreet_team_table_open_tag(db,size,0x4d414554u,&teams)||!field(&play,DIGT,&team_field)||!field(&play,DIGP,&id_field)||!field(&play,GILP,&flag_field))return 0;
 nflstreet_stat_override *changes=calloc(play.used?play.used:1,sizeof *changes);unsigned count=0;int ok=0;
 unsigned source[2][7];nflstreet_quick_match_roster r={0};
 if(!changes)return 0;
 for(unsigned side=0;side<2;++side){
  for(unsigned p=0;p<7;++p){source[side][p]=UINT32_MAX;
   if(s->selected_players[side][p]==0x7fffu||s->selected_players[side][p]==UINT32_MAX)goto done;
   for(unsigned prior=0;prior<p;++prior)if(s->selected_players[side][prior]==s->selected_players[side][p])goto done;
   for(unsigned row=0;row<play.used;++row){uint32_t team,id;
    if(!nflstreet_team_table_number(&play,row,team_field,&team)||!nflstreet_team_table_number(&play,row,id_field,&id))goto done;
    if(team==s->team_ids[side]&&id==s->selected_players[side][p]){if(source[side][p]!=UINT32_MAX)goto done;source[side][p]=row;}}
   if(source[side][p]==UINT32_MAX)goto done;
  }
  for(unsigned half=0;half<2;++half){unsigned mask=0;
   for(unsigned p=0;p<7;++p){unsigned selected=0;while(selected<7&&s->selected_players[side][selected]!=s->formation_players[side][half*7+p])++selected;
    if(selected==7||(mask&(1u<<selected)))goto done;mask|=1u<<selected;}}
 }
 for(unsigned row=0;row<play.used;++row){uint32_t team,id,flags;
  if(!nflstreet_team_table_number(&play,row,team_field,&team)||!nflstreet_team_table_number(&play,row,id_field,&id)||!nflstreet_team_table_number(&play,row,flag_field,&flags))goto done;
  int touched=0;for(unsigned side=0;side<2;++side)if(team==s->team_ids[side]){int chosen=0;
   for(unsigned p=0;p<7;++p)chosen|=source[side][p]==row;flags=nflstreet_lineup_flag(flags,side,chosen);touched=1;}
  if(touched)for(unsigned prior=0;prior<count;++prior)if(changes[prior].digp==id)goto done;
  if(touched)changes[count++]=(nflstreet_stat_override){id,GILP,flags};
 }
 if(!nflstreet_game_roster_build(&play,&thcd,&teams,schema,s->team_ids,2,changes,count,&r.game))goto done;
 unsigned game_team,game_id;if(!field(&r.game.table,DIGT,&game_team)||!field(&r.game.table,DIGP,&game_id))goto done;
 for(unsigned side=0;side<2;++side)for(unsigned p=0;p<7;++p){unsigned found=0;
  for(unsigned row=0;row<r.game.table.used;++row){uint32_t team,id;
   if(!nflstreet_team_table_number(&r.game.table,row,game_team,&team)||!nflstreet_team_table_number(&r.game.table,row,game_id,&id))goto done;
   if(team==side&&r.game.source_rows[row]==source[side][p]){if(found++)goto done;r.selected_rows[side][p]=row;r.selected_game_ids[side][p]=id;}}
  if(found!=1)goto done;
 }
 for(unsigned side=0;side<2;++side)for(unsigned p=0;p<14;++p){unsigned selected=0;while(selected<7&&s->selected_players[side][selected]!=s->formation_players[side][p])++selected;
  r.formation_rows[side][p]=r.selected_rows[side][selected];r.formation_game_ids[side][p]=r.selected_game_ids[side][selected];}
 r.selection=*s;*out=r;ok=1;
done:free(changes);if(!ok)nflstreet_quick_match_roster_close(&r);return ok;
}
