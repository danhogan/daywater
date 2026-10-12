#ifndef NFLSTREET_FACE_RESOURCE_PAIR_H
#define NFLSTREET_FACE_RESOURCE_PAIR_H
#include "archive.h"
#include "model_view.h"
typedef struct {
    unsigned indices[2];
    uint8_t *bytes[2];
    size_t sizes[2];
    uint32_t tokens[2];
    uint8_t views[2][NFLSTREET_MODEL_VIEW_BYTES];
} nflstreet_face_resource_pair;
/* Own two adjacent decoded face models and their processed model views.
   Index is supplied by the separately verified selected-player resolver.
   Cache policy and original filesystem ownership remain external. */
int nflstreet_face_resource_pair_open(const nflstreet_archive *,unsigned index,
    uint32_t identity_base,nflstreet_face_resource_pair *);
void nflstreet_face_resource_pair_close(nflstreet_face_resource_pair *);
#endif
