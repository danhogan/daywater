#include "team_activation.h"
#include "team_formation.h"
#include <string.h>

static int valid_pose(const nflstreet_team_activation_pose*p){
 float value;for(unsigned i=0;i<4;++i){uint32_t word=i<3?p->position[i]:p->rotation;memcpy(&value,&word,4);if(value!=value||value>1000000.0f||value< -1000000.0f)return 0;}
 return p->visibility<=255u;
}
static void push(nflstreet_team_activation*s,uint32_t kind,uint32_t index,
                 const uint32_t*w){if(s->effect_count>=NFLSTREET_TEAM_ACTIVATION_EFFECTS)return;
 nflstreet_team_activation_effect*e=&s->effects[s->effect_count++];memset(e,0,sizeof *e);e->kind=kind;e->index=index;if(w)memcpy(e->words,w,5*4);}

static int valid_weighted_rows(const nflstreet_team_activation_side*x){
 if(x->weighted_count!=x->count||x->weighted_count<7||x->weighted_count>NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX)return 0;
 for(unsigned i=0;i<x->weighted_count;++i){const nflstreet_team_lineup_player*p=&x->weighted_rows[i];
  if(p->player_id==NFLSTREET_TEAM_LINEUP_NONE||p->role>=NFLSTREET_TEAM_LINEUP_ROLE_COUNT||p->flags>1)return 0;
  unsigned hits=0;for(unsigned j=0;j<x->count;++j)if(x->roster[j].id==p->player_id)++hits;if(hits!=1)return 0;
  for(unsigned j=0;j<i;++j)if(x->weighted_rows[j].player_id==p->player_id)return 0;
 }
 for(unsigned i=0;i<x->count;++i){unsigned hits=0;for(unsigned j=0;j<x->weighted_count;++j)if(x->roster[i].id==x->weighted_rows[j].player_id)++hits;if(hits!=1)return 0;}
 return 1;
}

static int rebuild_roster(nflstreet_team_activation_side*x){
 nflstreet_team_lineup_roster r={0};r.count=x->count;
 for(unsigned i=0;i<x->count;++i){r.rows[i].player_id=x->roster[i].id;r.rows[i].slot=(uint32_t)x->roster[i].selected;r.rows[i].aux=x->roster[i].auxiliary;}
 if(!nflstreet_team_lineup_select_roster(&r,x->weighted_rows,x->weighted_count))return 0;
 for(unsigned i=0;i<x->count;++i){x->roster[i].id=r.rows[i].player_id;x->roster[i].selected=(int32_t)r.rows[i].slot;x->roster[i].auxiliary=r.rows[i].aux;}
 x->last_seventh_player_id=r.last_seventh_player_id;
 return 1;
}

static int assign_formation(nflstreet_team_activation_side*x,uint32_t*count){
 nflstreet_team_lineup_player rows[NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX];unsigned n=0;
 for(unsigned i=0;i<x->count;++i)if(x->roster[i].selected>=0){
  const nflstreet_team_lineup_player*found=NULL;for(unsigned j=0;j<x->weighted_count;++j)if(x->weighted_rows[j].player_id==x->roster[i].id){found=&x->weighted_rows[j];break;}
  if(!found||n>=NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX)return 0;rows[n++]=*found;x->builder_ids[*count]=found->player_id;++*count;
 }
 if(n<7)return 0;return nflstreet_team_formation_assign(rows,n,x->request_order,x->formation_words);
}

int nflstreet_team_activation_apply(nflstreet_team_activation*s,unsigned side,
                                    uint32_t*out,size_t capacity){
 if(!s||side>=NFLSTREET_TEAM_ACTIVATION_SIDES)return 0;
 nflstreet_team_activation next=*s;nflstreet_team_activation_side*x=&next.sides[side];
 if(x->count>NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX||x->dirty>1||x->rebuild>1||
    (out&&capacity<NFLSTREET_TEAM_ACTIVATION_POSES)||
    next.effect_count>NFLSTREET_TEAM_ACTIVATION_EFFECTS||
    NFLSTREET_TEAM_ACTIVATION_POSES*3u+(x->dirty?1u:0u)+(x->dirty&&x->rebuild?1u:0u)>NFLSTREET_TEAM_ACTIVATION_EFFECTS-next.effect_count)return 0;
 if(x->dirty&&x->weighted_count&&!valid_weighted_rows(x))return 0;
 if(x->dirty&&x->rebuild&&!x->weighted_count)return 0;
 if(x->dirty&&x->weighted_count)for(unsigned s=0;s<2;++s){unsigned mask=0;for(unsigned i=0;i<7;++i){unsigned v=x->request_order[s][i];if(v>=7||(mask&(1u<<v)))return 0;mask|=1u<<v;}}
 for(unsigned i=0;i<NFLSTREET_TEAM_ACTIVATION_POSES;++i)if(!valid_pose(&x->poses[i]))return 0;
 uint32_t selected_n=0;
 if(x->dirty&&x->rebuild){uint32_t w[5]={side,x->count,x->weighted_count,0,0};
  if(!rebuild_roster(x))return 0;x->rebuild=0;push(&next,NFLSTREET_TEAM_ACTIVATION_REBUILD,side,w);
 }
 if(x->dirty){
  if(x->weighted_count){if(!assign_formation(x,&selected_n))return 0;}
  else for(unsigned i=0;i<x->count;++i)if(x->roster[i].selected>=0)x->builder_ids[selected_n++]=x->roster[i].id;
  x->builder_count=selected_n;
  uint32_t builder[5]={side,x->formation_value,x->count,selected_n,x->weighted_count};
  push(&next,NFLSTREET_TEAM_ACTIVATION_BUILD,side,builder);
 }
 memcpy(x->active_formation,x->formation_words,sizeof x->active_formation);
 x->formation_count=7;
 x->selected_count=0;
 if(out){for(unsigned i=0;i<NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX&&x->selected_count<7;++i)if(x->roster[i].selected>=0){
   out[x->selected_count]=x->roster[i].id;x->selected_ids[x->selected_count++]=x->roster[i].id;
  }}
 else for(unsigned i=0;i<NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX&&x->selected_count<7;++i)if(x->roster[i].selected>=0)x->selected_ids[x->selected_count++]=x->roster[i].id;
 for(unsigned i=0;i<7;++i){uint32_t index=side*7u+i;uint32_t w[5]={x->poses[i].position[0],x->poses[i].position[1],x->poses[i].position[2],x->poses[i].rotation,x->poses[i].visibility};
  push(&next,NFLSTREET_TEAM_ACTIVATION_POSITION,index,w);push(&next,NFLSTREET_TEAM_ACTIVATION_ROTATION,index,w);push(&next,NFLSTREET_TEAM_ACTIVATION_VISIBILITY,index,w);
 }
 *s=next;return 1;
}
