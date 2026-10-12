#include "order.h"
#include <string.h>
int nflstreet_ui_order_move(nflstreet_ui_order*s,uint32_t identity,int32_t displacement){
 if(!s||!s->base||s->count>16||s->rate_count>64||s->continuation_count>8||s->count>(UINT32_MAX-s->base)/20)return 0;
 unsigned from=0;while(from<s->count&&s->loaded[from][1]!=identity)++from;if(from==s->count)return 1;
 uint32_t steps=displacement<0?0u-(uint32_t)displacement:(uint32_t)displacement;
 int direction=displacement<0?-1:1;
 while(steps--){int to=(int)from+direction;if(to<0||(unsigned)to>=s->count)break;
  unsigned next=(unsigned)to;uint32_t a=s->base+from*20,b=s->base+next*20;
  if(s->active==next)s->active=from;else if(s->active==from)s->active=next;
  for(unsigned i=0;i<s->rate_count;++i){uint32_t*v=&s->rates[i][7];if(*v==b)*v=a;else if(*v==a)*v=b;}
  for(unsigned i=0;i<s->continuation_count;++i){uint32_t*v=&s->continuations[i][6];if(*v==b)*v=a;else if(*v==a)*v=b;}
  uint32_t row[5];memcpy(row,s->loaded[next],sizeof row);memcpy(s->loaded[next],s->loaded[from],sizeof row);memcpy(s->loaded[from],row,sizeof row);from=next;
 }
 return 1;
}
int nflstreet_ui_order_request(nflstreet_ui_order*s,const nflstreet_ui_service_request*q){
 if(!q||q->kind!=9||q->property!=8||q->argc)return 0;return nflstreet_ui_order_move(s,q->target,(int32_t)q->aux);
}
