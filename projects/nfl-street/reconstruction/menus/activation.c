#include "activation.h"
static int emit(nflstreet_ui_activation*s,unsigned kind,unsigned event){
 if(s->active>=s->count)return 1;if(s->effect_count>=64)return 0;
 s->effects[s->effect_count++]=(nflstreet_ui_activation_effect){kind,event,s->active,s->flags,s->active,s->loaded[s->active].identity};return 1;
}
int nflstreet_ui_activate(nflstreet_ui_activation*s,unsigned activate,uint32_t identity){
 if(!s||s->count>16||s->effect_count>64||activate>1||s->busy>255)return 0;
 nflstreet_ui_activation t=*s;
 if(!t.coroutines){t.flags|=2;if(!emit(&t,0,0xfffffffb))return 0;t.flags&=~2u;}
 if(activate&&!t.busy){unsigned i;for(i=0;i<t.count;++i)if(t.loaded[i].identity==identity)break;
  if(i<t.count){if(t.loaded[i].pending)return 0;t.loaded[i].root_selected=1;t.active=i;
   if(t.notify&&!emit(&t,1,0))return 0;t.flags|=2;if(!emit(&t,0,0xfffffffc))return 0;t.flags&=~2u;
  }
 }
 *s=t;return 1;
}
