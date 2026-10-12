#ifndef NFLSTREET_TEAM_PLAYER_WINDOW_CALLBACK_H
#define NFLSTREET_TEAM_PLAYER_WINDOW_CALLBACK_H
#include "team_player_window.h"
#include "script.h"

/* Stack-aware service adapter for original callback E9D4 properties 0x0F/0x10.
   Screen bytes and the 256-word VM stack are separately owned inputs. */
int nflstreet_team_player_window_callback_memory(void *,uint8_t *,size_t,uint32_t,
 const nflstreet_ui_service_request *,uint32_t *,uint32_t,uint32_t[256]);

#endif
