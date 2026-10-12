#ifndef NFLSTREET_TEAM_PLAYER_WINDOW_H
#define NFLSTREET_TEAM_PLAYER_WINDOW_H
#include <stdint.h>

#define NFLSTREET_TEAM_PLAYER_WINDOW_SIDES 2u
#define NFLSTREET_TEAM_PLAYER_WINDOW_ROSTER_MAX 20u
#define NFLSTREET_TEAM_PLAYER_WINDOW_EFFECTS 1u

typedef struct {uint32_t id;int32_t slot;uint32_t auxiliary;} nflstreet_team_player_window_row;
typedef struct {
 uint32_t count,query_count;
 int32_t cursor,mode;
 nflstreet_team_player_window_row rows[NFLSTREET_TEAM_PLAYER_WINDOW_ROSTER_MAX];
 uint32_t query_ids[NFLSTREET_TEAM_PLAYER_WINDOW_ROSTER_MAX];
 uint32_t timer_word,timer_output;
 uint32_t visibility_count,visibility_index;
} nflstreet_team_player_window_side;
typedef struct {uint32_t id,slot;} nflstreet_team_player_window_pair;
typedef struct {
 nflstreet_team_player_window_side sides[NFLSTREET_TEAM_PLAYER_WINDOW_SIDES];
} nflstreet_team_player_window;

/* Caller-supplied roster triples, PLAY-ID order, and timer word. Getter/query
  service 8014E760 is supplied; the original D6F8/D988 callbacks are compared. */
int nflstreet_team_player_window_callback_f(const nflstreet_team_player_window *,unsigned,
 nflstreet_team_player_window_pair out[3],uint32_t *mode_out);
int nflstreet_team_player_window_callback_10(nflstreet_team_player_window *,unsigned,
 int32_t delta,nflstreet_team_player_window_pair *out);

#endif
