#include "team_bootstrap.h"
#include "team_preview.h"
#include "team_identity.h"
#include <string.h>
int nflstreet_team_bootstrap_prepare(const nflstreet_team_bootstrap*h,const uint8_t*db,size_t n,
 uint32_t team,unsigned side,nflstreet_team_activation_side*a,
 nflstreet_team_player_window_side*w,nflstreet_team_label_row labels[20]){
 if(!h||!h->constants||!db||!a||!w||!labels||side>1)return 0;
 uint32_t ids[20],count;if(!nflstreet_team_preview_roster_from_db(db,n,team,ids,&count)||count<7)return 0;
 nflstreet_team_activation_side x={0};nflstreet_team_player_window_side y={0};nflstreet_team_label_row rows[20]={0};
 x.count=x.weighted_count=count;x.dirty=x.rebuild=1;x.last_seventh_player_id=NFLSTREET_TEAM_LINEUP_NONE;
 for(unsigned i=0;i<20;++i){x.roster[i].id=NFLSTREET_TEAM_LINEUP_NONE;x.roster[i].selected=-1;x.roster[i].auxiliary=0xff;}
 memcpy(x.request_order,h->constants->formation_order,sizeof x.request_order);memcpy(x.poses,h->constants->poses[side],sizeof x.poses);
 y.count=y.query_count=count;y.cursor=(int32_t)count-1;y.mode=1;
 for(unsigned i=0;i<count;++i){nflstreet_team_weight_inputs inputs;nflstreet_team_gear_response response[12]={0};
  if(!nflstreet_team_weights_from_db(db,n,team,ids[i],&inputs))return 0;
  if(!nflstreet_team_bootstrap_responses_from_template(h->template_db,h->template_size,&inputs,h->constants->gear_categories,0x17,response))return 0;
  nflstreet_team_bootstrap_weight_row weighted;
  if(!nflstreet_team_bootstrap_weights_build(db,n,h->template_db,h->template_size,&h->curve,h->weighting_mode,team,ids[i],h->constants->gear_categories,response,0x17,&weighted))return 0;
  x.roster[i].id=ids[i];x.weighted_rows[i].player_id=ids[i];x.weighted_rows[i].role=weighted.role;memcpy(x.weighted_rows[i].scores,weighted.weighted,sizeof weighted.weighted);
  y.rows[i].id=y.query_ids[i]=ids[i];y.rows[i].slot=-1;y.rows[i].auxiliary=0xff;
  nflstreet_team_identity identity;if(!nflstreet_team_identity_from_db(db,n,team,ids[i],&identity))return 0;
  size_t first=strlen(identity.pfna),last=strlen(identity.plna);if(first>=sizeof rows[i].first_name||last>=sizeof rows[i].last_name)return 0;
  memcpy(rows[i].first_name,identity.pfna,first+1);memcpy(rows[i].last_name,identity.plna,last+1);rows[i].pbpo=weighted.role;
  memcpy(rows[i].ratings_by_original_ordinal,weighted.adjusted_ratings,sizeof weighted.adjusted_ratings);
 }
 *a=x;*w=y;memcpy(labels,rows,sizeof rows);return 1;
}
