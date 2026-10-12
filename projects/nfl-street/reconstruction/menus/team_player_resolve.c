#include "team_player_resolve.h"
#include <string.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
/* Validate the unique supported hash chunk before interpreting lookup failure
   as a miss. Malformed/unsupported archives never trigger a fallback. */
static int hashes(const nflstreet_archive*a){
 if(!a||a->size<16)return 0;size_t at=word(a->bytes+4);unsigned found=0;
 while(at<a->size){if(a->size-at<8)return 0;uint32_t size=word(a->bytes+at+4);
  if(size<8||size>a->size-at)return 0;
  if(!memcmp(a->bytes+at,"HSH1",4)){
   if(++found!=1||size<24||(size_t)a->count*4>size-24||
      word(a->bytes+at+8)!=1||word(a->bytes+at+12)!=4||word(a->bytes+at+16)!=4)return 0;
  }at+=size;
 }return found==1;
}
int nflstreet_team_player_resolver_open(const uint8_t*f,size_t fn,const uint8_t*h,size_t hn,nflstreet_team_player_resolver*out){
 if(!out)return 0;nflstreet_team_player_resolver next;
 if(!nflstreet_archive_open(f,fn,&next.face)||!nflstreet_archive_open(h,hn,&next.hair)||!hashes(&next.face)||!hashes(&next.hair))return 0;
 *out=next;return 1;
}
static int lookup(void*ctx,unsigned pool,const char*key,int32_t*out){
 const nflstreet_team_player_resolver*r=ctx;if(!r||!key||!out)return 0;
 const nflstreet_archive*a=pool==NFLSTREET_TEAM_PLAYER_POOL_FACE?&r->face:pool==NFLSTREET_TEAM_PLAYER_POOL_ROSTER_HAIR?&r->hair:NULL;
 if(!a)return 0;unsigned entry;*out=nflstreet_archive_find(a,key,&entry)?(int32_t)entry:-1;return 1;
}
int nflstreet_team_player_resolve(const nflstreet_team_player_resolver*r,const uint8_t*b,size_t n,uint32_t team,uint32_t player,nflstreet_team_player_manifest*out){
 if(!r||!out)return 0;nflstreet_team_player_manifest next;
 if(!nflstreet_team_player_manifest_from_db(b,n,team,player,lookup,(void*)r,&next)||
    (next.custom_name_branch&&(next.head_runtime_index<0||(uint32_t)next.head_runtime_index>=r->face.count)))return 0;
 *out=next;return 1;
}
