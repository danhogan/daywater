#ifndef NFLSTREET_MENU_ACTIVATION_H
#define NFLSTREET_MENU_ACTIVATION_H
#include <stdint.h>
/* Owned subset of801D1090/801D3828/801D12D8. Loaded records and
   activation scripts are supplied; this does not load or render screens. */
typedef struct {uint32_t identity,pending,root_selected;} nflstreet_ui_loaded;
typedef struct {uint32_t kind,event,slot,flags,active,identity;} nflstreet_ui_activation_effect;
typedef struct {uint32_t flags,active,count,coroutines,busy,notify;
 nflstreet_ui_loaded loaded[16];uint32_t effect_count;
 nflstreet_ui_activation_effect effects[64];} nflstreet_ui_activation;
/* activate=0 emits only the original deactivation stage, when unsuspended.
   activate=1 additionally selects an eligible supplied loaded record.
   Effects represent source-script/observer boundaries; caller must dispatch. */
int nflstreet_ui_activate(nflstreet_ui_activation*,unsigned activate,uint32_t identity);
#endif
