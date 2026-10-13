#ifndef NFLSTREET_MATCH_VISUALS_H
#define NFLSTREET_MATCH_VISUALS_H
#include "quick_match_session.h"
#include "selected_player_visual.h"
#define NFLSTREET_MATCH_PLAYERS 14u
/* Host logical-token reservations, not original RAM addresses. Each player
   gets a disjoint namespace outside all borrowed body resources. */
typedef struct {
 nflstreet_selected_player_visual_context visual;
 uint32_t namespace_base,namespace_size;
 unsigned record_side; /* Explicit constructor input; no controller inference. */
} nflstreet_match_visual_input;
typedef struct {
 nflstreet_selected_player_visual players[NFLSTREET_MATCH_PLAYERS];
 unsigned count;
} nflstreet_match_visuals;
/* Load both selected lineups in side-major/menu order. Session source and all
   contexts' borrowed catalogs/archives/body pools must stay alive. Flushes are
   queued until every owner and namespace is checked; a failed real flush may
   have side effects, but failure still preserves output and releases owners. */
int nflstreet_match_visuals_open(const nflstreet_quick_match_session *,
 const nflstreet_match_visual_input[NFLSTREET_MATCH_PLAYERS],nflstreet_match_visuals *);
void nflstreet_match_visuals_close(nflstreet_match_visuals *);
#endif
