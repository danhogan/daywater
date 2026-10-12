#ifndef NFLSTREET_MENU_FOCUS_H
#define NFLSTREET_MENU_FOCUS_H
#include "screen.h"
#include "script.h"
/* Original 801D0E18 focus stage with no optional index-list argument.
   Script requests are captured, not silently executed or discarded. */
typedef struct {uint32_t root_offset,script_offset,args[2];} nflstreet_ui_focus_request;
typedef struct {nflstreet_ui_focus_request requests[4];unsigned count;} nflstreet_ui_focus_effects;
int nflstreet_ui_focus(uint8_t*,size_t,uint32_t base,unsigned node,unsigned selected,
 uint32_t argument,nflstreet_ui_focus_effects*);
/* Execute requests immediately in original order, including previous deselect
   before target selection; entire file, flags and service context are atomic. */
int nflstreet_ui_focus_execute(uint8_t*,size_t,uint32_t base,unsigned node,
 unsigned selected,uint32_t argument,nflstreet_ui_focus_effects*,uint32_t*flags,
 nflstreet_ui_script_service,void*,size_t);
/* Original optional index-list argument, including source index passed to scripts. */
int nflstreet_ui_focus_indexed_execute(uint8_t*,size_t,uint32_t,unsigned,unsigned,
 uint32_t,nflstreet_ui_focus_effects*,uint32_t*,nflstreet_ui_script_service,
 void*,size_t,uint32_t list);
/* Opt-in context-aware form for a manager-owned script stack. Unlike the
   legacy entry point, this passes explicit screen identities and stack_base
   into VM services. Screen bytes, service context and focus effects commit
   together only when the complete focus operation succeeds. */
int nflstreet_ui_focus_indexed_execute_context(uint8_t*,size_t,uint32_t,
 unsigned node,unsigned selected,uint32_t argument,
 nflstreet_ui_focus_effects*,uint32_t*flags,
 uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base,
 nflstreet_ui_script_service,void*,size_t,uint32_t list,
 nflstreet_ui_script_memory_service);
#endif
