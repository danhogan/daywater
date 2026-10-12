#include "team_player_remove.h"
#include <string.h>
#include <stdint.h>

static int overlap(uintptr_t a,size_t an,uintptr_t b,size_t bn){return a<=b?b-a<an:a-b<bn;}
static int pose_valid(const nflstreet_team_activation_pose*p){float f;
 for(unsigned i=0;i<4;++i){uint32_t w=i<3?p->position[i]:p->rotation;memcpy(&f,&w,4);if(f!=f||f>1000000.0f||f< -1000000.0f)return 0;}
 return p->visibility<=255u;
}
static void append(nflstreet_team_activation*a,uint32_t kind,uint32_t index,const uint32_t words[5]){
 nflstreet_team_activation_effect*e=&a->effects[a->effect_count++];memset(e,0,sizeof *e);e->kind=kind;e->index=index;if(words)memcpy(e->words,words,20);
}

int nflstreet_team_player_remove(nflstreet_team_activation*a,
 nflstreet_team_player_window*w,unsigned side,
 const nflstreet_team_activation_pose*pose,uint32_t*outslot){
 if(!a||!w||!pose||!outslot||side>=NFLSTREET_TEAM_ACTIVATION_SIDES||!pose_valid(pose)||
    a->effect_count>NFLSTREET_TEAM_ACTIVATION_EFFECTS||NFLSTREET_TEAM_ACTIVATION_POSES*3u+2u>NFLSTREET_TEAM_ACTIVATION_EFFECTS-a->effect_count||
    overlap((uintptr_t)a,sizeof *a,(uintptr_t)w,sizeof *w)||
    overlap((uintptr_t)a,sizeof *a,(uintptr_t)pose,sizeof *pose)||
    overlap((uintptr_t)w,sizeof *w,(uintptr_t)pose,sizeof *pose)||
    overlap((uintptr_t)a,sizeof *a,(uintptr_t)outslot,sizeof *outslot)||
    overlap((uintptr_t)w,sizeof *w,(uintptr_t)outslot,sizeof *outslot)||
    overlap((uintptr_t)pose,sizeof *pose,(uintptr_t)outslot,sizeof *outslot))return 0;
 nflstreet_team_activation nexta=*a;nflstreet_team_player_window nextw=*w;
 nflstreet_team_activation_side*x=&nexta.sides[side];nflstreet_team_player_window_side*y=&nextw.sides[side];
 if(x->count>NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX||y->count!=x->count||y->query_count>NFLSTREET_TEAM_PLAYER_WINDOW_ROSTER_MAX||
    y->query_count!=y->count||y->mode<0||y->mode>2||y->cursor< -1||
    (y->cursor>=0&&(uint32_t)y->cursor>=y->query_count)||y->visibility_count>1)return 0;
 for(unsigned i=0;i<x->count;++i)if(x->roster[i].id!=y->rows[i].id||x->roster[i].selected!=y->rows[i].slot||x->roster[i].auxiliary!=y->rows[i].auxiliary)return 0;
 uint32_t last=x->last_seventh_player_id,ordinal=0;
 for(unsigned i=0;i<y->query_count;++i)if(y->query_ids[i]==last){if(ordinal)return 0;ordinal=i+1u;}
 int32_t cursor=ordinal?(int32_t)ordinal-1:-1;
 uint32_t removed=x->count;
 for(unsigned i=0;i<x->count;++i)if(x->roster[i].id==last){removed=i;break;}
 uint32_t visible_index=removed+side*7u;
 if(visible_index>UINT32_MAX-1u)return 0;
 x->last_seventh_player_id=NFLSTREET_TEAM_LINEUP_NONE;
 if(removed<x->count){x->roster[removed].selected=-1;y->rows[removed].slot=-1;}
 x->dirty=1;y->cursor=cursor;y->mode=1;y->visibility_count=1;y->visibility_index=visible_index;
 uint32_t deselect[5]={last,removed,(uint32_t)cursor,1,ordinal};append(&nexta,NFLSTREET_TEAM_ACTIVATION_DESELECT,visible_index,deselect);
 uint32_t vis[5]={pose->position[0],pose->position[1],pose->position[2],pose->rotation,0};append(&nexta,NFLSTREET_TEAM_ACTIVATION_VISIBILITY,visible_index,vis);
 for(unsigned i=0;i<NFLSTREET_TEAM_ACTIVATION_POSES;++i){uint32_t index=side*7u+i;uint32_t words[5]={pose->position[0],pose->position[1],pose->position[2],pose->rotation,pose->visibility};
  append(&nexta,NFLSTREET_TEAM_ACTIVATION_POSITION,index,words);append(&nexta,NFLSTREET_TEAM_ACTIVATION_ROTATION,index,words);append(&nexta,NFLSTREET_TEAM_ACTIVATION_VISIBILITY,index,words);
 }
 *a=nexta;*w=nextw;*outslot=removed;return 1;
}
