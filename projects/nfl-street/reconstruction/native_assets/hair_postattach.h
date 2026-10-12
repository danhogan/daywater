#ifndef NFLSTREET_HAIR_POSTATTACH_H
#define NFLSTREET_HAIR_POSTATTACH_H
#include "hair_resources.h"
#include "skeleton_resource.h"
#include "model_postattach.h"
/* Bind three owned models to a caller-owned SKL1 target. Missing slots are
   unchanged. View/file/target/allocator identities must be disjoint. Failure
   can retain earlier mutations; ownership is unchanged. */
int nflstreet_hair_resources_bind(nflstreet_hair_resources *,const nflstreet_skeleton_resource *,
 uint32_t view_identity,nflstreet_model_postattach_allocate,nflstreet_model_postattach_release,void *);
#endif
