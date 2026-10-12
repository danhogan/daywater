#ifndef NFLSTREET_SKELETON_RESOURCE_H
#define NFLSTREET_SKELETON_RESOURCE_H
#include "archive.h"
typedef struct {uint8_t *bytes;size_t size;uint32_t token;unsigned archive_index;} nflstreet_skeleton_resource;
/* Own original8017EB24 tree/name relocation for raw SKL1 resources. This does
   not apply the additional game-specific edits made by80041108 and its helpers. */
int nflstreet_skeleton_relocate(uint8_t *,size_t,uint32_t token);
int nflstreet_skeleton_resource_open(const nflstreet_archive *,unsigned index,
    uint32_t token,nflstreet_skeleton_resource *);
void nflstreet_skeleton_resource_close(nflstreet_skeleton_resource *);
#endif
