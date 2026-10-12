#include "team_start.h"
#include <stdint.h>
#include <string.h>

static int overlap(uintptr_t a,size_t an,uintptr_t b,size_t bn){
 return a<=b ? b-a<an : a-b<bn;
}

static int valid_catalog(const nflstreet_team_catalog*c){
 if(!c||!c->count||c->count>NFLSTREET_TEAM_CAROUSEL_MAX_IDS)return 0;
 for(unsigned i=0;i<c->count;++i){
  if(c->records[i].sivt>1)return 0;
  if(i&&c->records[i-1].id>=c->records[i].id)return 0;
 }
 return 1;
}

static void push(nflstreet_team_start_state*s,uint32_t kind,uint32_t side,
                 uint32_t a,uint32_t b,uint32_t c){
 s->effects[s->effect_count++]=(nflstreet_team_start_effect){kind,side,a,b,c};
}

static unsigned choose_center(const nflstreet_team_catalog*c,uint32_t selected){
 if(selected!=0xffffffffu)
  for(unsigned i=0;i<c->count;++i)if(c->records[i].id==selected)return i;
 /* 8011C1DC calls 801490D0 then 8011CCAC on a missing ID. With the supplied
    801E3C00 response zero, the random selector does not advance from raw row0. */
 return 0;
}

static void seed_carousel(const nflstreet_team_catalog*c,
                          nflstreet_team_start_state*s,unsigned side){
 nflstreet_team_carousel_catalog*out=&s->carousel.catalogs[side];
 out->count=c->count;
 unsigned center=choose_center(c,s->selected_id[side]);
 for(unsigned i=0;i<c->count;++i){
  out->entries[i].id=c->records[i].id;
  out->entries[i].d060_result=(uint32_t)(c->records[i].sivt==0);
 }
 s->carousel.windows[side][0]=c->records[(center+c->count-1u)%c->count].id;
 s->carousel.windows[side][1]=c->records[center].id;
 s->carousel.windows[side][2]=c->records[(center+1u)%c->count].id;
 out->cursor=(center+2u)%c->count;
}

int nflstreet_team_start_apply(const nflstreet_team_catalog*c,uint32_t query_mode,
                               nflstreet_team_start_state*state){
 if(!state||!valid_catalog(c)||query_mode>1||
    overlap((uintptr_t)c,sizeof *c,(uintptr_t)state,sizeof *state))return 0;
 if(!query_mode&&(state->eligibility_count[0]>255u||state->eligibility_count[1]>255u))return 0;
 nflstreet_team_start_state next=*state;
 next.refresh_calls=next.callback_calls=next.archive_reads=next.reset_calls=0;
 next.allocation_calls=next.query_open_calls=next.effect_count=0;
 memset(next.effects,0,sizeof next.effects);
 next.profile_mode=NFLSTREET_TEAM_START_PROFILE_MODE;

 /* 8011C294, no-profile branch: a non-FFFFFFFF previous value is cleared. */
 for(unsigned side=0;side<2;++side){
  next.profile_changed[side]=(uint32_t)(next.prior_profile[side]!=0xffffffffu);
  next.prior_profile[side]=0xffffffffu;
 }

 /* The no-profile 80150808 query configs differ by side in 8011C414. */
 next.query_configs[0][0]=next.query_configs[1][0]=1;
 next.query_configs[0][1]=next.query_configs[1][1]=1;
 next.query_configs[0][2]=0;
 next.query_configs[1][2]=1;

 if(query_mode){
  push(&next,NFLSTREET_TEAM_START_REFRESH,0xffffffffu,0x8014aafcu,0,0);
  ++next.refresh_calls;
 }
 for(unsigned side=0;side<2;++side){
  push(&next,NFLSTREET_TEAM_START_QUERY_OPEN,side,
       next.query_configs[side][0],next.query_configs[side][1],next.query_configs[side][2]);
  ++next.query_open_calls;
  if(query_mode){
   push(&next,NFLSTREET_TEAM_START_CALLBACK,side,0x8011c3a0u,side,0);
   ++next.callback_calls;
  }
 }

 /* 80150A00 false: keep each stored selected_id untouched. The selected row
    only determines the center used to build the three-entry carousel. */
 for(unsigned side=0;side<2;++side){
  if(query_mode){
   unsigned eligible=0;
   for(unsigned i=0;i<c->count;++i)eligible+=c->records[i].sivt!=0;
   next.eligibility_count[side]=eligible;
  }
  seed_carousel(c,&next,side);
 }

 push(&next,NFLSTREET_TEAM_START_ALLOC,0xffffffffu,8,0,0);++next.allocation_calls;
 push(&next,NFLSTREET_TEAM_START_ALLOC,0xffffffffu,12,0,0);++next.allocation_calls;
 push(&next,NFLSTREET_TEAM_START_ALLOC,0xffffffffu,12,0,0);++next.allocation_calls;
 push(&next,NFLSTREET_TEAM_START_ARCHIVE_OPEN,0xffffffffu,1,0,0);
 for(unsigned i=0;i<2;++i){
  push(&next,NFLSTREET_TEAM_START_ARCHIVE_READ,0xffffffffu,0x0d,0x3e,0);
  ++next.archive_reads;
 }
 push(&next,NFLSTREET_TEAM_START_RESET,0xffffffffu,0x8003f824u,0,0);++next.reset_calls;

 next.opaque_flags[0]=next.opaque_flags[1]=(uint8_t)(query_mode==1);
 if(query_mode){
  next.opaque_flags[2]=next.opaque_flags[3]=next.opaque_flags[4]=1;
  next.opaque_flags[5]=next.opaque_flags[6]=next.opaque_flags[7]=1;
 }
 next.zero_words[0]=next.zero_words[1]=next.zero_words[2]=0;
 *state=next;
 return 1;
}
