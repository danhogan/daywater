#ifndef NFLSTREET_MATCH_PLAYER_QUERY_H
#define NFLSTREET_MATCH_PLAYER_QUERY_H
#include <stddef.h>
#include <stdint.h>
/* Resolve the literal PLAY predicate consumed by original801EA480:
   DIGT==supplied side context and DIOP==input word, returning DIGP.
   Does not establish where the original side-context query obtains its value.
   Returns1 for one match,0 for malformed/missing/ambiguous data; output is
   unchanged on rejection. No original query-engine status is fabricated. */
int nflstreet_match_player_query(const uint8_t *, size_t,
    uint32_t side_context, uint32_t input_word, uint32_t *player_id);
#endif
