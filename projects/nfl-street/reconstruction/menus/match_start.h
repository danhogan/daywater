#ifndef NFLSTREET_MATCH_START_H
#define NFLSTREET_MATCH_START_H
#include <stdint.h>

/* Semantic boundaries of original801EA480. The callback supplies unresolved
   query/resource/session services; this orchestration does not create actors. */
typedef enum {
    NFLSTREET_MATCH_REGISTER,
    NFLSTREET_MATCH_QUERY_MODE,
    NFLSTREET_MATCH_SETUP_A,
    NFLSTREET_MATCH_SETUP_B,
    NFLSTREET_MATCH_SETUP_PAIR,
    NFLSTREET_MATCH_SETUP_FINAL,
    NFLSTREET_MATCH_QUERY_SIDE,
    NFLSTREET_MATCH_SET_SIDE_A,
    NFLSTREET_MATCH_SET_SIDE_B,
    NFLSTREET_MATCH_QUERY_PLAYER,
    NFLSTREET_MATCH_SESSION,
    NFLSTREET_MATCH_UPDATE
} nflstreet_match_start_kind;
typedef struct {
    nflstreet_match_start_kind kind;
    uint32_t side, index, value, context, argument0, argument1;
    const uint32_t (*formations)[14];
} nflstreet_match_start_request;
/* Query callbacks mutate *output; player output starts at the input word.
   Return the original signed status. Registration status is ignored. Player
   query status is also ignored by the original caller. Services must be valid
   for this supplied context; zero is not a default for unresolved services. */
typedef int32_t (*nflstreet_match_start_service)(void *,
    const nflstreet_match_start_request *, uint32_t *output);
typedef struct {
    uint32_t formations[2][14], side_context[2];
    uint8_t mode;
    unsigned session_requested, update_requested;
    int32_t status;
} nflstreet_match_start_result;
/* Returns0 only for invalid API inputs, otherwise1 with original game status.
   Owns transformed copies; input selections are preserved. On a service error
   the result retains mutations made before the original immediate return. */
int nflstreet_match_start_run(const uint32_t formations[2][14],
    nflstreet_match_start_service, void *, nflstreet_match_start_result *);
#endif
