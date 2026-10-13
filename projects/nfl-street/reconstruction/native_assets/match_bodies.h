#ifndef NFLSTREET_MATCH_BODIES_H
#define NFLSTREET_MATCH_BODIES_H
#include "match_visuals.h"
#include "body_postattach.h"
#include "match_skeletons.h"

typedef struct {
    uint32_t namespace_base, namespace_size;
    uint32_t model_token, view_token;
    uint32_t temporary_token, temporary_capacity;
    unsigned target_index;
} nflstreet_match_body_input;

typedef struct {
    nflstreet_owned_body players[NFLSTREET_MATCH_PLAYERS];
    unsigned count, temporary_allocations, temporary_releases;
} nflstreet_match_bodies;

/* Replace the borrowed body pointers in visuals only after all fourteen
   private body files have been relocated, attached and bound. Caller must keep this owner at a stable address while visual pointers refer
   to its inline file/view fields. Close visuals before this owner; keep skeletons alive while binding. */
int nflstreet_match_bodies_bind(nflstreet_match_visuals *,
    const nflstreet_match_skeletons *, const uint8_t *raw_plamodel, size_t,
    const nflstreet_match_body_input[NFLSTREET_MATCH_PLAYERS],
    nflstreet_match_bodies *);
void nflstreet_match_bodies_close(nflstreet_match_bodies *);
#endif
