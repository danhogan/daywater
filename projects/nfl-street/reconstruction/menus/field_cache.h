#ifndef NFLSTREET_FIELD_CACHE_H
#define NFLSTREET_FIELD_CACHE_H

#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_FIELD_CACHE_RECORD_SIZE 205u
#define NFLSTREET_FIELD_CACHE_RECORD_COUNT 8u
#define NFLSTREET_FIELD_CACHE_NAME_SIZE 33u
#define NFLSTREET_FIELD_CACHE_SHORT_SIZE 102u

typedef struct {
    char text0[NFLSTREET_FIELD_CACHE_NAME_SIZE];
    char text21[NFLSTREET_FIELD_CACHE_NAME_SIZE];
    char text42[NFLSTREET_FIELD_CACHE_NAME_SIZE];
    char text99[NFLSTREET_FIELD_CACHE_SHORT_SIZE];
    uint8_t value_c9;
    uint32_t selected_index;
} nflstreet_field_cache_output;

/* Read the cached-record branch of original8011F1F0. Records and outputs are
   caller-owned. A return of 0 means invalid input or cache flag CA == 0;
   output remains untouched. Cache-flag rejection here is native API behavior;
   the original cache-miss construction path is outside this model. */
int nflstreet_field_cache_read(const uint8_t *records, size_t records_size,
                               uint32_t record_count, uint32_t index,
                               nflstreet_field_cache_output *output);

#endif
