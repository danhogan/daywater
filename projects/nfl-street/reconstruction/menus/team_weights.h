#ifndef NFLSTREET_TEAM_WEIGHTS_H
#define NFLSTREET_TEAM_WEIGHTS_H
#include "team_catalog.h"
#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_TEAM_WEIGHTS_BASE_COUNT 10u
#define NFLSTREET_TEAM_WEIGHTS_QUERY_COUNT 12u
#define NFLSTREET_TEAM_WEIGHTS_MAX_PLAYERS 20u
#define NFLSTREET_TEAM_WEIGHTS_ROLE_COUNT 14u

/* Raw PLAY values queried by 8014F180 and 8014F60C. These names are the
   literal four-byte query tags; they are not assigned gameplay meanings. */
typedef struct {
 uint32_t team_id,player_id;
 uint32_t base_values[NFLSTREET_TEAM_WEIGHTS_BASE_COUNT];
 uint32_t query_ids[NFLSTREET_TEAM_WEIGHTS_QUERY_COUNT];
} nflstreet_team_weight_inputs;

/* Explicit response boundary for each 801B1B30 STAT query made by 8014F2B0.
   Logical response index 7 is normally skipped; if query_ids[6] is zero,
   original role/category 6 instead uses response index 7. Status 0x17
   contributes nothing; otherwise signed shorts accumulate with 16-bit wrap. */
typedef struct { uint32_t status; int16_t values[NFLSTREET_TEAM_WEIGHTS_BASE_COUNT]; } nflstreet_team_gear_response;

/* Result of a GEAR.GPKY=GRIP.GPKY join. found==0 means no unique joined row;
   this is not a query status. GRIP type-2 columns are decoded as signed
   width-9 integers, matching the original DB field decoder. */
typedef struct {
 int found;
 uint32_t gear_row,grip_row,gear_key;
 int16_t values[NFLSTREET_TEAM_WEIGHTS_BASE_COUNT];
} nflstreet_team_gear_lookup;

typedef struct {
 float floor_value,weight_threshold,scale,offset,clamp_low,clamp_high;
 double integer_bias,conversion_base;
 float role_anchor[NFLSTREET_TEAM_WEIGHTS_ROLE_COUNT];
 float role_multiplier[NFLSTREET_TEAM_WEIGHTS_ROLE_COUNT];
 float role_limit[NFLSTREET_TEAM_WEIGHTS_ROLE_COUNT];
 float coefficients[NFLSTREET_TEAM_WEIGHTS_ROLE_COUNT][NFLSTREET_TEAM_WEIGHTS_BASE_COUNT];
} nflstreet_team_rating_curve;

/* Finds one PLAY row by its actual DIGT/DIGP values and extracts exact query
   tags used by 8014F180 and 8014F60C. */
int nflstreet_team_weights_from_db(const uint8_t *db,size_t size,uint32_t team_id,
 uint32_t player_id,nflstreet_team_weight_inputs *out);

/* Looks up one GEAR row by raw GEID/GRTI values, then joins its GPKY to GRIP.
   This only extracts table data; it does not invent a STAT query status. */
int nflstreet_team_gear_lookup_from_template(const uint8_t *db,size_t size,
 uint32_t gear_id,uint32_t category,nflstreet_team_gear_lookup *out);

/* Bounded 8014F2B0/8014F504 arithmetic. Query responses stay explicit inputs;
   no gear/STAT DB lookup is claimed. */
int nflstreet_team_weights_build(const nflstreet_team_weight_inputs *inputs,
 const nflstreet_team_gear_response responses[12],uint32_t status_17,
 uint32_t ratings[10]);

/* Reads the raw 801E0D00 constants and 14x10 coefficient table from an
   original RAM capture. Curve members are raw coefficients/constants, not
   player-facing categories. */
int nflstreet_team_rating_curve_from_ram(const uint8_t *ram,size_t ram_size,
 uint32_t ram_base,nflstreet_team_rating_curve *out);

/* Bounded 801E0D00 formula. Inputs are the ten integers passed to the original
   routine after 8014FCCC's reorder; scores retain original unsigned encoding. */
int nflstreet_team_rating_score(const nflstreet_team_rating_curve *curve,
 uint32_t role,const uint32_t values[10],uint32_t clamp_mode,uint32_t *score);

/* Exact 8014FCCC reorder from the 8014F504 ten-value vector. */
void nflstreet_team_rating_reorder(const uint32_t values[10],uint32_t out[10]);

#endif
