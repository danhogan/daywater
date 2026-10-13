#ifndef NFLSTREET_ATTACHMENT_PALETTE_SCALE_H
#define NFLSTREET_ATTACHMENT_PALETTE_SCALE_H
#include <stddef.h>
#include <stdint.h>
/* Explicit original 80162760 model slots, not inferred appearance roles. */
typedef enum {NFLSTREET_SCALE_SLOT_5A8,NFLSTREET_SCALE_SLOT_788,
 NFLSTREET_SCALE_SLOT_968,NFLSTREET_SCALE_SLOT_698} nflstreet_attachment_scale_slot;
/* Mirror the caller's two distinct matrix areas: slot788 scales staged matrices
   then copies scratch over them; other slots scale scratch. Caller supplies
   Clst rows, LOD and scale; count1..255 and disjoint arrays required. Slot5A8
   skips the copy as well as scaling at LOD>=2. Commit both areas only after validation. */
int nflstreet_attachment_palette_scale(const uint8_t *rows,size_t rows_size,
 unsigned count,nflstreet_attachment_scale_slot,unsigned lod,float scale,
 float scratch[256][12],float staged[256][12]);
#endif
