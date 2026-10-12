#ifndef NFLSTREET_TEAM_FORMATION_H
#define NFLSTREET_TEAM_FORMATION_H
#include "team_lineup.h"
/* Original8004278C assignment over supplied weighted rows and two supplied
   permutations of roles0..6. Role order is private formation data, not inferred.
   Database weight construction and actor loading are outside this stage. */
int nflstreet_team_formation_assign(const nflstreet_team_lineup_player *,uint32_t,
 const uint8_t role_order[2][7],uint32_t assigned_ids[14]);
#endif
