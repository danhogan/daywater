#ifndef NFLSTREET_GAME_ROSTER_H
#define NFLSTREET_GAME_ROSTER_H
#include "../menus/team_catalog.h"
#define NFLSTREET_GAME_TEAMS_MAX 2u
/* Schema of one table of a version-8 DB block, including empty tables that
   store no records (template 00001 is the empty GAME layout): record
   bytes/bits, allocated rows and field descriptors; used stays 0. */
int nflstreet_db_schema_open(const uint8_t *,size_t,uint32_t tag,nflstreet_team_table *);
/* One caller-supplied STAT PLAY field value applied before the copy (the
   menu writes GILP in STAT before match start; that writer is not traced). */
typedef struct {uint32_t digp,tag,value;} nflstreet_stat_override;
/* The match GAME PLAY table built by original 801E05A4 -> 801E1628/801E78C4
   for an empty GAME database. For each selected team in MTTS UNLS order, team
   slot u gets GAME DIGT u. New DIGPs are handed out 0,1,2... (801E7154) the
   first time a source DIGP is copied. The per-team copy visits the team's STAT
   THCD rows and then its PLAY rows, each in ascending source DIGP, so players
   without a THCD row are numbered after the rest of their team. The PLAY rows
   are stored in ascending source DIGP, converted to the GAME schema by field
   tag (STAT-only fields such as LILP drop), with the new DIGP and DIGT = u.
   All other fields, including ITGT, DIOP and GILP, are copied. The table view
   owns field descriptors and records; source_rows maps GAME rows to STAT rows. */
typedef struct {
 uint8_t *bytes;nflstreet_team_table table;unsigned *source_rows;
 unsigned team_rows[NFLSTREET_GAME_TEAMS_MAX];
} nflstreet_game_roster;
/* Two selected teams are supported (the verified case). Rejects a team with no STAT TEAM row, a PYTT 5 (created) team (skipped by
   the original loader and imported elsewhere), a GAME field without a STAT field
   of equal type/width, an override for a row that is not copied, or more rows
   than the GAME table allocates. */
int nflstreet_game_roster_build(const nflstreet_team_table *stat_play,const nflstreet_team_table *stat_thcd,const nflstreet_team_table *stat_teams,
 const nflstreet_team_table *game_schema,const uint32_t *teams,unsigned team_count,
 const nflstreet_stat_override *,unsigned override_count,nflstreet_game_roster *);
void nflstreet_game_roster_close(nflstreet_game_roster *);
#endif
