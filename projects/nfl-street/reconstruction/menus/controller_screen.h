#ifndef NFLSTREET_CONTROLLER_SCREEN_H
#define NFLSTREET_CONTROLLER_SCREEN_H
#include "controller_callback.h"
#include "options_screen.h"
/* Reuses checked text/image class services; Options policy is not dispatched.
   supplied_static_archives explicitly supplies flags without streaming bit2.
   Startup/advance/input are atomic; consume captured device effects between updates. */
typedef struct {nflstreet_ui_options_screen classes;nflstreet_controller_callback game;uint32_t supplied_static_archives;} nflstreet_controller_screen;
int nflstreet_controller_screen_service(void*,uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*,uint32_t*);
int nflstreet_controller_screen_memory(void*,uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*,uint32_t*,uint32_t,uint32_t[256]);
int nflstreet_controller_screen_construct(uint8_t*,size_t,uint32_t,nflstreet_controller_screen*);
int nflstreet_controller_screen_start(uint8_t*,size_t,uint32_t,nflstreet_controller_screen*);
int nflstreet_controller_screen_advance(uint8_t*,size_t,uint32_t,nflstreet_controller_screen*,uint32_t elapsed);
/* Already translated source event2..7, logical port0..9, supplied float0..1.
   Hardware translation/repeat, queued screen changes and request I/O are external. */
int nflstreet_controller_screen_input(uint8_t*,size_t,uint32_t,nflstreet_controller_screen*,uint32_t port,uint16_t event,float value);
#endif
