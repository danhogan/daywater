#ifndef NFLSTREET_HEADWEAR_STATE_H
#define NFLSTREET_HEADWEAR_STATE_H
#include "../menus/team_catalog.h"
typedef struct {nflstreet_team_table table;unsigned id_field,type_field,state_field;} nflstreet_headwear_catalog;
typedef struct {
 uint32_t play_row,gear9_id,gear0_id,gear9_value,gear0_value,state;
 uint8_t queried_gear0;
} nflstreet_headwear_selection;
/* Borrow the original GEAR table. Keys and values remain opaque; no inferred
   gear labels. Missing/duplicate joins reject instead of emulating EOF state. */
int nflstreet_headwear_catalog_open(const uint8_t *,size_t,nflstreet_headwear_catalog *);
int nflstreet_headwear_resolve(const nflstreet_headwear_catalog *,const uint8_t *,size_t,
 uint32_t team,uint32_t player,nflstreet_headwear_selection *);
uint32_t nflstreet_headwear_map(uint32_t);
#endif
