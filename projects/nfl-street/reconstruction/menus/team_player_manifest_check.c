#include "team_player_manifest.h"
#include <stdio.h>
#include <string.h>

static void put16(uint8_t*p,uint16_t x){p[0]=(uint8_t)(x>>8);p[1]=(uint8_t)x;}
static void put32(uint8_t*p,uint32_t x){p[0]=(uint8_t)(x>>24);p[1]=(uint8_t)(x>>16);p[2]=(uint8_t)(x>>8);p[3]=(uint8_t)x;}

static const uint32_t tags[14]={
 0x44494754u,0x44494750u,0x494b5350u,0x43465250u,0x54425950u,
 0x54474850u,0x59545250u,0x50585350u,0x52414850u,0x54484c50u,
 0x4e454a50u,0x58454c50u,0x414e4c50u,0x414e4650u
};
static int make_db(uint8_t *b,size_t cap,size_t *size,uint32_t tbyp,
 uint32_t ytrp,uint32_t pxsp,uint32_t cfrp,unsigned rows){
 const unsigned fields=14,record_bytes=80;
 const size_t base=36,table_size=36u+fields*16u+(size_t)record_bytes*rows,total=base+table_size;
 if(cap<total)return 0;
 memset(b,0,total);
 memcpy(b,"DB\0\x08",4);put32(b+8,(uint32_t)total);put32(b+16,1);
 put32(b+24,0x59414c50u);put32(b+28,0);
 uint8_t*t=b+base;put32(t+4,record_bytes);put32(t+8,record_bytes*8);
 put16(t+16,(uint16_t)rows);put16(t+18,(uint16_t)rows);t[24]=(uint8_t)fields;
 unsigned bit=0;
 for(unsigned i=0;i<fields;++i){uint8_t*d=t+36+i*16;
  put32(d,i<12?3:0);put32(d+4,bit);put32(d+8,tags[i]);
  uint32_t width=i<12?32u:(i==12?112u:96u);put32(d+12,width);bit+=width;
 }
 for(unsigned row=0;row<rows;++row){uint8_t*r=t+36+fields*16+(size_t)row*record_bytes;
  const uint32_t values[12]={1,355,4,cfrp,tbyp,73,ytrp,pxsp,22,7,10,4};
  for(unsigned i=0;i<12;++i)put32(r+i*4,values[i]);
  memcpy(r+48,"Stewart",7);memcpy(r+62,"Kordell",7);
 }
 put32(b+4,0); /* reserved */
 *size=total;return 1;
}

typedef struct {unsigned calls;int fail;int head_miss;} lookup_ctx;
static int checked_lookup(void *opaque,unsigned pool,const char*key,int32_t*out){
 lookup_ctx*c=(lookup_ctx*)opaque;if(!c||!key||!out||c->fail)return 0;c->calls++;
 if(pool==NFLSTREET_TEAM_PLAYER_POOL_FACE&&strcmp(key,"HEAD_MODEL_STEWARTKORDELL_2700")==0){*out=c->head_miss?-1:12;return 1;}
 if(pool==NFLSTREET_TEAM_PLAYER_POOL_ROSTER_HAIR&&strcmp(key,"HAIR_MODEL_STEWARTKORDELL_2700")==0){*out=21;return 1;}
 if(pool==NFLSTREET_TEAM_PLAYER_POOL_ROSTER_HAIR&&strcmp(key,"HAIR_MODEL_STEWARTKORDELLHAT_2700")==0){*out=23;return 1;}
 return 0;
}

int main(void){
 uint8_t bytes[1024];size_t size=0;nflstreet_team_player_manifest m;
 memset(&m,0xa5,sizeof m);if(!make_db(bytes,sizeof bytes,&size,2,0,0,63,1))return 2;
 lookup_ctx c={0,0,0};
 if(!nflstreet_team_player_manifest_from_db(bytes,size,1,355,checked_lookup,&c,&m)||
    c.calls!=3||m.team_id!=1||m.player_id!=355||m.play_row!=0||
    m.iksp!=4||m.cfrp!=63||m.tbyp!=2||m.phar!=22||m.plht!=7||
    m.pjen!=10||m.plex!=4||m.phgt!=73||m.portrait_selector!=0||
    !m.body_key_supported||m.body_key!=2||!m.custom_name_branch||
    m.hair_variant_resolved||strcmp(m.normalized_name,"STEWARTKORDELL")||
    m.head_candidate_index!=12||m.head_runtime_index!=12||
    m.hair_candidate_index!=21||m.hair_hat_candidate_index!=23)return 3;

 nflstreet_team_player_manifest before=m;c.head_miss=1;c.calls=0;
 if(!nflstreet_team_player_manifest_from_db(bytes,size,1,355,checked_lookup,&c,&m)||
    c.calls!=3||m.head_candidate_index!=-1||m.head_runtime_index!=0x28a||
    m.hair_candidate_index!=21||m.hair_hat_candidate_index!=23)return 4;

 /* Non-custom YTRP uses CFRP/TBYP portrait selection, skips name lookups,
    and still records TBYP's verified body-key behavior. */
 for(uint32_t tbyp=0;tbyp<=2;++tbyp){
  if(!make_db(bytes,sizeof bytes,&size,tbyp,1,9,17,1))return 5;
  memset(&m,0,sizeof m);c.calls=0;c.fail=1;
  if(!nflstreet_team_player_manifest_from_db(bytes,size,1,355,checked_lookup,&c,&m)||
     c.calls||m.custom_name_branch||m.head_key[0]||m.normalized_name[0]||
     m.portrait_selector!=17u*3u+0x1adu+tbyp||
     m.body_key!=tbyp||!m.body_key_supported||m.hair_variant_resolved)return 6;
 }
 if(!make_db(bytes,sizeof bytes,&size,7,0,0,63,1))return 7;
 c.fail=0;c.head_miss=0;c.calls=0;
 if(!nflstreet_team_player_manifest_from_db(bytes,size,1,355,checked_lookup,&c,&m)||
    !m.body_key_supported||m.body_key!=2||m.tbyp!=7)return 8;
 if(!make_db(bytes,sizeof bytes,&size,3,0,0,63,1)||
    !nflstreet_team_player_manifest_from_db(bytes,size,1,355,checked_lookup,&c,&m)||
    m.body_key_supported)return 9;

 /* Duplicate and absent row failures preserve the caller's output. */
 before=m;
 if(!make_db(bytes,sizeof bytes,&size,2,0,0,63,2)||
    nflstreet_team_player_manifest_from_db(bytes,size,1,355,checked_lookup,&c,&m)||
    memcmp(&m,&before,sizeof m))return 10;
 if(!make_db(bytes,sizeof bytes,&size,2,0,0,63,1))return 11;
 if(nflstreet_team_player_manifest_from_db(bytes,size,1,999,checked_lookup,&c,&m)||
    memcmp(&m,&before,sizeof m))return 12;

 /* The portrait formula must reject overflow after the TBYP adjustment. */
 if(!make_db(bytes,sizeof bytes,&size,2,1,0,0x55555555u,1))return 13;
 if(nflstreet_team_player_manifest_from_db(bytes,size,1,355,checked_lookup,&c,&m)||
    memcmp(&m,&before,sizeof m))return 14;

 if(!make_db(bytes,sizeof bytes,&size,2,0,0,63,1))return 15;
 before=m;c.fail=1;
 if(nflstreet_team_player_manifest_from_db(bytes,size,1,355,checked_lookup,&c,&m)||
    memcmp(&m,&before,sizeof m))return 16;
 puts("Selected PLAY manifest synthetic checks passed: exact row, raw fields, lookup hit/miss fallback, YTRP portrait branches, TBYP keys, duplicate/missing rows, overflow, atomic failure.");
 return 0;
}
