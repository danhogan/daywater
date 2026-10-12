#include "team_confirm.h"
#include <string.h>

static int emit(nflstreet_team_confirm *s,uint32_t kind,
                uint32_t a,uint32_t b,uint32_t c,uint32_t d,uint32_t e){
 if(s->effect_count>=NFLSTREET_TEAM_CONFIRM_EFFECTS)return 0;
 s->effects[s->effect_count++]=(nflstreet_team_confirm_effect){kind,{a,b,c,d,e}};
 return 1;
}

int nflstreet_team_confirm_apply(nflstreet_team_confirm *state,uint32_t side_arg,
 uint32_t player_id,uint32_t *next_cursor,uint32_t *return_value,
 const uint32_t pose_a[2][7],const uint32_t pose_b[2][7]){
 if(!state||!next_cursor||!return_value)return 0;
 nflstreet_team_confirm next=*state;next.effect_count=0;
 uint32_t result=0;
 if(player_id!=UINT32_MAX){
  unsigned side=side_arg!=0;
  nflstreet_team_confirm_side *s=&next.sides[side];
  if(s->count>NFLSTREET_TEAM_CONFIRM_ROSTER_MAX)return 0;
  for(uint32_t i=0;i<s->count;++i){
   int32_t slot=s->rows[i].slot;
   if(slot>=0&&slot>=7)return 0;
  }
  uint32_t slot=0;
  if(s->count&&s->rows[0].slot>=0){
   while(slot<s->count&&s->rows[slot].slot>=0)++slot;
  }
  if(slot>=7)return 0;
  uint32_t visible=side*7+slot;
  if(visible>=NFLSTREET_TEAM_CONFIRM_VISIBLE_SLOTS)return 0;

  uint32_t found=s->count;
  for(uint32_t i=0;i<s->count;++i)if(s->rows[i].player_id==player_id){found=i;break;}
  if(found<s->count&&slot>=s->count)return 0;
  if(found==s->count&&found!=slot)return 0;

  next.visibility[visible]=0xff;
  if(!emit(&next,NFLSTREET_TEAM_CONFIRM_VISIBILITY_BYTE,visible,0xff,0,0,0))return 0;
  if(!s->source_flag_6e34){
   if(!pose_a||!pose_b)return 0;
   uint32_t a=pose_a[side][slot],b=pose_b[side][slot];
   if(a>=14||b>=14)return 0;
   next.preview_map[side*14+a]=player_id;
   next.preview_map[side*14+b]=player_id;
   if(!emit(&next,NFLSTREET_TEAM_CONFIRM_PREVIEW_INDEX_WRITE,
       player_id,side*14+a,side*14+b,a,b))return 0;
  }
  if(found<s->count){
   s->rows[found].slot=(int32_t)slot;
   if(found!=slot){
    nflstreet_team_confirm_row row=s->rows[found];
    s->rows[found]=s->rows[slot];s->rows[slot]=row;
    if(!emit(&next,NFLSTREET_TEAM_CONFIRM_ROSTER_SWAP,found,slot,0,0,0))return 0;
   }
  }
  uint32_t cursor=slot+1;
  while(cursor<s->count&&s->rows[cursor].slot>=0)++cursor;
  *next_cursor=cursor;

  uint32_t selected=0;
  for(uint32_t i=0;i<s->count;++i)if(s->rows[i].slot>=0)++selected;
  if(selected>6){s->last_confirmed_id=player_id;
   if(!emit(&next,NFLSTREET_TEAM_CONFIRM_LAST_ID_WRITE,side,player_id,0,0,0))return 0;
  }
  s->source_flag_6e30=0;
  if(!emit(&next,NFLSTREET_TEAM_CONFIRM_FLAG_CLEAR,side,0x6e30,0,0,0))return 0;
  result=1;
 }
 *state=next;*return_value=result;return 1;
}
