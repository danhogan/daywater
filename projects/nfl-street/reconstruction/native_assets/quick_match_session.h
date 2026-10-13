#ifndef NFLSTREET_QUICK_MATCH_SESSION_H
#define NFLSTREET_QUICK_MATCH_SESSION_H
#include "quick_match_roster.h"
/* Native host ownership, not a reconstruction of original session allocation.
   Own a STAT copy so the roster survives teardown of its menu/database views.
   Selection, formations and field choice are retained; no actors or field load. */
typedef struct {
 uint8_t *stat_bytes;size_t stat_size;
 nflstreet_quick_match_roster roster;
} nflstreet_quick_match_session;
/* Output must be zero-initialized and empty. Failure preserves it. The GAME
   schema input is borrowed only during open; all retained views are owned. */
int nflstreet_quick_match_session_open(const uint8_t *,size_t,const uint8_t *,size_t,
 const nflstreet_quick_match_selection *,nflstreet_quick_match_session *);
void nflstreet_quick_match_session_close(nflstreet_quick_match_session *);
#endif
