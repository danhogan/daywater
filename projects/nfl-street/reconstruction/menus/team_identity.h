#ifndef NFLSTREET_TEAM_IDENTITY_H
#define NFLSTREET_TEAM_IDENTITY_H
#include <stddef.h>
#include <stdint.h>
/* Raw selected PLAY identity, separate from captured NAP1 appearance. */
typedef struct {
 uint32_t team_id,player_id;
 char plna[15],pfna[13],model_name[27];
} nflstreet_team_identity;
/* Original 8014EB58's bounded ASCII name path: last then first, uppercase,
   retain A-Z. append_limit is the original third argument (at most 26 here).
   No profile/career-name branch or non-ASCII locale is represented. */
int nflstreet_team_identity_name(const char plna[15],const char pfna[13],
 uint32_t append_limit,char out[27]);
int nflstreet_team_identity_from_db(const uint8_t *,size_t,uint32_t team_id,
 uint32_t player_id,nflstreet_team_identity *);
#endif
