#ifndef NFLSTREET_MENU_DISPATCHER_H
#define NFLSTREET_MENU_DISPATCHER_H
#include "script.h"
typedef struct {uint32_t query134,transition[3];uint32_t count;nflstreet_ui_service_request requests[64];} nflstreet_ui_dispatcher;
/* Bounded source screen loader/readiness/activation request capture.
   Screen files and current loaded identity are supplied. No I/O or readiness claim. */
int nflstreet_ui_dispatcher_service(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*);
/* Actual group0/entry2 source root activation and checked Options notification.
   Execute source script only; retained requests need a manager/loader. */
int nflstreet_ui_dispatcher_activate(uint8_t*,size_t,uint32_t,uint32_t*,nflstreet_ui_dispatcher*);
int nflstreet_ui_dispatcher_options(uint8_t*,size_t,uint32_t,uint32_t*,nflstreet_ui_dispatcher*);
/* Source coordinator notifications1/2 and reactivationFFFC. Numeric state/result
   words are supplied by the originating source screen; no menu meanings inferred.
   Captures ordered load/readiness/activation requests; does not load recipients. */
int nflstreet_ui_dispatcher_notify(uint8_t*,size_t,uint32_t,uint32_t*,
 nflstreet_ui_dispatcher*,unsigned event,uint32_t state,uint32_t result);
int nflstreet_ui_dispatcher_reactivate(uint8_t*,size_t,uint32_t,uint32_t*,
 nflstreet_ui_dispatcher*);
/* Original group0/entry5 Quick Game coordinator: activationFFFE starts
   controller selection6; notifications1/2 and activationFFFC execute source
   forward/back routing to teams7, field8 and completion. Requests captured. */
int nflstreet_ui_quick_setup_event(uint8_t*,size_t,uint32_t,uint32_t*,
 nflstreet_ui_dispatcher*,unsigned event,uint32_t state,uint32_t result);
#endif
