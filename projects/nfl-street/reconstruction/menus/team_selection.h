#ifndef NFLSTREET_TEAM_SELECTION_H
#define NFLSTREET_TEAM_SELECTION_H
#include "team_catalog.h"
#include "team_carousel.h"
typedef struct {
 uint32_t id,sivt,fields[3]; /* Raw FORT, EDRT, VORT; meanings not inferred. */
 uint32_t logo_entry,raw_type,resource_fields; /* Original LGLT/PYTT readers; bitmask3 when present. */
 uint32_t name_bytes; /* Declared raw ANDT byte width, excluding host terminator. */
 char name[64];
} nflstreet_team_record;
typedef struct {
 unsigned count;
 nflstreet_team_record records[NFLSTREET_TEAM_CAROUSEL_MAX_IDS];
} nflstreet_team_catalog;
int nflstreet_team_catalog_append(nflstreet_team_catalog *,const nflstreet_team_table *);
/* Explicit ID-sorted local catalog and supplied center IDs. Original DB query
   initialization and profile filtering are outside this bounded interface. */
int nflstreet_team_selection_seed(const nflstreet_team_catalog *,const uint32_t centers[2],nflstreet_team_carousel *);
const nflstreet_team_record *nflstreet_team_catalog_find(const nflstreet_team_catalog *,uint32_t);
#endif
