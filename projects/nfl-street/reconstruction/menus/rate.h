#ifndef NFLSTREET_MENU_RATE_H
#define NFLSTREET_MENU_RATE_H
#include <stddef.h>
#include <stdint.h>
#include "script.h"
/* Original 801D1348/801CF610/801D15B0 on owned serialized records.
   Addresses are supplied identities, never live pointers. Script calls are
   delegated to the caller; a missing required service fails atomically. */
typedef struct {uint32_t words[13];} nflstreet_ui_rate_record;
typedef struct {uint32_t count,clock,flags; nflstreet_ui_rate_record records[64];} nflstreet_ui_rates;
/* result initially contains float1 for a tick, float0 for completion, matching
   801D399C's optional original return slot. */
typedef int (*nflstreet_ui_rate_script)(void*,nflstreet_ui_rates*,uint8_t*,size_t,uint32_t base,
 uint32_t screen,uint32_t root,
 uint32_t script,unsigned argc,const uint32_t*args,float*result);
unsigned nflstreet_ui_property_offset(uint32_t property);
int nflstreet_ui_rate_add(nflstreet_ui_rates*,const uint8_t*screen,size_t,
 uint32_t base,uint32_t screen_id,uint32_t root,uint32_t identity,
 uint32_t target_object,uint32_t property,float target,uint32_t duration,
 uint32_t completion,uint32_t tick_script);
int nflstreet_ui_rate_update(nflstreet_ui_rates*,uint8_t*screen,size_t,
 uint32_t base,uint32_t screen_id,uint32_t elapsed,uint32_t default_completion,
 nflstreet_ui_rate_script,void*);
/* Original801D1564 marks the first matching root/identity, without compaction. */
int nflstreet_ui_rate_cancel_identity(nflstreet_ui_rates*,uint32_t root,uint32_t identity);
int nflstreet_ui_rate_cancel(nflstreet_ui_rates*,uint32_t screen_id);
/* Decode original opcode06 timed-script,07 cancellation or58 request and construct its actual checked rate.
   Optional library-relative callbacks remain unsupported. */
int nflstreet_ui_rate_request(nflstreet_ui_rates*,uint8_t*,size_t,uint32_t base,
 uint32_t screen_id,const nflstreet_ui_service_request*);
#endif
