#ifndef NFLSTREET_CARD_PROMPT_H
#define NFLSTREET_CARD_PROMPT_H
#include "card_screen.h"
#include "text_lines.h"
typedef struct {nflstreet_card_screen card;char templates[8][256];uint32_t line_base,warning;unsigned text_count;struct {uint32_t target;nflstreet_ui_text_lines plan;} text[64];} nflstreet_card_prompt;
/* Original group1/entry9. Private original templates and device responses are
   supplied. Owned line addresses are serialization addresses, not live RAM. */
int nflstreet_card_prompt_service(void*,uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*,uint32_t*);
int nflstreet_card_prompt_memory(void*,uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*,uint32_t*,uint32_t,uint32_t[256]);
int nflstreet_card_prompt_construct(uint8_t*,size_t,uint32_t,nflstreet_card_prompt*);
int nflstreet_card_prompt_start(uint8_t*,size_t,uint32_t,nflstreet_card_prompt*,const uint32_t[3]);
int nflstreet_card_prompt_advance(uint8_t*,size_t,uint32_t,nflstreet_card_prompt*,uint32_t);
int nflstreet_card_prompt_control(uint8_t*,size_t,uint32_t,nflstreet_card_prompt*,unsigned);
#endif
