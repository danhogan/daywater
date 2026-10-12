#ifndef NFLSTREET_UI_EVENTS_H
#define NFLSTREET_UI_EVENTS_H
#include "script.h"
/* Owned subset of801CF34C: child events precede selected-self events.
   Optional capture API retains original70 suspensions; child-return2 remains unsupported. */
int nflstreet_ui_events(uint8_t*,size_t,uint32_t,uint32_t identity,
 uint32_t*flags,unsigned node,uint16_t event,uint32_t argument,unsigned argc,
 const uint32_t*args,nflstreet_ui_script_service,void*,size_t,unsigned*executed);
int nflstreet_ui_events_linked(uint8_t*,size_t,uint32_t,uint32_t identity,uint32_t linked,
 uint32_t*flags,unsigned node,uint16_t event,uint32_t argument,unsigned argc,
 const uint32_t*args,nflstreet_ui_script_service,void*,size_t,unsigned*executed);
int nflstreet_ui_events_capture(uint8_t*,size_t,uint32_t,uint32_t,uint32_t,
 uint32_t*,unsigned,uint16_t,uint32_t,unsigned,const uint32_t*,
 nflstreet_ui_script_service,void*,size_t,unsigned*,nflstreet_ui_script_capture);
int nflstreet_ui_events_memory(uint8_t*,size_t,uint32_t,uint32_t,uint32_t,uint32_t*,unsigned,uint16_t,uint32_t,unsigned,const uint32_t*,nflstreet_ui_script_service,void*,size_t,unsigned*,nflstreet_ui_script_capture,nflstreet_ui_script_memory_service);
/* Explicit VM stack for manager-owned callers. Rejects a stack overlapping the
   source range even when the event has no matching script. */
int nflstreet_ui_events_memory_context(uint8_t*,size_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*,unsigned,uint16_t,uint32_t,unsigned,const uint32_t*,nflstreet_ui_script_service,void*,size_t,unsigned*,nflstreet_ui_script_capture,nflstreet_ui_script_memory_service);
#endif
