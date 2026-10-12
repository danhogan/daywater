#ifndef NFLSTREET_MENU_TEXT_CONSTRUCT_H
#define NFLSTREET_MENU_TEXT_CONSTRUCT_H
#include "font.h"
/* Source class8 propertyFFFFFFFF: bind the relative descriptor and prepare
   one owned single-line object with an explicitly resolved font. Resource
   acquisition remains the caller's responsibility. Failure is atomic. */
int nflstreet_ui_text_construct(uint8_t*,size_t,uint32_t,uint32_t,const nflstreet_font*);
#endif
