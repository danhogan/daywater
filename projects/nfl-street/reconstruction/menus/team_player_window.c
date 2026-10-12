#include "team_player_window.h"
#include <string.h>
#include <stdint.h>

static int overlap(uintptr_t a,size_t an,uintptr_t b,size_t bn){return a<=b?b-a<an:a-b<bn;}

static int valid(const nflstreet_team_player_window_side *s){
 if(!s||s->count>NFLSTREET_TEAM_PLAYER_WINDOW_ROSTER_MAX||
    s->query_count>NFLSTREET_TEAM_PLAYER_WINDOW_ROSTER_MAX||
    s->query_count!=s->count||s->mode<0||s->mode>2||
    s->cursor< -1||(s->cursor>=0&&(uint32_t)s->cursor>=s->query_count)||
    s->visibility_count>1)return 0;
 return 1;
}
static int query_pair(const nflstreet_team_player_window_side*s,int32_t index,
                      nflstreet_team_player_window_pair*out){
 if(index<0){out->id=UINT32_MAX;out->slot=UINT32_MAX;return 1;}
 if((uint32_t)index>=s->query_count)return 0;
 uint32_t id=s->query_ids[index];out->id=id;out->slot=UINT32_MAX;
 for(uint32_t i=0;i<s->count;++i)if(s->rows[i].id==id&&s->rows[i].slot>=0){out->slot=(uint32_t)s->rows[i].slot;break;}
 return 1;
}
int nflstreet_team_player_window_callback_f(const nflstreet_team_player_window*s,unsigned side,
 nflstreet_team_player_window_pair out[3],uint32_t*mode_out){
 if(!s||side>=NFLSTREET_TEAM_PLAYER_WINDOW_SIDES||!out||!mode_out||
    overlap((uintptr_t)s,sizeof *s,(uintptr_t)out,3*sizeof *out)||
    overlap((uintptr_t)s,sizeof *s,(uintptr_t)mode_out,sizeof *mode_out)||
    overlap((uintptr_t)out,3*sizeof *out,(uintptr_t)mode_out,sizeof *mode_out)||!valid(&s->sides[side]))return 0;
 nflstreet_team_player_window_pair local[3];int32_t index[3];const nflstreet_team_player_window_side*x=&s->sides[side];
 index[0]=x->cursor;index[1]=(x->cursor==(int32_t)x->count-1)?-1:x->cursor+1;
 index[2]=(index[1]==(int32_t)x->count-1)?-1:index[1]+1;
 for(unsigned i=0;i<3;++i)if(!query_pair(x,index[i],&local[i]))return 0;
 memcpy(out,local,sizeof local);*mode_out=(uint32_t)x->mode;return 1;
}
int nflstreet_team_player_window_callback_10(nflstreet_team_player_window*s,unsigned side,
 int32_t delta,nflstreet_team_player_window_pair*out){
 if(!s||side>=NFLSTREET_TEAM_PLAYER_WINDOW_SIDES||!out||
    overlap((uintptr_t)s,sizeof *s,(uintptr_t)out,sizeof *out)||!valid(&s->sides[side])||(delta!=-1&&delta!=1))return 0;
 nflstreet_team_player_window next=*s;nflstreet_team_player_window_side*x=&next.sides[side];
 int64_t value=(int64_t)x->cursor+x->mode;
 if(value<(int64_t)x->query_count)value+=delta;
 else value=value-(int64_t)x->query_count+delta-1;
 if(value< -1)value=(int64_t)x->query_count-1;
 else if(value>=(int64_t)x->query_count)value=-1;
 int32_t index=(int32_t)value;
 if(x->mode==0&&delta<0)x->cursor=index;
 else if(x->mode==2&&delta>0){if(x->cursor==(int32_t)x->query_count-1)x->cursor=-1;else ++x->cursor;}
 else x->mode+=delta;
 nflstreet_team_player_window_pair result;if(!query_pair(x,index,&result))return 0;
 unsigned visible=0;while(visible<7&&x->rows[visible].slot>=0)++visible;
 x->visibility_count=0;if(visible<7){x->visibility_count=1;x->visibility_index=side*7u+visible;}
 x->timer_output=x->timer_word;*s=next;*out=result;return 1;
}
