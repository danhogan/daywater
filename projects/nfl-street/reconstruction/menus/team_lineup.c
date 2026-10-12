#include "team_lineup.h"
#include <string.h>

int nflstreet_team_lineup_rank(const nflstreet_team_lineup_player *p,
 uint32_t count,uint32_t role,nflstreet_team_lineup_ranked out[4]){
 if(!p||!out||count>NFLSTREET_TEAM_LINEUP_MAX_PLAYERS||role>=NFLSTREET_TEAM_LINEUP_ROLE_COUNT)return 0;
 nflstreet_team_lineup_ranked top[4];
 for(unsigned i=0;i<4;++i){top[i].player_id=NFLSTREET_TEAM_LINEUP_NONE;top[i].score=0;}
 for(uint32_t i=0;i<count;++i){
  if(p[i].flags!=0)continue;
  uint32_t score=p[i].scores[role];
  if(top[0].player_id==NFLSTREET_TEAM_LINEUP_NONE||top[0].score<score||
     (top[0].score==score&&role==p[i].role)){
   for(int j=3;j>0;--j)top[j]=top[j-1];
   top[0].player_id=p[i].player_id;top[0].score=score;
  }else{
   for(int slot=3;slot>0;--slot){
    if(top[slot].player_id!=NFLSTREET_TEAM_LINEUP_NONE){
     if(score<=top[slot].score&&!(score==top[slot].score&&role==p[i].role))break;
     if(slot<3)top[slot+1]=top[slot];
     top[slot].player_id=p[i].player_id;top[slot].score=score;
    }
   }
  }
 }
 memcpy(out,top,sizeof top);return 1;
}

int nflstreet_team_lineup_choose_role(const nflstreet_team_lineup_player *p,
 uint32_t count,const uint32_t *eligible,uint32_t eligible_count,
 const uint32_t *excluded,uint32_t excluded_count,
 nflstreet_team_lineup_choice *out){
 if(!p||!out||count>NFLSTREET_TEAM_LINEUP_MAX_PLAYERS||
    eligible_count>NFLSTREET_TEAM_LINEUP_MAX_PLAYERS||excluded_count>NFLSTREET_TEAM_LINEUP_ROLE_COUNT||
    (eligible_count&&!eligible)||(excluded_count&&!excluded))return 0;
 nflstreet_team_lineup_player filtered[20];memcpy(filtered,p,count*sizeof *p);
 if(eligible_count)for(uint32_t i=0;i<count;++i){int found=0;for(uint32_t j=0;j<eligible_count;++j)if(eligible[j]==p[i].player_id){found=1;break;}if(!found)filtered[i].flags=1;}
 nflstreet_team_lineup_choice result={NFLSTREET_TEAM_LINEUP_NONE,0xffu,0};
 for(uint32_t role=0;role<NFLSTREET_TEAM_LINEUP_ROLE_COUNT;++role){
  int skip=0;for(uint32_t j=0;j<excluded_count;++j)if(excluded[j]==role){skip=1;break;}if(skip)continue;
  nflstreet_team_lineup_ranked ranked[4];
  if(!nflstreet_team_lineup_rank(filtered,count,role,ranked))return 0;
  if(ranked[0].player_id==NFLSTREET_TEAM_LINEUP_NONE)continue;
  /* 80042850 uses <=, so an equal score replaces the prior role result. */
  if(result.player_id==NFLSTREET_TEAM_LINEUP_NONE||result.score<=ranked[0].score){
   result.player_id=ranked[0].player_id;result.role=role;result.score=ranked[0].score;
  }
 }
 *out=result;return 1;
}

int nflstreet_team_lineup_select_roster(nflstreet_team_lineup_roster *roster,
 const nflstreet_team_lineup_player *weighted,uint32_t weighted_count){
 if(!roster||!weighted||roster->count<7||roster->count>NFLSTREET_TEAM_LINEUP_MAX_PLAYERS||
    weighted_count>NFLSTREET_TEAM_LINEUP_MAX_PLAYERS)return 0;
 nflstreet_team_lineup_roster next=*roster;uint32_t first=-1;
 for(uint32_t i=0;i<next.count;++i)if((int32_t)next.rows[i].slot<0){first=i;break;}
 for(uint32_t iteration=0;iteration<7;++iteration){
  uint32_t assigned=0,eligible_ids[20],eligible_count=0,excluded_roles[14],excluded_count=0;
  nflstreet_team_lineup_player candidates[20];memcpy(candidates,weighted,weighted_count*sizeof *weighted);
  for(uint32_t i=0;i<next.count;++i){
   nflstreet_team_lineup_triple *row=&next.rows[i];
   if((int32_t)row->slot>=0){
    ++assigned;
    if(row->aux!=0xffu){int seen=0;for(uint32_t j=0;j<excluded_count;++j)if(excluded_roles[j]==row->aux){seen=1;break;}if(!seen&&excluded_count<14)excluded_roles[excluded_count++]=row->aux;
     for(uint32_t j=0;j<weighted_count;++j)if(candidates[j].player_id==row->player_id)candidates[j].flags=1;
    }else if(eligible_count<20)eligible_ids[eligible_count++]=row->player_id;
   }else if(eligible_count<20)eligible_ids[eligible_count++]=row->player_id;
  }
  uint32_t selected_id=0x7fffu,selected_role=0xffu;
  if(assigned<7){
   nflstreet_team_lineup_choice choice;
   if(!nflstreet_team_lineup_choose_role(candidates,weighted_count,eligible_ids,eligible_count,excluded_roles,excluded_count,&choice))return 0;
   selected_id=choice.player_id;selected_role=choice.role;
  }else{
   for(uint32_t i=0;i<next.count;++i)if((int32_t)next.rows[i].slot>=0&&next.rows[i].aux==0xffu){selected_id=next.rows[i].player_id;selected_role=0;break;}
  }
  uint32_t selected_index=next.count;
  for(uint32_t i=0;i<next.count;++i)if(next.rows[i].player_id==selected_id){selected_index=i;break;}
  if(selected_index==next.count)return 0;
  if((int32_t)next.rows[selected_index].slot<0){
   if(first>=next.count||first>=7)return 0;
   if(first!=selected_index){nflstreet_team_lineup_triple tmp=next.rows[first];next.rows[first]=next.rows[selected_index];next.rows[selected_index]=tmp;}
   next.rows[first].slot=first;
   ++first;while(first<7&&first<next.count&&(int32_t)next.rows[first].slot>=0)++first;
  }
  /* Resolve by ID because the swap may have moved the selected row. */
  for(uint32_t i=0;i<next.count;++i)if(next.rows[i].player_id==selected_id){next.rows[i].aux=selected_role;break;}
 }
 for(uint32_t i=0;i<7;++i)next.selected_ids[i]=next.rows[i].player_id;
 next.last_seventh_player_id=next.rows[6].player_id;
 for(uint32_t i=0;i<next.count;++i)next.rows[i].aux=0xffu;
 *roster=next;return 1;
}
