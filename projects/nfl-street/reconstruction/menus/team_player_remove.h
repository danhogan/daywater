#ifndef NFLSTREET_TEAM_PLAYER_REMOVE_H
#define NFLSTREET_TEAM_PLAYER_REMOVE_H
#include "team_activation.h"
#include "team_player_window.h"

/* Bounded E234 removal after activation. The ordered query IDs and side reset
   pose are supplied from their separate sources. */
int nflstreet_team_player_remove(nflstreet_team_activation *,
 nflstreet_team_player_window *,unsigned side,
 const nflstreet_team_activation_pose *reset_pose,uint32_t *removed_slot);

#endif
