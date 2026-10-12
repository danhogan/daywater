#ifndef NFLSTREET_TEAM_PLAYER_RESOLVE_H
#define NFLSTREET_TEAM_PLAYER_RESOLVE_H
#include "team_player_manifest.h"
#include "../native_assets/archive.h"
/* Fresh archive HSH1 owner. Borrowed views; no original module or capture. */
typedef struct {nflstreet_archive face,hair;} nflstreet_team_player_resolver;
int nflstreet_team_player_resolver_open(const uint8_t*,size_t,const uint8_t*,size_t,nflstreet_team_player_resolver*);
int nflstreet_team_player_resolve(const nflstreet_team_player_resolver*,const uint8_t*,size_t,uint32_t,uint32_t,nflstreet_team_player_manifest*);
#endif
