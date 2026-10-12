#ifndef NFLSTREET_TEAM_PLAYER_MANIFEST_H
#define NFLSTREET_TEAM_PLAYER_MANIFEST_H

#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_TEAM_PLAYER_MANIFEST_NAME_BYTES 27u
#define NFLSTREET_TEAM_PLAYER_MANIFEST_RAW_NAME_BYTES 15u
#define NFLSTREET_TEAM_PLAYER_MANIFEST_RAW_GIVEN_BYTES 13u

/* These are native manifest callback selectors only, not original archive
   type/enum values. */
enum {
 NFLSTREET_TEAM_PLAYER_POOL_FACE = 1,
 NFLSTREET_TEAM_PLAYER_POOL_ROSTER_HAIR = 2
};

/* Lookup is an explicit dependency. It must return the index reported by the
   selected lookup oracle, or -1 for no unique match. The manifest does not
   implement HSH1 and does not choose a hair/hat branch. */
typedef int (*nflstreet_team_player_manifest_lookup)(void *context,
 unsigned pool,const char *key,int32_t *index_out);

typedef struct {
 uint32_t team_id, player_id, play_row;
 uint32_t iksp, cfrp, tbyp, tghp, ytrp, pxsp;
 uint32_t phar, plht, pjen, plex;
 uint32_t phgt; /* Alias of raw TGHP (0x54474850), not a separate DB field. */
 uint32_t portrait_selector;
 uint32_t body_key;
 uint8_t body_key_supported;
 uint8_t custom_name_branch;
 uint8_t hair_variant_resolved; /* Always zero: original choice is not mapped. */
 char plna[NFLSTREET_TEAM_PLAYER_MANIFEST_RAW_NAME_BYTES];
 char pfna[NFLSTREET_TEAM_PLAYER_MANIFEST_RAW_GIVEN_BYTES];
 char normalized_name[NFLSTREET_TEAM_PLAYER_MANIFEST_NAME_BYTES];
 char head_key[64];
 char hair_key[64];
 char hair_hat_key[64];
 int32_t head_candidate_index;
 int32_t head_runtime_index; /* 0x28a is original miss fallback. */
 int32_t hair_candidate_index;
 int32_t hair_hat_candidate_index;
} nflstreet_team_player_manifest;

/* Extracts one exact PLAY row from a version8 DB block. Keys and indices are
   requested only on YTRP==0, the observed custom-name branch. Portrait uses
   the verified selector formula; body_key records only the TBYP->key mapping.
   This manifest identifies resources but does not resolve archive-to-file or
   render an athlete. Output is committed only on complete success. */
int nflstreet_team_player_manifest_from_db(const uint8_t *bytes,size_t size,
 uint32_t team_id,uint32_t player_id,
 nflstreet_team_player_manifest_lookup lookup,void *lookup_context,
 nflstreet_team_player_manifest *out);

#endif
