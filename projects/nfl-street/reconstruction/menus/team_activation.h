#ifndef NFLSTREET_TEAM_ACTIVATION_H
#define NFLSTREET_TEAM_ACTIVATION_H
#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_TEAM_ACTIVATION_SIDES 2u
#define NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX 20u
#define NFLSTREET_TEAM_ACTIVATION_FORMATION_WORDS 14u
#define NFLSTREET_TEAM_ACTIVATION_POSES 7u
#define NFLSTREET_TEAM_ACTIVATION_EFFECTS 64u
#include "team_lineup.h"

typedef struct {uint32_t id;int32_t selected;uint32_t auxiliary;} nflstreet_team_activation_row;
/* Position and rotation are supplied IEEE-754 binary32 encodings. The final
   word is passed to 80129068, whose callee consumes its low signed byte. */
typedef struct {uint32_t position[3],rotation,visibility;} nflstreet_team_activation_pose;
typedef struct {
 uint32_t count;
 nflstreet_team_activation_row roster[NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX];
 uint8_t dirty,rebuild;
 uint32_t formation_value;
 uint32_t formation_words[NFLSTREET_TEAM_ACTIVATION_FORMATION_WORDS];
 nflstreet_team_activation_pose poses[NFLSTREET_TEAM_ACTIVATION_POSES];
 uint32_t builder_count,builder_ids[NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX];
 /* Explicit input to the otherwise database-backed 80042568 boundary. Rows
    are keyed by player_id and supplied in the same universe as this roster. */
 uint32_t weighted_count;
 nflstreet_team_lineup_player weighted_rows[NFLSTREET_TEAM_ACTIVATION_ROSTER_MAX];
 /* The two seven-byte request orders consumed by 8004278C. Original DOL
    defaults are at 0x803A1E94 and 0x803A1E9C. */
 uint8_t request_order[2][NFLSTREET_TEAM_ACTIVATION_POSES];
 uint32_t selected_count,selected_ids[NFLSTREET_TEAM_ACTIVATION_POSES];
 uint32_t last_seventh_player_id;
 uint32_t formation_count,active_formation[NFLSTREET_TEAM_ACTIVATION_FORMATION_WORDS];
} nflstreet_team_activation_side;
typedef enum {NFLSTREET_TEAM_ACTIVATION_REBUILD=1,NFLSTREET_TEAM_ACTIVATION_BUILD,
 NFLSTREET_TEAM_ACTIVATION_POSITION,NFLSTREET_TEAM_ACTIVATION_ROTATION,
 NFLSTREET_TEAM_ACTIVATION_VISIBILITY,NFLSTREET_TEAM_ACTIVATION_DESELECT} nflstreet_team_activation_effect_kind;
typedef struct {uint32_t kind,index,words[5];} nflstreet_team_activation_effect;
typedef struct {
 nflstreet_team_activation_side sides[NFLSTREET_TEAM_ACTIVATION_SIDES];
 uint32_t effect_count;
 nflstreet_team_activation_effect effects[NFLSTREET_TEAM_ACTIVATION_EFFECTS];
} nflstreet_team_activation;

/* Apply clean, dirty, and dirty+rebuild 8011E460/E7EC/E3E8/8003F4EC paths.
   Database-generated weighted rows and the two 8004278C request orders remain
   explicit caller inputs. Optional output receives IDs from E460's
   nonnegative-slot scan (at most 7). */
int nflstreet_team_activation_apply(nflstreet_team_activation *,unsigned side,
                                    uint32_t *selected_output,size_t capacity);
#endif
