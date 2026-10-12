#ifndef NFLSTREET_MENU_SCRIPT_H
#define NFLSTREET_MENU_SCRIPT_H
#include <stddef.h>
#include <stdint.h>
/* Bounded integer/control and finite-float subset of original 801D16A0. Logical addresses
   serialize owned screen/stack bytes; no live RAM or input mapping.
   Opcode7D supports the supplied null diagnostic callback branch.
   Services require explicit transactional handlers; unsupported instructions
   and unsupported/non-finite conversions reject atomically. */
/* root supplies original param5 for opcode47's selected-self guard.
   linked_screen supplies loaded record+8 for opcode48; no screen ID is inferred.
   current_screen supplies packed loaded record+4/+6 for source load requests. */
typedef struct {uint32_t words[256],sp,pc,flags,root,linked_screen,current_screen,suspension;} nflstreet_ui_script;
/* Requests retain original numeric IDs. Service state is copied for each run
   and committed with screen/stack only on success. Handlers must mutate only
   the supplied copies and must not perform external I/O. */
/* kind9 retains load/readiness/activation request fields: source packed ID,
   target packed ID, link for load0, aux for readiness1. Property8 retains
   original72 loaded-record ordering: signed displacement aux and source link. Uninitialized padding
   from the original payload is excluded. */
/* kind10 retains opcode4C destination descriptor, format descriptor and ordered arguments. */
typedef struct {unsigned kind;uint32_t target,property,class_id,argc,args[16],source,link,aux;} nflstreet_ui_service_request;
typedef int (*nflstreet_ui_script_service)(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*result);
/* Optional service access to the transactional owned VM stack. Words are host
   uint32 values; logical addresses are supplied by stack_base. No live RAM or
   external I/O. The callback must bound/alignment-check pointer arguments. */
typedef int (*nflstreet_ui_script_memory_service)(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*,uint32_t stack_base,uint32_t words[256]);
/* Optional original70 continuation capture. Return1 accepts an owned frame,
   2 means capacity exhausted (original discards only argument words),0 rejects.
   Capture receives a transactional context copy and must avoid external I/O. */
typedef int (*nflstreet_ui_script_capture)(void*,const nflstreet_ui_script*,const nflstreet_ui_service_request*);
int nflstreet_ui_script_services_memory(uint8_t*,size_t,uint32_t,uint32_t,
 nflstreet_ui_script*,unsigned,nflstreet_ui_script_service,void*,size_t,
 nflstreet_ui_script_capture,nflstreet_ui_script_memory_service);
int nflstreet_ui_script_services_capture(uint8_t*,size_t,uint32_t,uint32_t,
 nflstreet_ui_script*,unsigned,nflstreet_ui_script_service,void*,size_t,nflstreet_ui_script_capture);
int nflstreet_ui_script_services(uint8_t*,size_t,uint32_t,uint32_t,
 nflstreet_ui_script*,unsigned,nflstreet_ui_script_service,void*,size_t);
int nflstreet_ui_script_run(uint8_t*screen,size_t,uint32_t screen_base,
 uint32_t stack_base,nflstreet_ui_script*,unsigned budget);
#endif
