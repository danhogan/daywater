#ifndef NFLSTREET_MENU_LINK_H
#define NFLSTREET_MENU_LINK_H
#include "activation.h"
#include "script.h"
/* Semantic subset of801D4B3C/801D4CDC/801D502C, kind3 only.
   Uninitialized payload padding is excluded. No loading or script dispatch. */
typedef struct {uint32_t count,identities[16];nflstreet_ui_activation manager;} nflstreet_ui_links;
int nflstreet_ui_link_service(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*);
/* Queue pump guard bit1 and ordered source boundaries, supplied loaded screens. */
int nflstreet_ui_links_pump(nflstreet_ui_links*);
#endif
