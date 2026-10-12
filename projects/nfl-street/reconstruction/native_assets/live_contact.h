#ifndef NFLSTREET_LIVE_CONTACT_H
#define NFLSTREET_LIVE_CONTACT_H
#include <stddef.h>
#include <stdint.h>
/* Bounded source-backed inspection contact, not natural gameplay eligibility. */
typedef struct {
 uint8_t actor[2][0x14d4],manager[256],globals[8],stream[2][64];
 uint8_t program[2][4096];size_t program_size[2];uint32_t flags[2];
 uint16_t clips[2];unsigned slots[2];unsigned done[2],active,updates,callbacks,completions;
} nflstreet_live_contact;
int nflstreet_live_contact_open(nflstreet_live_contact *,const char *,const char *,const uint8_t *,size_t);
/* Returns two frame-index pairs and 12-bit fractions in output[i][0..2].
   It advances only the selected finite streams and their checked flag events. */
int nflstreet_live_contact_step(nflstreet_live_contact *,double,int16_t [2][384],unsigned *);
#endif
