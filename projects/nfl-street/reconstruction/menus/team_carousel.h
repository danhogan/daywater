#ifndef NFLSTREET_TEAM_CAROUSEL_H
#define NFLSTREET_TEAM_CAROUSEL_H
#include <stdint.h>

#define NFLSTREET_TEAM_CAROUSEL_SIDES 2u
#define NFLSTREET_TEAM_CAROUSEL_WINDOW 3u
#define NFLSTREET_TEAM_CAROUSEL_MAX_IDS 64u

/* Bounded local model for the data window edited by original8011CE64.
   IDs and predicate responses are supplied per side; this is not the game's
   team database or a claim about the meaning of the three selected fields. */
typedef struct {
 uint32_t id, d060_result;
} nflstreet_team_carousel_entry;

typedef struct {
 uint32_t count, cursor;
 nflstreet_team_carousel_entry entries[NFLSTREET_TEAM_CAROUSEL_MAX_IDS];
} nflstreet_team_carousel_catalog;

typedef struct {
 uint32_t windows[NFLSTREET_TEAM_CAROUSEL_SIDES][NFLSTREET_TEAM_CAROUSEL_WINDOW];
 nflstreet_team_carousel_catalog catalogs[NFLSTREET_TEAM_CAROUSEL_SIDES];
} nflstreet_team_carousel;

typedef struct {
 uint32_t values[NFLSTREET_TEAM_CAROUSEL_WINDOW];
 uint32_t flags[NFLSTREET_TEAM_CAROUSEL_WINDOW];
} nflstreet_team_carousel_output;

/* Models original operation1 (forward), operation3 (backward), and the
   general copy/check branch. Query services 80009E5C/6C are supplied false.
   The final cursor is the catalog position of output.values[2]. */
int nflstreet_team_carousel_apply(nflstreet_team_carousel *,uint32_t side,
                                  uint32_t operation,nflstreet_team_carousel_output *);

#endif
