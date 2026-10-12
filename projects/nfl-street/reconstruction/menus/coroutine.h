#ifndef NFLSTREET_UI_COROUTINE_H
#define NFLSTREET_UI_COROUTINE_H
#include "script.h"
/* Owned original70 frozen VM frames. No screen loading, card I/O or dialog
   result is synthesized. The caller supplies destination completion words. */
typedef struct {nflstreet_ui_script vm;nflstreet_ui_service_request request;} nflstreet_ui_continuation;
typedef struct {uint32_t capacity,count;nflstreet_ui_continuation frames[8];} nflstreet_ui_coroutines;
int nflstreet_ui_coroutine_capture(void*,const nflstreet_ui_script*,const nflstreet_ui_service_request*);
/* Top destination match only. A lower matching destination/source prevents
   out-of-order completion, as in801D0310. Unrelated completion is a no-op.
   context and stack are transactional. completed reports whether a frame ran. */
int nflstreet_ui_coroutine_resume(nflstreet_ui_coroutines*,uint32_t destination,
 uint32_t result,uint8_t*,size_t,uint32_t,uint32_t,nflstreet_ui_script*,
 nflstreet_ui_script_service,void*,size_t,unsigned*completed);
#endif
