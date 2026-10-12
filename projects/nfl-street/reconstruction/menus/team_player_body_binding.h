#ifndef NFLSTREET_TEAM_PLAYER_BODY_BINDING_H
#define NFLSTREET_TEAM_PLAYER_BODY_BINDING_H

#include <stddef.h>
#include <stdint.h>
#include "../native_assets/archive.h"

/* A decoded plamodel base body selected by the original TBYP -> PLYRBODY
   lookup and descriptor variant route. Owns bytes and the parsed Info name. */
typedef struct {
    uint32_t tbyp, pool_key, variant, archive_index;
    uint8_t *bytes;
    size_t size;
    char info_name[128];
} nflstreet_team_body_binding;

/* Bounded binding for the verified base body table: TBYP 7 aliases key 2;
   keys 0..2 and variants 0..2 select TERF indices 1 + key*3 + variant. */
int nflstreet_team_body_binding_open(const uint8_t *archive_bytes,
                                     size_t archive_size,
                                     uint32_t tbyp,
                                     uint32_t variant,
                                     nflstreet_team_body_binding *out);
void nflstreet_team_body_binding_close(nflstreet_team_body_binding *binding);

#endif
