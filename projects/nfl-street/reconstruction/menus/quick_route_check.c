/* Independent join of original main/coordinator/Quick Game source scripts.
   Screen6/7/8 completion notifications are supplied. This verifies routing,
   not visible selection, asset loading, roster creation or match startup. */
#include "main_menu.h"
#include "dispatcher.h"
#include "order.h"
#include <stdio.h>
#include <stdlib.h>
static uint8_t *read_file(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<32||z>8*1024*1024){fclose(f);return NULL;}rewind(f);uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);b=NULL;}fclose(f);*n=(size_t)z;return b;}
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int requested(const nflstreet_ui_dispatcher*s,unsigned first,unsigned kind,unsigned target){for(unsigned i=first;i<s->count;++i)if(s->requests[i].property==kind&&s->requests[i].target==target)return 1;return 0;}
static int apply_order(nflstreet_ui_order*s,const nflstreet_ui_dispatcher*d,unsigned first){for(unsigned i=first;i<d->count;++i)if(d->requests[i].property==8&&!nflstreet_ui_order_request(s,&d->requests[i]))return 0;return 1;}
int main(int argc,char**argv){if(argc!=5)return 2;size_t n,dn,qn,fn;uint8_t*b=read_file(argv[1],&n),*d=read_file(argv[2],&dn),*q=read_file(argv[3],&qn),*f=read_file(argv[4],&fn);if(!b||!d||!q||!f)return 3;unsigned relocated,flags=0,quickflags=0;const unsigned base=0x1000000;
 if(!nflstreet_ui_relocate(b,n,base,&relocated)||!nflstreet_ui_relocate(d,dn,base,&relocated)||!nflstreet_ui_relocate(q,qn,base,&relocated))return 4;
 nflstreet_ui_order ordering={0};ordering.base=0x1080000;ordering.count=4;for(unsigned i=0;i<4;++i)ordering.loaded[i][1]=i+5;
 nflstreet_ui_main_menu menu={0};nflstreet_ui_dispatcher dispatcher={0},quick={0};if(!nflstreet_font_open(f,fn,&menu.font)||!nflstreet_ui_main_text_bind(b,n,base))return 4;
 unsigned nodes=be(b+4)-base;for(unsigned i=0;i<be(b);++i)put(b+be(b+nodes+i*20)-base+4,0);menu.rates.clock=16;menu.link_enabled=1;menu.linked_screen=2;
 if(!nflstreet_ui_main_layout(b,n,base,&menu,0)||!nflstreet_ui_dispatcher_activate(d,dn,base,&flags,&dispatcher))return 5;
 unsigned controls[]={17,13};for(unsigned k=0;k<2;++k){if(!nflstreet_ui_main_control(b,n,base,&menu,controls[k],7))return 6;for(unsigned tick=0;tick<160;++tick)if(!nflstreet_ui_main_advance(b,n,base,&menu,16))return 6;}
 if(menu.callback_word_6e74!=7||menu.link_count!=1||menu.links[0]!=2)return 7;
 unsigned first=dispatcher.count,forwarded=0;for(unsigned i=0;i<menu.broadcast_count;++i){const nflstreet_ui_service_request*r=&menu.broadcasts[i];if(r->property==1&&r->argc==2){if(!nflstreet_ui_dispatcher_notify(d,dn,base,&flags,&dispatcher,1,r->args[0],r->args[1]))return 7;++forwarded;}}
 if(forwarded!=1||!nflstreet_ui_dispatcher_reactivate(d,dn,base,&flags,&dispatcher)||!requested(&dispatcher,first,0,5))return 8;
 puts("Source main Play It -> Quick Game -> coordinator2 -> Quick Game coordinator5 passed;raw mode7 retained");
 if(!nflstreet_ui_quick_setup_event(q,qn,base,&quickflags,&quick,0xfffe,0,0)||!requested(&quick,0,0,6)||!requested(&quick,0,3,6))return 9;
 if(!apply_order(&ordering,&quick,0))return 9;puts("Source controller screen6 load/activation requested;ordering applied to supplied owned records");
 const unsigned targets[]={7,8};for(unsigned k=0;k<2;++k){first=quick.count;if(!nflstreet_ui_quick_setup_event(q,qn,base,&quickflags,&quick,1,0x76,UINT32_MAX)||!nflstreet_ui_quick_setup_event(q,qn,base,&quickflags,&quick,0xfffc,0,0)||!requested(&quick,first,0,targets[k])||!requested(&quick,first,3,targets[k])||!apply_order(&ordering,&quick,first))return 10;printf("Supplied completion -> source %s screen%u load/order/activation requested\n",k?"field":"team",targets[k]);}
 first=quick.count;if(!nflstreet_ui_quick_setup_event(q,qn,base,&quickflags,&quick,1,0x76,UINT32_MAX)||!nflstreet_ui_quick_setup_event(q,qn,base,&quickflags,&quick,0xfffc,0,0)||!requested(&quick,first,1,8)||be(q+0xf8)!=1)return 11;
 puts("Supplied field completion -> source field release/finished state1;native match creation remains unimplemented");
 puts("All source ordering requests applied;loaded recipient creation and selection notifications remain supplied");
 free(b);free(d);free(q);free(f);return 0;}
