#ifndef NFLSTREET_TEAM_PREVIEW_H
#define NFLSTREET_TEAM_PREVIEW_H
#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS 20u
#define NFLSTREET_TEAM_PREVIEW_VISIBLE_ROWS 7u
#define NFLSTREET_TEAM_PREVIEW_QUERY_RECORD_STRIDE 12u

/* Bounded projection of 8011D628/8011D5F8 state. Player IDs are supplied
   from the selected private PLAY rows; this is not an actor renderer. */
typedef struct {
 uint32_t team_id;
 uint32_t count;
 uint32_t player_ids[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS];
 uint32_t cursor;
 uint32_t cursor_mode;
 uint32_t cleared_slots[NFLSTREET_TEAM_PREVIEW_VISIBLE_ROWS];
 uint32_t clear_count;
} nflstreet_team_preview_roster;

/* Reads a v8 team DB block's TEAM/DIGT identity and PLAY rows filtered by
   PLAY/DIGT, returning their PLAY/DIGP values in stored row order. */
int nflstreet_team_preview_roster_from_db(const uint8_t *,size_t,uint32_t,
 uint32_t player_ids[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS],uint32_t *count);
int nflstreet_team_preview_team_id_from_db(const uint8_t *,size_t,uint32_t *team_id);

/* Reads original8014EAC4 asset selection from supplied private PLAY rows.
   YTRP=0 uses PXSP; otherwise CFRP*3+1AD plus TBYP 1/2 adjustment. */
int nflstreet_team_preview_assets_from_db(const uint8_t *,size_t,uint32_t,
 uint32_t assets[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS],uint32_t *count);

/* Original8004221C maps supplied TBYP 7 to key2. Original800DFBE4 and
   801EB3F4 request three plamodel entries per supported key0..2. Archive
   registration, model decoding and selected-athlete rendering stay separate. */
int nflstreet_team_preview_body_entries(uint32_t raw_tbyp,uint32_t entries[3]);

/* Models the owned result of 8011D628 followed by 8011D5F8. All inputs are
   supplied and state changes commit atomically. */
int nflstreet_team_preview_roster_select(nflstreet_team_preview_roster *,
 uint32_t side,uint32_t team_id,const uint32_t *player_ids,uint32_t count);

#endif
