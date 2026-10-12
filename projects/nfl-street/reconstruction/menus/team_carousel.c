#include "team_carousel.h"
#include <stdint.h>
#include <string.h>

static int ranges_overlap(uintptr_t a,size_t an,uintptr_t b,size_t bn){
 return a<=b ? b-a<an : a-b<bn;
}

static int find_id(const nflstreet_team_carousel_catalog*c,uint32_t id,uint32_t*index){
 for(uint32_t i=0;i<c->count;++i)if(c->entries[i].id==id){*index=i;return 1;}
 return 0;
}

static int valid(const nflstreet_team_carousel*s){
 if(!s)return 0;
 for(unsigned side=0;side<NFLSTREET_TEAM_CAROUSEL_SIDES;++side){
  const nflstreet_team_carousel_catalog*c=&s->catalogs[side];
  if(!c->count||c->count>NFLSTREET_TEAM_CAROUSEL_MAX_IDS||c->cursor>=c->count)return 0;
  for(uint32_t i=0;i<c->count;++i)for(uint32_t j=0;j<i;++j)
   if(c->entries[i].id==c->entries[j].id)return 0;
  for(unsigned i=0;i<NFLSTREET_TEAM_CAROUSEL_WINDOW;++i){uint32_t ignored;
   if(!find_id(c,s->windows[side][i],&ignored))return 0;
  }
 }
 return 1;
}

int nflstreet_team_carousel_apply(nflstreet_team_carousel*s,uint32_t side,
                                  uint32_t operation,nflstreet_team_carousel_output*out){
 if(!valid(s)||!out||side>=NFLSTREET_TEAM_CAROUSEL_SIDES||
    ranges_overlap((uintptr_t)s,sizeof *s,(uintptr_t)out,sizeof *out))return 0;
 nflstreet_team_carousel next=*s;
 nflstreet_team_carousel_output result={{0,0,0},{0,0,0}};
 uint32_t*window=next.windows[side];
 nflstreet_team_carousel_catalog*c=&next.catalogs[side];
 uint32_t index;
 if(operation==1){
  if(!find_id(c,window[2],&index))return 0;
  window[0]=window[1];window[1]=window[2];
  window[2]=c->entries[(index+1u)%c->count].id;
 }else if(operation==3){
  if(!find_id(c,window[0],&index))return 0;
  window[2]=window[1];window[1]=window[0];
  window[0]=c->entries[(index+c->count-1u)%c->count].id;
 }
 for(unsigned i=0;i<NFLSTREET_TEAM_CAROUSEL_WINDOW;++i){
  if(!find_id(c,window[i],&index))return 0;
  c->cursor=index;
  result.values[i]=window[i];
  /* The supplied D060 result is (raw 801495BC SIVT result == 0).
     CE64 stores (8011D060 result == 0). */
  result.flags[i]=(uint32_t)(c->entries[index].d060_result==0);
 }
 *s=next;*out=result;return 1;
}
