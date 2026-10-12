#ifndef NFLSTREET_OPTIONS_SCREEN_H
#define NFLSTREET_OPTIONS_SCREEN_H
#include "options.h"
#include "main_menu.h"
#include "coroutine.h"
typedef struct {nflstreet_ui_options options;nflstreet_ui_main_menu shared;
 nflstreet_font fonts[3];uint32_t profile[24];
 uint32_t selection,preview_channel,query_41d54,query_99bcc,query_c498;
 nflstreet_ui_coroutines coroutines;
 uint32_t return_count,return_identity;
 uint32_t effect_count;struct {uint32_t function,argc,args[3];} effects[128];} nflstreet_ui_options_screen;
/* Controlled group5/entry0 root FFFE script on already constructed objects.
   Supplied fonts/profile values and static non-streaming archives. Requests
   are retained; this legacy helper covers only the root. Child startup uses start below. */
int nflstreet_ui_options_screen_service(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*);
int nflstreet_ui_options_screen_activate(uint8_t*,size_t,uint32_t,nflstreet_ui_options_screen*);
/* Advance owned scheduler; focus/broadcast requests stay retained for caller dispatch. */
int nflstreet_ui_options_screen_advance(uint8_t*,size_t,uint32_t,nflstreet_ui_options_screen*,uint32_t elapsed);
/* Child creation followed by the selected-screen activation event. */
int nflstreet_ui_options_screen_start(uint8_t*,size_t,uint32_t,nflstreet_ui_options_screen*);
/* Source numeric events2..7; queued selection is executed in source order.
   Retained effect arrays are bounded logs; callers drain/clear between updates.
   Profile/query context is explicitly supplied. Setting coroutines.capacity enables
   capture only; card recipients and result routing remain the caller responsibility. */
int nflstreet_ui_options_screen_control(uint8_t*,size_t,uint32_t,nflstreet_ui_options_screen*,unsigned event);
/* Resume only an explicitly completed destination; result meaning remains
   supplied by the checked recipient. Nested source70 captures are retained. */
int nflstreet_ui_options_screen_complete(uint8_t*,size_t,uint32_t,nflstreet_ui_options_screen*,uint32_t destination,uint32_t result,unsigned*completed);
#endif
