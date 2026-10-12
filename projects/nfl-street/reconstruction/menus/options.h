#ifndef NFLSTREET_MENU_OPTIONS_H
#define NFLSTREET_MENU_OPTIONS_H
#include "script.h"
/* Original 80123988, reached through the group5/entry0 common callback.
   Only supplied query80004668 == 0 is supported. External calls are retained
   as ordered requests; no sound, hardware, card or profile I/O is performed. */
typedef struct {uint32_t function,argc,args[2];} nflstreet_ui_options_request;
typedef struct {
 uint32_t query_4668,query_7,sound_handle,query_440f0,query_441f4[2];
 uint32_t word_6da8,byte_6db0,word_6da0,word_6dac,word_6d9c,byte_59f8;
 uint32_t count;nflstreet_ui_options_request requests[64];
} nflstreet_ui_options;
int nflstreet_ui_options_initialize(nflstreet_ui_options*);
/* Only common callback80000001, argc0. Other Options services reject. */
int nflstreet_ui_options_service(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*);
#endif
