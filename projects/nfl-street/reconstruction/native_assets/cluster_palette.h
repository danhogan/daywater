#ifndef NFLSTREET_CLUSTER_PALETTE_H
#define NFLSTREET_CLUSTER_PALETTE_H
#include "model_view.h"
#include <stddef.h>
#include <stdint.h>

/* Build the processed TMdl weighted-cluster palette using the original game's
   paired polynomial and matrix operation order. Optional pose angles are in
   model skeleton-row order. Outputs are committed only after the complete palette
   has been validated. */
int nflstreet_cluster_palette(const nflstreet_model_view_source *,
    const uint8_t view[NFLSTREET_MODEL_VIEW_BYTES], const int16_t *pose,
    size_t pose_count, const float root[12], const float polynomial[6],
    float out[256][12], unsigned *count);
/* The palette arithmetic on resolved big-endian tables: per bone 16-byte
   translations and three signed-short angles, per cluster {bone, parent,
   16-bit weight}. Matrix 0 is the root. Optional pose angles are indexed
   by bone (three per bone). Output is written only on success. */
int nflstreet_cluster_palette_rows(const uint8_t *translations,
    const uint8_t *rotations, const uint8_t *rows, unsigned bones,
    unsigned clusters, const int16_t *pose, const float root[12],
    const float polynomial[6], float out[][12]);
#endif
