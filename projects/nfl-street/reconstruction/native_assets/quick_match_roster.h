#ifndef NFLSTREET_QUICK_MATCH_ROSTER_H
#define NFLSTREET_QUICK_MATCH_ROSTER_H
#include "game_roster.h"
#include "../menus/quick_match_selection.h"
/* Original 8014FD50 scalar operation; field read/write services are separate.
   side 0 modifies bit0; any nonzero side modifies bit1. */
uint32_t nflstreet_lineup_flag(uint32_t prior,unsigned side,int selected);
typedef struct {
 nflstreet_quick_match_selection selection;
 nflstreet_game_roster game;
 unsigned selected_rows[2][7],formation_rows[2][14];
 uint32_t selected_game_ids[2][7],formation_game_ids[2][14];
} nflstreet_quick_match_roster;
/* Consume native menu selections in STAT DIGT/DIGP namespaces. Each side's
   seven ids must belong to its selected STAT team; each formation half must be
   a permutation of those seven. Update only the original side's GILP bit on
   that team's rows, preserving other bits. Build the native GAME roster and
   map selected/formation order to its rows and new ids. No STAT writes.
   Mirrored teams are unverified and reject. Field slot/key are retained; field availability remains menu-owned.
   Source STAT DB must outlive this owner and remain unchanged. Output empty;
   failures preserve output and release staged allocations. No actor creation. */
int nflstreet_quick_match_roster_build(const uint8_t *stat_db,size_t,const nflstreet_team_table *game_schema,
 const nflstreet_quick_match_selection *,nflstreet_quick_match_roster *);
void nflstreet_quick_match_roster_close(nflstreet_quick_match_roster *);
#endif
