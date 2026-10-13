#ifndef NFLSTREET_MATCH_ATTACHMENT_PALETTES_H
#define NFLSTREET_MATCH_ATTACHMENT_PALETTES_H
#include "match_body_palettes.h"
#include "attachment_palette_scale.h"
#define NFLSTREET_MATCH_ATTACHMENT_PARTS 5u
/* Parts are the two owned face files followed by three owned hair files.
   The caller explicitly supplies original scale slot, LOD, scale and root/pose
   for each part. No automatic role mapping or gameplay pose selection. */
typedef struct {nflstreet_match_body_pose pose;nflstreet_attachment_scale_slot slot;
 unsigned lod;float scale;} nflstreet_match_attachment_pose;
typedef struct {float matrices[14][5][256][12];unsigned counts[14][5];} nflstreet_match_attachment_palettes;
int nflstreet_match_attachment_palettes_build(const nflstreet_match_visuals *,
 const nflstreet_match_attachment_pose[14][5],const float polynomial[6],
 nflstreet_match_attachment_palettes *);
#endif
