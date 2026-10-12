#ifndef NFLSTREET_QUICK_MATCH_SELECTION_H
#define NFLSTREET_QUICK_MATCH_SELECTION_H
#include "team_activation.h"
#include "field_bootstrap.h"
typedef struct {
    uint32_t team_ids[2], selected_players[2][7], formation_players[2][14];
    uint32_t field_slot;
    uint8_t field_key;
} nflstreet_quick_match_selection;
/* Native menu handoff snapshot. This owns selected inputs only; it does not
   implement original801EA480/session/actor creation or start a match. */
int nflstreet_quick_match_selection_build(const uint32_t team_ids[2],
    const nflstreet_team_activation *, const nflstreet_field_bootstrap *,
    unsigned field_slot, nflstreet_quick_match_selection *);
#endif
