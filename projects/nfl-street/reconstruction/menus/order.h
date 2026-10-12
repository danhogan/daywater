#ifndef NFLSTREET_UI_ORDER_H
#define NFLSTREET_UI_ORDER_H
#include "script.h"
/* Owned original801D463C manager records. Words retain opaque source values.
   Logical row addresses identify references only; no host dereference or RAM I/O.
   Loaded identity=word1;rate owner=word7;continuation owner=word6. */
typedef struct {uint32_t base,count,active,rate_count,continuation_count;
 uint32_t loaded[16][5],rates[64][13],continuations[8][10];} nflstreet_ui_order;
int nflstreet_ui_order_move(nflstreet_ui_order*,uint32_t identity,int32_t displacement);
/* Applies captured opcode72/kind8 ordering only. Load/readiness/activation
   require their actual recipient handlers and are rejected by this adapter. */
int nflstreet_ui_order_request(nflstreet_ui_order*,const nflstreet_ui_service_request*);
#endif
