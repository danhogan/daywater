#ifndef NFLSTREET_MENU_CONSTRUCT_H
#define NFLSTREET_MENU_CONSTRUCT_H
#include "script.h"
/* Owned801CE994 traversal. Callback registration is supplied for classes0..15.
   Calls retain original properties and logical object addresses. Resource/font
   effects require explicit handlers; no I/O or actual class effect is inferred. */
int nflstreet_ui_construct(uint8_t*,size_t,uint32_t,unsigned,uint32_t,uint16_t,
 nflstreet_ui_script_service,void*,size_t);
#endif
