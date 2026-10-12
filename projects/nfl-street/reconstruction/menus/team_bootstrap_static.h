#ifndef NFLSTREET_TEAM_BOOTSTRAP_STATIC_H
#define NFLSTREET_TEAM_BOOTSTRAP_STATIC_H
#include "team_activation.h"
typedef struct {
 uint8_t *mapped_dol; size_t mapped_size;
 uint8_t formation_order[2][7],actor_labels[14];
 uint32_t gear_categories[12];
 nflstreet_team_activation_pose poses[2][7], reset_poses[2];
 char role_names[14][64];
} nflstreet_team_bootstrap_static;
/* Caller supplies the original hash-checked GNNE69-r0 DOL. Bounds are checked
   here; the owning private verification/asset loader checks its SHA1. No
   emulator/module execution or savestate is involved. */
int nflstreet_team_bootstrap_static_from_dol(const uint8_t *,size_t,
 nflstreet_team_bootstrap_static *);
void nflstreet_team_bootstrap_static_free(nflstreet_team_bootstrap_static *);
#endif
