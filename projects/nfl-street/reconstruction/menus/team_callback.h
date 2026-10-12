#ifndef NFLSTREET_TEAM_CALLBACK_H
#define NFLSTREET_TEAM_CALLBACK_H
#include "team_start.h"
#include "script.h"
typedef struct {
 nflstreet_team_catalog catalog;
 nflstreet_team_start_state startup;
 uint32_t query_mode;
} nflstreet_team_callback;
/* Original8011E9D4 startup, labels/raw fields, carousel and random0 refresh,
   plus bounded common logo and empty-device queries. Common logo uses supplied
   mode6/actor-gatefalse; regular raw team types only. Full original profile,
   database I/O and actor initialization are not implemented here. Screen and VM stack are caller-owned;
   unsupported requests and invalid destinations reject without mutation. */
int nflstreet_team_callback_memory(void *,uint8_t *,size_t,uint32_t,
 const nflstreet_ui_service_request *,uint32_t *,uint32_t,uint32_t[256]);
#endif
