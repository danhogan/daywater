#ifndef NFLSTREET_MATCH_BODY_PALETTES_H
#define NFLSTREET_MATCH_BODY_PALETTES_H
#include "match_visuals.h"
#include "cluster_palette.h"
typedef struct {const int16_t *rotations;size_t rotation_count;float root[12];} nflstreet_match_body_pose;
typedef struct {float matrices[NFLSTREET_MATCH_PLAYERS][256][12];unsigned counts[NFLSTREET_MATCH_PLAYERS];} nflstreet_match_body_palettes;
/* Read each selected model's current owned/bound view and generate its body
   palette from explicit roots/angles/polynomial. No animation policy, actor
   flags, attachment scaling, palette token allocation or source writes. Whole
   output is committed only after all14 succeed. */
int nflstreet_match_body_palettes_build(const nflstreet_match_visuals *,
 const nflstreet_match_body_pose[NFLSTREET_MATCH_PLAYERS],const float polynomial[6],nflstreet_match_body_palettes *);
#endif
