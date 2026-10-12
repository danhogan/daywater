#ifndef NFLSTREET_MAIN_MENU_H
#define NFLSTREET_MAIN_MENU_H
#include "focus.h"
#include "text.h"
#include "rate.h"
typedef struct {uint32_t kind,archive,entry;} nflstreet_ui_button_effect;
typedef struct {nflstreet_font font;nflstreet_ui_rates rates;uint32_t entry_flags,callback_word_6e68,callback_byte_6f79,broadcast_count,resource_count,pending_count,audio_count;uint32_t registry_value,registry_count,registry[128];uint32_t callback_byte_6f78,callback_byte_7fcc,callback_word_6f7c,callback_byte_6dd0;uint32_t audio[128];
 nflstreet_ui_service_request broadcasts[128],pending[128];
 /* Explicit inspection adapter; zero keeps unsupported linked requests rejected. */
 uint32_t link_enabled,linked_screen,link_count,links[16];
 uint32_t callback_word_6e74; /* Original8011B0E0 property80000003 raw store. */
 nflstreet_ui_button_effect resources[128];} nflstreet_ui_main_menu;
/* Bounded original static archive and single-line font3 text services.
   No full class initialization, streamed archives, audio or match setup. */
/* Source class traversal, with an explicit handler instead of main-specific resources.
   Transaction ownership is the caller's; handlers must avoid external I/O. */
int nflstreet_ui_class_activation(uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,nflstreet_ui_script_service,void*);
int nflstreet_ui_main_service(void*,uint8_t*,size_t,uint32_t,
 const nflstreet_ui_service_request*,uint32_t*);
int nflstreet_ui_main_select(uint8_t*,size_t,uint32_t,nflstreet_ui_main_menu*,
 unsigned node,nflstreet_ui_focus_effects*);
/* Supplied font and six source text descriptor pointers, not full class startup. */
int nflstreet_ui_main_text_bind(uint8_t*,size_t,uint32_t);
int nflstreet_ui_main_layout(uint8_t*,size_t,uint32_t,nflstreet_ui_main_menu*,unsigned mode);
/* Source numeric event2/3 with supplied pressed-value word3F800000.
   Executes direction script and queued focus requests; physical routing is separate. */
int nflstreet_ui_main_move(uint8_t*,size_t,uint32_t,nflstreet_ui_main_menu*,
 unsigned current_node,unsigned event);
/* Controlled source confirm event7 for node, or root back event6.
   Broader callbacks still require explicit supported services. */
int nflstreet_ui_main_control(uint8_t*,size_t,uint32_t,nflstreet_ui_main_menu*,unsigned node,unsigned event);
int nflstreet_ui_main_advance(uint8_t*,size_t,uint32_t,nflstreet_ui_main_menu*,uint32_t elapsed);
#endif
