#include "link.h"
int nflstreet_ui_link_service(void*ctx,uint8_t*b,size_t n,uint32_t base,
 const nflstreet_ui_service_request*q,uint32_t*result){
 (void)b;(void)n;(void)base;(void)result;nflstreet_ui_links*s=ctx;
 if(!s||!q||q->kind!=8||q->property!=3||q->argc||s->count>=16)return 0;
 s->identities[s->count++]=q->target;return 1;
}
int nflstreet_ui_links_pump(nflstreet_ui_links*s){
 if(!s||s->count>16)return 0;if(s->manager.flags&1)return 1;
 nflstreet_ui_links t=*s;t.manager.flags|=1;
 for(unsigned i=0;i<t.count;++i)if(!nflstreet_ui_activate(&t.manager,1,t.identities[i]))return 0;
 t.count=0;t.manager.flags&=~1u;*s=t;return 1;
}
