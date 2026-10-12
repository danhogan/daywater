#ifndef NFLSTREET_MENU_BACKGROUND_H
#define NFLSTREET_MENU_BACKGROUND_H
#include "rate.h"
/* Bounded original uis_all/0 background scripts, with explicitly supplied
   archive8 (uis_bgpl) readiness/busy states. Does not initialize the loader. */
typedef struct {uint32_t kind,archive,entry;} nflstreet_ui_resource_effect;
typedef struct {
 nflstreet_ui_rates rates;
 uint32_t entry_flags,effect_count;
 float random_sample; /* Supplied unit draw; original RNG stream not owned here. */
 uint8_t ready[22],busy[22];
 nflstreet_ui_resource_effect effects[512];
} nflstreet_ui_background;
int nflstreet_ui_background_activate(uint8_t*,size_t,uint32_t,nflstreet_ui_background*);
int nflstreet_ui_background_service(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*);
int nflstreet_ui_background_advance(uint8_t*,size_t,uint32_t,
 nflstreet_ui_background*,uint32_t elapsed);
#endif
