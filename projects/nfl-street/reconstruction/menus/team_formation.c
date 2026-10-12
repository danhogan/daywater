#include "team_formation.h"
#include <string.h>
int nflstreet_team_formation_assign(const nflstreet_team_lineup_player*p,uint32_t count,
 const uint8_t order[2][7],uint32_t out[14]){
 if(!p||!order||!out||count<7||count>20)return 0;
 for(unsigned side=0;side<2;++side){unsigned mask=0;for(unsigned i=0;i<7;++i){unsigned r=order[side][i];if(r>=7||mask&(1u<<r))return 0;mask|=1u<<r;}}
 for(unsigned i=0;i<count;++i){if(p[i].player_id==0x7fff)return 0;for(unsigned j=0;j<i;++j)if(p[i].player_id==p[j].player_id)return 0;}
 nflstreet_team_lineup_player rows[20];memcpy(rows,p,count*sizeof *p);uint32_t result[14];
 for(unsigned side=0;side<2;++side){if(side)for(unsigned i=0;i<count;++i)rows[i].flags=0;
  for(unsigned i=0;i<7;++i){unsigned role=order[side][i]+side*7;nflstreet_team_lineup_ranked top[4];if(!nflstreet_team_lineup_rank(rows,count,role,top)||top[0].player_id==0x7fff)return 0;
   result[role]=top[0].player_id;for(unsigned j=0;j<count;++j)if(rows[j].player_id==top[0].player_id){rows[j].flags=1;break;}
  }
 }
 memcpy(out,result,sizeof result);return 1;
}
