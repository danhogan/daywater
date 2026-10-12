#include "team_preview.h"
#include <stdint.h>
#include <string.h>

int nflstreet_team_preview_body_entries(uint32_t raw_tbyp,uint32_t entries[3]){
 if(!entries)return 0;
 uint32_t key=raw_tbyp==7?2:raw_tbyp;
 if(key>2)return 0;
 for(unsigned i=0;i<3;++i)entries[i]=key*3+i+1;
 return 1;
}

#define DB_TABLE_PLAY 0x59414c50u
#define DB_TABLE_TEAM 0x4d414554u
#define DB_FIELD_DIGT 0x44494754u
#define DB_FIELD_DIGP 0x44494750u

static uint16_t be16(const uint8_t*p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}

static int range(size_t n,size_t at,size_t z){return at<=n&&z<=n-at;}

static int extract_table(const uint8_t*b,size_t n,uint32_t wanted,
                         const uint8_t**table,size_t*table_size){
 if(!b||n<28||memcmp(b,"DB\0\x08",4)||be32(b+8)!=n)return 0;
 uint32_t count=be32(b+16);if(!count||count>64||!range(n,28,(size_t)count*8))return 0;
 size_t base=28+(size_t)count*8,found=0,at=0;
 for(uint32_t i=0;i<count;++i){
  uint32_t tag=be32(b+24+i*8),offset=be32(b+28+i*8);
  if(tag!=wanted)continue;
  if(offset>n-base||++found!=1)return 0;
  at=base+offset;
 }
 if(found!=1||!range(n,at,36))return 0;
 uint32_t record_bytes=be32(b+at+4),record_bits=be32(b+at+8);
 uint16_t allocated=be16(b+at+16),used=be16(b+at+18);uint8_t fields=b[at+24];
 if(!record_bytes||record_bytes>4096||!record_bits||record_bits>(uint64_t)record_bytes*8||
    used>allocated||!fields||!range(n,at+36,(size_t)fields*16))return 0;
 size_t rows=at+36+(size_t)fields*16;
 if(!range(n,rows,(size_t)allocated*record_bytes))return 0;
 *table=b+at;*table_size=n-at;return 1;
}

static int field(const uint8_t*t,size_t tn,uint32_t wanted,uint32_t*bit,uint32_t*width){
 if(tn<36)return 0;uint32_t count=t[24];
 if(!range(tn,36,(size_t)count*16))return 0;unsigned found=0;
 for(uint32_t i=0;i<count;++i){const uint8_t*p=t+36+i*16;
  if(be32(p+8)!=wanted)continue;
  if(be32(p)!=3||++found!=1)return 0;
  *bit=be32(p+4);*width=be32(p+12);
 }
 return found==1&&*width&&*width<=32;
}

static int read_bits(const uint8_t*row,size_t bytes,uint32_t bit,uint32_t width,uint32_t*out){
 if(!out||!width||width>32||bit>(uint64_t)bytes*8||width>(uint64_t)bytes*8-bit)return 0;
 uint32_t value=0;for(uint32_t i=0;i<width;++i){uint32_t at=bit+i;
  value=(value<<1)|((row[at>>3]>>(7u-(at&7u)))&1u);
 }
 *out=value;return 1;
}

int nflstreet_team_preview_roster_from_db(const uint8_t*b,size_t n,uint32_t team_id,
 uint32_t ids[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS],uint32_t*out_count){
 if(!b||!ids||!out_count)return 0;
 const uint8_t*team=NULL,*play=NULL;size_t team_size=0,play_size=0;
 if(!extract_table(b,n,DB_TABLE_TEAM,&team,&team_size)||
    !extract_table(b,n,DB_TABLE_PLAY,&play,&play_size))return 0;
 uint32_t team_key_bit=0,team_key_width=0,play_team_bit=0,play_team_width=0,player_bit=0,player_width=0;
 if(!field(team,team_size,DB_FIELD_DIGT,&team_key_bit,&team_key_width)||
    !field(play,play_size,DB_FIELD_DIGT,&play_team_bit,&play_team_width)||
    !field(play,play_size,DB_FIELD_DIGP,&player_bit,&player_width))return 0;
 uint32_t record_bytes=be32(team+4);uint16_t team_used=be16(team+18),play_used=be16(play+18);
 uint32_t team_fields=team[24],play_fields=play[24];
 size_t team_rows=36+(size_t)team_fields*16,play_rows=36+(size_t)play_fields*16;
 if(!team_used||team_used>1||record_bytes>team_size||
    !range(team_size,team_rows,record_bytes))return 0;
 uint32_t found_id=0;if(!read_bits(team+team_rows,record_bytes,team_key_bit,team_key_width,&found_id)||found_id!=team_id)return 0;
 uint32_t play_bytes=be32(play+4);if(!play_bytes||play_bytes>4096||play_used>be16(play+16)||
    !range(play_size,play_rows,(size_t)play_used*play_bytes))return 0;
 uint32_t result[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS];unsigned count=0;
 for(uint32_t i=0;i<play_used;++i){const uint8_t*row=play+play_rows+(size_t)i*play_bytes;uint32_t row_team=0,player=0;
  if(!read_bits(row,play_bytes,play_team_bit,play_team_width,&row_team))return 0;
  if(row_team!=team_id)continue;
  if(count>=NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS||
     !read_bits(row,play_bytes,player_bit,player_width,&player))return 0;
  result[count++]=player;
 }
 if(!count)return 0;memcpy(ids,result,count*sizeof result[0]);*out_count=count;return 1;
}

int nflstreet_team_preview_team_id_from_db(const uint8_t*b,size_t n,uint32_t*out){
 if(!b||!out)return 0;const uint8_t*team=NULL;size_t team_size=0;
 if(!extract_table(b,n,DB_TABLE_TEAM,&team,&team_size))return 0;
 uint32_t bit=0,width=0;if(!field(team,team_size,DB_FIELD_DIGT,&bit,&width))return 0;
 uint16_t used=be16(team+18);uint32_t fields=team[24],record_bytes=be32(team+4);
 size_t rows=36+(size_t)fields*16;if(used!=1||!range(team_size,rows,record_bytes))return 0;
 return read_bits(team+rows,record_bytes,bit,width,out);
}

int nflstreet_team_preview_roster_select(nflstreet_team_preview_roster*s,
 uint32_t side,uint32_t team_id,const uint32_t*ids,uint32_t count){
 if(!s||side>1||count>NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS||(count&&!ids))return 0;
 nflstreet_team_preview_roster next={0};next.team_id=team_id;next.count=count;
 if(count)memcpy(next.player_ids,ids,count*sizeof ids[0]);
 next.cursor=count?count-1:UINT32_MAX;next.cursor_mode=1;
 for(uint32_t i=0;i<NFLSTREET_TEAM_PREVIEW_VISIBLE_ROWS;++i)next.cleared_slots[i]=side*7+i;
 next.clear_count=NFLSTREET_TEAM_PREVIEW_VISIBLE_ROWS;*s=next;return 1;
}

int nflstreet_team_preview_assets_from_db(const uint8_t*b,size_t n,uint32_t team_id,
 uint32_t assets[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS],uint32_t*out_count){
 if(!b||!assets||!out_count)return 0;const uint8_t*t;size_t z;
 if(!extract_table(b,n,DB_TABLE_PLAY,&t,&z))return 0;
 const uint32_t tags[5]={DB_FIELD_DIGT,0x59545250u,0x50585350u,0x43465250u,0x54425950u};
 uint32_t bits[5],widths[5];for(unsigned j=0;j<5;++j)if(!field(t,z,tags[j],&bits[j],&widths[j]))return 0;
 uint32_t bytes=be32(t+4),used=be16(t+18);size_t at=36+(size_t)t[24]*16;uint32_t result[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS],count=0;
 for(unsigned i=0;i<used;++i){uint32_t v[5];const uint8_t*r=t+at+(size_t)i*bytes;for(unsigned j=0;j<5;++j)if(!read_bits(r,bytes,bits[j],widths[j],&v[j]))return 0;
  if(v[0]!=team_id)continue;if(count>=NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS)return 0;
  if(v[1]){if(v[3]>(UINT32_MAX-0x1afu)/3)return 0;result[count]=v[3]*3+0x1ad+(v[4]==1?1:v[4]==2?2:0);}else result[count]=v[2];++count;
 }
 if(!count)return 0;memcpy(assets,result,count*sizeof *result);*out_count=count;return 1;
}
