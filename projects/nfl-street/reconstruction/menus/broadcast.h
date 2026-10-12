#ifndef NFLSTREET_MENU_BROADCAST_H
#define NFLSTREET_MENU_BROADCAST_H
#include "script.h"
/* Original 801D4858: descend nested-node events before mask4000 selection.
   Scripts execute; unsupported services roll back the entire dispatch.
   Caller supplies the loaded recipient, its opaque flags and services. */
int nflstreet_ui_broadcast(uint8_t*,size_t,uint32_t base,uint32_t*entry_flags,
 uint16_t property,unsigned argc,const uint32_t*args,
 nflstreet_ui_script_service,void*,size_t,unsigned*executed);
#endif
