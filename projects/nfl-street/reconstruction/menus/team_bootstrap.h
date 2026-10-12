#ifndef NFLSTREET_TEAM_BOOTSTRAP_H
#define NFLSTREET_TEAM_BOOTSTRAP_H
#include "team_bootstrap_static.h"
#include "team_bootstrap_weights.h"
#include "team_player_labels.h"
#include "team_player_window.h"
typedef struct {
 const nflstreet_team_bootstrap_static *constants;
 const uint8_t *template_db; size_t template_size;
 nflstreet_team_rating_curve curve;
 uint32_t weighting_mode; /* Explicit host seed outcome, original RNG not reconstructed. */
} nflstreet_team_bootstrap;
int nflstreet_team_bootstrap_prepare(const nflstreet_team_bootstrap *,const uint8_t *,size_t,
 uint32_t team,unsigned side,nflstreet_team_activation_side *,
 nflstreet_team_player_window_side *,nflstreet_team_label_row [20]);
#endif
