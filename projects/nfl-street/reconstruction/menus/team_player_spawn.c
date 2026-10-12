#include "team_player_spawn.h"
#include "team_catalog.h"
#include "team_identity.h"
#include <string.h>

#define PLAY_TAG 0x59414c50u
#define DIGT_TAG 0x44494754u
#define DIGP_TAG 0x44494750u
#define IKSP_TAG 0x494b5350u
#define CFRP_TAG 0x43465250u
#define TBYP_TAG 0x54425950u
#define TGHP_TAG 0x54474850u
#define YTRP_TAG 0x59545250u

static int numeric_field(const nflstreet_team_table *t,uint32_t tag,unsigned *index){
 unsigned found=0;
 for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;
  if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag!=tag)continue;
  if(f.type!=3||!f.width||f.width>32||++found!=1)return 0;
  *index=i;
 }
 return found==1;
}

int nflstreet_team_player_spawn_from_db(const uint8_t *bytes,size_t size,
 uint32_t team_id,uint32_t player_id,uint32_t actor_index,
 uint32_t callback_argument,uint32_t factory_enabled,uint8_t side_label,
 nflstreet_team_player_spawn_request *out){
 if(!bytes||!out||actor_index>=14||factory_enabled>1)return 0;
 nflstreet_team_table t;
 if(!nflstreet_team_table_open_tag(bytes,size,PLAY_TAG,&t))return 0;
 const uint32_t tags[]={DIGT_TAG,DIGP_TAG,IKSP_TAG,CFRP_TAG,TBYP_TAG,TGHP_TAG,YTRP_TAG};
 unsigned ix[7];
 for(unsigned i=0;i<7;++i)if(!numeric_field(&t,tags[i],&ix[i]))return 0;
 nflstreet_team_player_spawn_request next={0};
 next.team_id=team_id;next.player_id=player_id;next.actor_index=actor_index;
 next.callback_argument=callback_argument;next.factory_enabled=factory_enabled;
 next.side_label=side_label;
 unsigned matches=0;
 for(unsigned row=0;row<t.used;++row){uint32_t v[7];
  for(unsigned i=0;i<7;++i)if(!nflstreet_team_table_number(&t,row,ix[i],&v[i]))return 0;
  if(v[0]!=team_id||v[1]!=player_id)continue;
  if(++matches!=1)return 0;
  next.digt=v[0];next.iksp=v[2];next.cfrp=v[3];next.tbyp=v[4];next.tghp=v[5];next.ytrp=v[6];
 }
 if(matches!=1)return 0;
 /* 8014EB58 queries YTRP first. A nonzero result takes its empty-name path;
    the observed zero branch appends PLNA then PFNA and retains only A-Z. */
 if(!next.ytrp){nflstreet_team_identity identity;
  if(!nflstreet_team_identity_from_db(bytes,size,team_id,player_id,&identity))return 0;
  memcpy(next.model_name,identity.model_name,sizeof next.model_name);
 }
 *out=next;return 1;
}
