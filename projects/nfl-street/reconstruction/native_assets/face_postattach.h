#ifndef NFLSTREET_FACE_POSTATTACH_H
#define NFLSTREET_FACE_POSTATTACH_H
#include "face_resource_pair.h"
#include "skeleton_resource.h"
#include "model_postattach.h"
/* Bind owned face files/views to a loaded SKL1 target. View identities and
   allocator storage must share a disjoint caller namespace. Failure preserves
   earlier file mutations; pair/target allocation ownership is unchanged. */
int nflstreet_face_resource_pair_bind(nflstreet_face_resource_pair *,
    const nflstreet_skeleton_resource *,uint32_t view_identity_base,
    nflstreet_model_postattach_allocate,nflstreet_model_postattach_release,void *);
#endif
