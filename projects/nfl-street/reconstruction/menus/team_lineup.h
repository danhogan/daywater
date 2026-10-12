#ifndef NFLSTREET_TEAM_LINEUP_H
#define NFLSTREET_TEAM_LINEUP_H
#include <stdint.h>

#define NFLSTREET_TEAM_LINEUP_MAX_PLAYERS 20u
#define NFLSTREET_TEAM_LINEUP_ROLE_COUNT 14u
#define NFLSTREET_TEAM_LINEUP_TOP_COUNT 4u
#define NFLSTREET_TEAM_LINEUP_NONE 0x7fffu

/* The caller supplies already weighted rows. This does not construct the
   database/session weighting performed by original 80042568. */
typedef struct {
 uint32_t player_id;
 uint32_t scores[NFLSTREET_TEAM_LINEUP_ROLE_COUNT];
 uint32_t role;
 uint8_t flags;
} nflstreet_team_lineup_player;

typedef struct { uint32_t player_id,score; } nflstreet_team_lineup_ranked;
typedef struct { uint32_t player_id,role,score; } nflstreet_team_lineup_choice;
typedef struct { uint32_t player_id,slot,aux; } nflstreet_team_lineup_triple;
typedef struct {
 uint32_t count;
 nflstreet_team_lineup_triple rows[NFLSTREET_TEAM_LINEUP_MAX_PLAYERS];
 uint32_t selected_ids[7];
 uint32_t last_seventh_player_id;
} nflstreet_team_lineup_roster;

/* Bounded translations of original 8004231C and 80042850 over supplied rows.
   flags==0 is the eligible-row condition from byte +0x3c. eligible_player_ids
   models the player filter prepared by 80042514; excluded_roles are role
   numbers skipped by 8004229C while 80042850 evaluates roles 0 through 13. */
int nflstreet_team_lineup_rank(const nflstreet_team_lineup_player *players,
 uint32_t player_count,uint32_t role,nflstreet_team_lineup_ranked out[4]);
int nflstreet_team_lineup_choose_role(const nflstreet_team_lineup_player *players,
 uint32_t player_count,const uint32_t *eligible_player_ids,uint32_t eligible_count,
 const uint32_t *excluded_roles,uint32_t excluded_count,
 nflstreet_team_lineup_choice *out);

/* Verified controlled model of E7EC/E610 over caller-supplied raw roster triples and
   weighted rows. It does not implement 80042568's database weight builder.
   Returns the seven selected IDs in final order and applies E7EC's cleanup
   (slot fields retained, aux fields reset to 0xff). */
int nflstreet_team_lineup_select_roster(nflstreet_team_lineup_roster *roster,
 const nflstreet_team_lineup_player *weighted,uint32_t weighted_count);

#endif
