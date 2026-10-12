#ifndef NFLSTREET_TEAM_ACTOR_H
#define NFLSTREET_TEAM_ACTOR_H
#include "script.h"

#define NFLSTREET_TEAM_ACTOR_REQUEST_CAPACITY 64u

typedef struct {
 uint32_t selected_index;
 /* Raw IEEE-754 single-precision words read from object offsets +8,+4,+12,+16. */
 uint32_t float_words[4];
} nflstreet_team_actor_request;

typedef struct {
 uint32_t callback_registered;
 uint32_t request_count;
 nflstreet_team_actor_request requests[NFLSTREET_TEAM_ACTOR_REQUEST_CAPACITY];
} nflstreet_team_actor;

/* Bounded class3 object properties used by team entry7. Callback registration
   is supplied by the screen owner; emitted requests do not run a renderer. */
int nflstreet_team_actor_service(void *,uint8_t *,size_t,uint32_t,
                                 const nflstreet_ui_service_request *,uint32_t *);
#endif
