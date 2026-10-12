#ifndef NFLSTREET_CONTROLLER_CALLBACK_H
#define NFLSTREET_CONTROLLER_CALLBACK_H
#include "controller_select.h"
#include "controller_devices.h"
#include "script.h"
typedef struct {
 nflstreet_controller_select selection;
 int8_t next_mapping[4];
 uint32_t handles[4];
 nflstreet_controller_commit committed;
 nflstreet_controller_devices devices;
} nflstreet_controller_callback;
/* Bounded local subset of group0/entry6 callback8011BC68. Device responses
   are supplied; network/profile database services remain unsupported. This plugs
   into the transactional VM; addresses refer only to owned file/stack bytes. */
int nflstreet_controller_callback_memory(void*,uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*,uint32_t*,uint32_t,uint32_t[256]);
#endif
