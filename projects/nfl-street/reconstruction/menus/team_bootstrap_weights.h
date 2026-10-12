#ifndef NFLSTREET_TEAM_BOOTSTRAP_WEIGHTS_H
#define NFLSTREET_TEAM_BOOTSTRAP_WEIGHTS_H
#include "team_weights.h"

/* One bounded weighted row emitted by the 80042568 formula. role is the raw
   PLAY.PBPO value; scores and weights retain the literal 0..13 TSTR columns. */
typedef struct {
 uint32_t player_id,role;
 uint32_t adjusted_ratings[NFLSTREET_TEAM_WEIGHTS_BASE_COUNT];
 uint32_t scores[NFLSTREET_TEAM_WEIGHTS_ROLE_COUNT];
 uint32_t weighted[NFLSTREET_TEAM_WEIGHTS_ROLE_COUNT];
} nflstreet_team_bootstrap_weight_row;

/* The caller supplies the original 8014BC04 role-category values and each
   actual 801B1B30 status/result. A missing GEAR/GRIP join is never assigned a
   status here. The function fails when supplied success/status disagrees with
   raw table presence. */
int nflstreet_team_bootstrap_curve_from_template(const uint8_t *template_db,size_t template_size,
 const uint8_t *mapped_dol_ram,size_t ram_size,uint32_t ram_base,
 nflstreet_team_rating_curve *out);

/* Produces the status classes and decoded payloads whose 8014F2B0 aggregate
   was compared with the untouched query engine across the supplied roster.
   Present rows use status 0; absent unique joins use status_no_row (normally
   0x17). This models the verified contribution behavior, not other raw status
   codes the engine may return. */
int nflstreet_team_bootstrap_responses_from_template(const uint8_t *template_db,
 size_t template_size,const nflstreet_team_weight_inputs *inputs,
 const uint32_t categories[NFLSTREET_TEAM_WEIGHTS_QUERY_COUNT],uint32_t status_no_row,
 nflstreet_team_gear_response out[NFLSTREET_TEAM_WEIGHTS_QUERY_COUNT]);

int nflstreet_team_bootstrap_weights_build(const uint8_t *db_teams,size_t db_size,
 const uint8_t *template_db,size_t template_size,const nflstreet_team_rating_curve *curve,
 uint32_t mode,uint32_t team_id,uint32_t player_id,
 const uint32_t categories[NFLSTREET_TEAM_WEIGHTS_QUERY_COUNT],
 const nflstreet_team_gear_response statuses_and_responses[NFLSTREET_TEAM_WEIGHTS_QUERY_COUNT],
 uint32_t status_no_row,nflstreet_team_bootstrap_weight_row *out);

#endif
