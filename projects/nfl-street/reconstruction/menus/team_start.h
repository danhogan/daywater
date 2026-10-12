#ifndef NFLSTREET_TEAM_START_H
#define NFLSTREET_TEAM_START_H
#include "team_selection.h"

#define NFLSTREET_TEAM_START_EFFECT_CAPACITY 12u
#define NFLSTREET_TEAM_START_OPAQUE_FLAGS 8u
#define NFLSTREET_TEAM_START_PROFILE_MODE 6u

typedef enum {
 NFLSTREET_TEAM_START_REFRESH=1,
 NFLSTREET_TEAM_START_QUERY_OPEN,
 NFLSTREET_TEAM_START_CALLBACK,
 NFLSTREET_TEAM_START_ALLOC,
 NFLSTREET_TEAM_START_ARCHIVE_OPEN,
 NFLSTREET_TEAM_START_ARCHIVE_READ,
 NFLSTREET_TEAM_START_RESET
} nflstreet_team_start_effect_kind;

typedef struct {
 uint32_t kind, side, arg0, arg1, arg2;
} nflstreet_team_start_effect;

typedef struct {
 uint32_t prior_profile[2], selected_id[2];
 uint32_t profile_changed[2], eligibility_count[2];
 uint32_t profile_mode;
 nflstreet_team_carousel carousel;
 uint32_t query_configs[2][3];
 /* Order: r13-6E44, -6E43, -6E3F, -6E34, -6E33, -6E30, -6E2F, -6E40. */
 uint8_t opaque_flags[NFLSTREET_TEAM_START_OPAQUE_FLAGS];
 /* Order: r13-6E18, -6E14, -6E1C. */
 uint32_t zero_words[3];
 uint32_t refresh_calls, callback_calls, archive_reads, reset_calls;
 uint32_t allocation_calls, query_open_calls;
 uint32_t effect_count;
 nflstreet_team_start_effect effects[NFLSTREET_TEAM_START_EFFECT_CAPACITY];
} nflstreet_team_start_state;

/* Models only original 8011C414's controlled no-profile branch. `query_mode`
   supplies the repeated 8010C390 response and must be 0 or 1. The supplied
   8010B60C profile mode is 6; 80149AF0 is supplied true, 80150A00/801E3C00 are false,
   and 80004668 is false. Initial prior IDs, selected IDs, opaque flag bytes,
   and eligibility counts are read from `state`; query mode 0 clears opaque
   bytes -6E44/-6E43 but preserves eligibility counts and the remaining six
   opaque bytes. */
int nflstreet_team_start_apply(const nflstreet_team_catalog *,uint32_t query_mode,
                               nflstreet_team_start_state *state);

#endif
