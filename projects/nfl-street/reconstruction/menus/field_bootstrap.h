#ifndef NFLSTREET_FIELD_BOOTSTRAP_H
#define NFLSTREET_FIELD_BOOTSTRAP_H
#include "field_catalog.h"

typedef struct {
    /* UI slot -> raw DIVE key, from original DOL r13-6E10. */
    uint8_t keys[8];
    uint8_t records[8 * NFLSTREET_FIELD_CACHE_RECORD_SIZE];
} nflstreet_field_bootstrap;

/* Map original DOL sections, read its eight-slot order and key0 description,
   then reorder caller-owned catalog records. Catalog text42/C9 remain explicit
   inputs; original profile/unlock/query/cache-miss execution is not modeled.
   Output is unchanged on failure. */
int nflstreet_field_bootstrap_from_dol(const uint8_t *, size_t,
    const nflstreet_field_catalog_entry catalog[8], nflstreet_field_bootstrap *);
#endif
