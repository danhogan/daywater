#ifndef NFLSTREET_CLUSTER_PALETTE_H
#define NFLSTREET_CLUSTER_PALETTE_H
#include "model_view.h"
#include <stddef.h>
#include <stdint.h>

/* Build the processed TMdl weighted-cluster palette using the original game's
   paired polynomial and matrix operation order. Optional pose angles are in
   compact bone order. Outputs are committed only after the complete palette
   has been validated. */
int nflstreet_cluster_palette(const nflstreet_model_view_source *,
    const uint8_t view[NFLSTREET_MODEL_VIEW_BYTES], const int16_t *pose,
    size_t pose_count, const float root[12], const float polynomial[6],
    float out[256][12], unsigned *count);
#endif
