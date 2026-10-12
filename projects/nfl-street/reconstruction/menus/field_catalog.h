#ifndef NFLSTREET_FIELD_CATALOG_H
#define NFLSTREET_FIELD_CATALOG_H

#include "field_cache.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    /* Raw numeric fields from RVNE, retained by their four-byte tags. */
    uint32_t DIVE, LTVE, ROVE, SDVE, TOVE;
    uint8_t record[NFLSTREET_FIELD_CACHE_RECORD_SIZE];
} nflstreet_field_catalog_entry;

/* Read all eight unique DIVE-keyed records from the caller's RVNE table.
   text42 and value_c9 are explicit host inputs because this reader does not
   model the runtime-formatted field value or profile/unlock query. Output is
   atomic on failure and is sorted by raw DIVE key 0..7. */
int nflstreet_field_catalog_from_template(const uint8_t *template_bytes,
                                          size_t template_size,
                                          const char text42[8][33],
                                          const uint8_t value_c9[8],
                                          nflstreet_field_catalog_entry output[8]);

#endif
