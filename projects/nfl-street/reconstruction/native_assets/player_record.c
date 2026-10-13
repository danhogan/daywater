#include "player_record.h"
#include <string.h>
#define DEFAULT_SELECTORS 0x2ccu /* 802717BC within the GTC4 window */
#define TAG_ITGT 0x49544754u
#define TAG_DIGT 0x44494754u
#define TAG_DIGP 0x44494750u
static int field_index(const nflstreet_team_table *t,uint32_t tag,unsigned *index,unsigned *found){
 *found=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(*found)return 0;*index=i;*found=1;}}
 return 1;
}
/* 80149614-style getter: a missing tag stops 801BFFDC with the item still 0. */
static int number(const nflstreet_team_table *t,unsigned row,uint32_t tag,uint32_t *value){
 unsigned index=0,found;if(!field_index(t,tag,&index,&found))return 0;
 if(!found){*value=0;return 1;}return nflstreet_team_table_number(t,row,index,value);
}
/* 8014947C: first row whose key equals id; 0 when there is none. */
static int find_row(const nflstreet_team_table *t,uint32_t key,uint32_t id,unsigned *row,int *found){
 unsigned index=0,present;if(!field_index(t,key,&index,&present)||!present)return 0;*found=0;
 for(unsigned r=0;r<t->used;++r){uint32_t v;if(!nflstreet_team_table_number(t,r,index,&v))return 0;if(v==id){*row=r;*found=1;return 1;}}
 return 1;
}
unsigned nflstreet_player_side(unsigned player,uint8_t r13_6f68,uint32_t word_8030b1fc){
 return r13_6f68?1u:word_8030b1fc?player>6u:player<7u;
}
int nflstreet_player_record_build(const nflstreet_team_table *play,unsigned row,const nflstreet_gear_records *g,
 const nflstreet_team_table *teams,const nflstreet_team_table *colors,const nflstreet_gear_texture_catalog *c,
 unsigned side,uint8_t out[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]){
 static const uint32_t team_colors[3]={0x31434d54u,0x32434d54u,0x33434d54u};
 if(!play||!g||!teams||!colors||!c||!out||row>=play->used||side>255)return 0;
 uint8_t r[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];memcpy(r,out,sizeof r);uint32_t team,player,v,type;unsigned at;int found;
 if(!number(play,row,TAG_ITGT,&team)||!find_row(teams,TAG_DIGT,team,&at,&found)||!found)return 0;
 for(unsigned i=0;i<3;++i){if(!number(teams,at,team_colors[i],&v))return 0;r[0x5f+i]=(uint8_t)v;}
 if(!number(teams,at,0x4c474c54u,&v)||!number(teams,at,0x50595454u,&type))return 0;r[0x3e]=(uint8_t)(v>>8);r[0x3f]=(uint8_t)v;
 if(type==5){if(!number(teams,at,0x4c445443u,&v))return 0;r[0x3c]=v!=0;}else r[0x3c]=1;
 if(!nflstreet_gear_player_record_from_play(play,row,g,r))return 0;
 r[0x64]=(uint8_t)side;if(!number(play,row,0x31414850u,&v))return 0;r[0x5e]=(uint8_t)(v+0x80u);
 if(!number(play,row,TAG_DIGP,&player)||!find_row(colors,TAG_DIGP,player,&at,&found)||found)return 0;
 uint8_t *s=r+0x4c;const uint8_t *selectors=c->window+DEFAULT_SELECTORS;
 for(unsigned i=0;i<18;++i){uint8_t b=selectors[i];
  if((uint8_t)(b-0x13)<3)s[i]=s[b];else if(b==0xf0)s[i]=0x75;else if(b==0xf1)s[i]=0x7f;}
 s[0x16]=0x75;s[0x17]=s[0x14];
 if(!side){s[6]=s[5];s[0x16]=s[0x13];s[5]=s[4];s[4]=0x75;}
 memcpy(out,r,sizeof r);return 1;
}
