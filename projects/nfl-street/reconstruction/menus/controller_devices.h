#ifndef NFLSTREET_CONTROLLER_DEVICES_H
#define NFLSTREET_CONTROLLER_DEVICES_H
#include "controller_select.h"
#define NFLSTREET_CONTROLLER_DEVICE_EFFECTS 128
typedef struct {
 uint8_t connected[8];
 uint32_t status[8],bindings[10],active_port,query_mode,profile_mode;
 uint32_t count;
 nflstreet_controller_effect effects[NFLSTREET_CONTROLLER_DEVICE_EFFECTS];
} nflstreet_controller_devices;
/* Device status values are supplied by the host. Original status2 is accepted
   here; no Windows hardware identification or controller mapping is inferred. */
int nflstreet_controller_refresh(nflstreet_controller_select*,nflstreet_controller_devices*,uint32_t force,uint32_t*changed);
/* Local8011B698 branch: remap, optional forced refresh, source profile mode
   requests and raw initial choice. Network/profile-file lookup is separate. */
int nflstreet_controller_initialize(nflstreet_controller_select*,nflstreet_controller_devices*,const int8_t map[4],uint32_t*choice);
#endif
