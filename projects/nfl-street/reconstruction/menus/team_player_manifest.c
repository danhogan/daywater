#include "team_player_manifest.h"
#include "team_catalog.h"
#include "team_identity.h"
#include <stdio.h>
#include <string.h>

#define PLAY_TAG 0x59414c50u
#define TAG_DIGT 0x44494754u
#define TAG_DIGP 0x44494750u
#define TAG_IKSP 0x494b5350u
#define TAG_CFRP 0x43465250u
#define TAG_TBYP 0x54425950u
#define TAG_TGHP 0x54474850u
#define TAG_YTRP 0x59545250u
#define TAG_PXSP 0x50585350u
#define TAG_PHAR 0x52414850u
#define TAG_PLHT 0x54484c50u
#define TAG_PJEN 0x4e454a50u
#define TAG_PLEX 0x58454c50u
#define TAG_PLNA 0x414e4c50u
#define TAG_PFNA 0x414e4650u

static int find_field(const nflstreet_team_table *t,uint32_t tag,
 unsigned expected_type,unsigned *index){
 unsigned found=0;
 for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;
  if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag!=tag)continue;
  if(f.type!=expected_type||++found!=1)return 0;
  *index=i;
 }
 return found==1;
}

static int key_format(char *out,size_t cap,const char *prefix,
 const char *name,const char *suffix){
 int n=snprintf(out,cap,"%s%s%s",prefix,name,suffix);
 return n>0&&(size_t)n<cap;
}

static int ask(nflstreet_team_player_manifest_lookup lookup,void *context,
 unsigned pool,const char *key,int32_t *out){
 int32_t value=-1;
 if(!lookup||!lookup(context,pool,key,&value)||value < -1)return 0;
 *out=value;return 1;
}

int nflstreet_team_player_manifest_from_db(const uint8_t *bytes,size_t size,
 uint32_t team_id,uint32_t player_id,
 nflstreet_team_player_manifest_lookup lookup,void *lookup_context,
 nflstreet_team_player_manifest *out){
 if(!bytes||!out||!team_id||!lookup)return 0;
 nflstreet_team_table t;
 if(!nflstreet_team_table_open_tag(bytes,size,PLAY_TAG,&t))return 0;
 const uint32_t numeric_tags[]={TAG_DIGT,TAG_DIGP,TAG_IKSP,TAG_CFRP,TAG_TBYP,
  TAG_TGHP,TAG_YTRP,TAG_PXSP,TAG_PHAR,TAG_PLHT,TAG_PJEN,TAG_PLEX};
 unsigned ix[12];
 for(unsigned i=0;i<12;++i)if(!find_field(&t,numeric_tags[i],3,&ix[i]))return 0;
 unsigned plna_ix=0,pfna_ix=0;
 if(!find_field(&t,TAG_PLNA,0,&plna_ix)||!find_field(&t,TAG_PFNA,0,&pfna_ix))return 0;

 nflstreet_team_player_manifest next={0};
 next.team_id=team_id;next.player_id=player_id;
 next.head_candidate_index=-1;next.head_runtime_index=-1;
 next.hair_candidate_index=-1;next.hair_hat_candidate_index=-1;
 next.hair_variant_resolved=0;
 unsigned matches=0;
 for(unsigned row=0;row<t.used;++row){uint32_t id=0,pid=0;
  if(!nflstreet_team_table_number(&t,row,ix[0],&id)||
     !nflstreet_team_table_number(&t,row,ix[1],&pid))return 0;
  if(id!=team_id||pid!=player_id)continue;
  if(++matches!=1)return 0;
  next.play_row=row;
  uint32_t *values[]={&next.team_id,&next.player_id,&next.iksp,&next.cfrp,
   &next.tbyp,&next.tghp,&next.ytrp,&next.pxsp,&next.phar,&next.plht,
   &next.pjen,&next.plex};
  for(unsigned i=0;i<12;++i)
   if(!nflstreet_team_table_number(&t,row,ix[i],values[i]))return 0;
  if(!nflstreet_team_table_text(&t,row,plna_ix,next.plna,sizeof next.plna)||
     !nflstreet_team_table_text(&t,row,pfna_ix,next.pfna,sizeof next.pfna))return 0;
 }
 if(matches!=1)return 0;
 next.phgt=next.tghp; /* 8014F028 reads the same raw 0x54474850 tag. */

 if(next.ytrp==0){
  next.custom_name_branch=1;
  if(!nflstreet_team_identity_name(next.plna,next.pfna,26,next.normalized_name)||
     !next.normalized_name[0])return 0;
  if(!key_format(next.head_key,sizeof next.head_key,"HEAD_MODEL_",
                 next.normalized_name,"_2700")||
     !key_format(next.hair_key,sizeof next.hair_key,"HAIR_MODEL_",
                 next.normalized_name,"_2700")||
     !key_format(next.hair_hat_key,sizeof next.hair_hat_key,"HAIR_MODEL_",
                 next.normalized_name,"HAT_2700"))return 0;
  if(!ask(lookup,lookup_context,NFLSTREET_TEAM_PLAYER_POOL_FACE,
          next.head_key,&next.head_candidate_index)||
     !ask(lookup,lookup_context,NFLSTREET_TEAM_PLAYER_POOL_ROSTER_HAIR,
          next.hair_key,&next.hair_candidate_index)||
     !ask(lookup,lookup_context,NFLSTREET_TEAM_PLAYER_POOL_ROSTER_HAIR,
          next.hair_hat_key,&next.hair_hat_candidate_index))return 0;
  next.head_runtime_index=next.head_candidate_index<0?0x28a:
                          next.head_candidate_index;
 }

 if(next.ytrp==0)next.portrait_selector=next.pxsp;
 else {
  /* Include the largest TBYP adjustment (2) in the overflow bound. */
  if(next.cfrp>(UINT32_MAX-0x1afu)/3u)return 0;
  next.portrait_selector=next.cfrp*3u+0x1adu+
    (next.tbyp==1?1u:next.tbyp==2?2u:0u);
 }
 if(next.tbyp<=2){next.body_key=next.tbyp;next.body_key_supported=1;}
 else if(next.tbyp==7){next.body_key=2;next.body_key_supported=1;}
 *out=next;
 return 1;
}
