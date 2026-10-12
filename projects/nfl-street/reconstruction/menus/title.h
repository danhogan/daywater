#ifndef NFLSTREET_MENU_TITLE_H
#define NFLSTREET_MENU_TITLE_H
#include "rate.h"
#include "script.h"
/* Title script execution with original title callbacks and rate service.
   Broadcast requests are retained for the screen manager, not discarded or
   described as executed. Full screen loading/navigation remains separate. */
typedef struct {
 uint8_t title_flag,flag_470c,flag_22130;
 uint32_t entry_flags;
 unsigned broadcasts;
 nflstreet_ui_service_request requests[32];
 nflstreet_ui_rates rates;
} nflstreet_ui_title;
int nflstreet_ui_title_service(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*);
int nflstreet_ui_title_activate(uint8_t*,size_t,uint32_t,nflstreet_ui_title*);
int nflstreet_ui_title_advance(uint8_t*,size_t,uint32_t,nflstreet_ui_title*,uint32_t elapsed);
#endif
