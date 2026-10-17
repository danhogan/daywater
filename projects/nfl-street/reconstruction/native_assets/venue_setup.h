#ifndef NFLSTREET_VENUE_SETUP_H
#define NFLSTREET_VENUE_SETUP_H
#include <stddef.h>
#include <stdint.h>
/* Controlled scalar producer, 80026C34 + 80026CCC (GNNE69 rev 0).
   The caller owns a big-endian MEM1 span and the live FNIG query services.
   Reader IDs are original entry PCs, not invented weather semantics. Services
   execute synchronously, in original order; they may observe preceding writes.
   Constants 80245820/24 must already come from the matching DOL/data owner.
   No allocation, resource loading, selected-key publication, or FNIG ownership. */
typedef struct {
 void *user;
 uint32_t (*word)(void *user,uint32_t reader);
 double (*real)(void *user,uint32_t reader);
} nflstreet_venue_setup_services;
uint32_t nflstreet_venue_archive_base(uint32_t raw_key);
/* Returns 0 for an undersized/null span or missing services, before any write.
   The original void routine has no defined return value. Host success is 1. */
int nflstreet_venue_setup(uint8_t *mem1,size_t bytes,const nflstreet_venue_setup_services *services);
#endif
