#ifndef NFLSTREET_CARD_SCREEN_H
#define NFLSTREET_CARD_SCREEN_H
#include "options_screen.h"
#include "storage_dialog.h"
typedef struct {nflstreet_ui_options_screen ui;nflstreet_storage_dialog storage;uint32_t byte_6f77,word_5a14,query_69a8,query_6d6c,device[2],operation;char device_name[17],selected_name[17];uint32_t request_count;nflstreet_ui_service_request requests[8];} nflstreet_card_screen;
/* Original group6/entry4 source scripts. External device services retained. */
int nflstreet_card_screen_service(void*,uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*,uint32_t*);
int nflstreet_card_screen_construct(uint8_t*,size_t,uint32_t,nflstreet_card_screen*);
int nflstreet_card_screen_start(uint8_t*,size_t,uint32_t,nflstreet_card_screen*,const uint32_t args[2],unsigned*);
int nflstreet_card_screen_advance(uint8_t*,size_t,uint32_t,nflstreet_card_screen*,uint32_t);
/* Explicit nested destination completion; raw result semantics remain supplied. */
int nflstreet_card_screen_complete(uint8_t*,size_t,uint32_t,nflstreet_card_screen*,uint32_t,uint32_t,unsigned*);
#endif
