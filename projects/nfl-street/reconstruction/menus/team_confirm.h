#ifndef NFLSTREET_TEAM_CONFIRM_H
#define NFLSTREET_TEAM_CONFIRM_H
#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_TEAM_CONFIRM_SIDES 2u
#define NFLSTREET_TEAM_CONFIRM_ROSTER_MAX 20u
#define NFLSTREET_TEAM_CONFIRM_VISIBLE_SLOTS 14u
#define NFLSTREET_TEAM_CONFIRM_MAP_WORDS 28u
#define NFLSTREET_TEAM_CONFIRM_EFFECTS 5u

/* The source roster triple at 0x802F50D4 is (player ID, signed slot,
   auxiliary word). Only the signed interpretation of the middle word is
   used by 8011DF88; the third word is retained and swapped unchanged. */
typedef struct {uint32_t player_id;int32_t slot;uint32_t auxiliary;} nflstreet_team_confirm_row;
typedef struct {
 uint32_t count;
 nflstreet_team_confirm_row rows[NFLSTREET_TEAM_CONFIRM_ROSTER_MAX];
 uint8_t source_flag_6e34,source_flag_6e30;
 uint32_t last_confirmed_id;
} nflstreet_team_confirm_side;
typedef enum {
 NFLSTREET_TEAM_CONFIRM_VISIBILITY_BYTE=1,
 NFLSTREET_TEAM_CONFIRM_PREVIEW_INDEX_WRITE,
 NFLSTREET_TEAM_CONFIRM_ROSTER_SWAP,
 NFLSTREET_TEAM_CONFIRM_LAST_ID_WRITE,
 NFLSTREET_TEAM_CONFIRM_FLAG_CLEAR
} nflstreet_team_confirm_effect_kind;
typedef struct {uint32_t kind,words[5];} nflstreet_team_confirm_effect;
typedef struct {
 nflstreet_team_confirm_side sides[NFLSTREET_TEAM_CONFIRM_SIDES];
 uint8_t visibility[NFLSTREET_TEAM_CONFIRM_VISIBLE_SLOTS];
 uint32_t preview_map[NFLSTREET_TEAM_CONFIRM_MAP_WORDS];
 uint32_t effect_count;
 nflstreet_team_confirm_effect effects[NFLSTREET_TEAM_CONFIRM_EFFECTS];
} nflstreet_team_confirm;

/* Bounded model of 8011DF88. Pose indices are the two words read from its
   supplied per-side/per-slot source tables (802F5340 and 802F5324). A -1 ID
   is the original no-op; unsupported array/table states reject atomically. */
int nflstreet_team_confirm_apply(nflstreet_team_confirm *,uint32_t side,
 uint32_t player_id,uint32_t *next_cursor,uint32_t *return_value,
 const uint32_t pose_index_a[2][7],const uint32_t pose_index_b[2][7]);

#endif
