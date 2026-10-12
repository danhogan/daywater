#ifndef NFLSTREET_FIELD_AVAILABILITY_H
#define NFLSTREET_FIELD_AVAILABILITY_H

#include <stddef.h>
#include <stdint.h>

/* Reads the actual IVID table and resolves OCID 0/1 through DIGD to IFID.
   This models only the no-profile C9 predicate used by 8011F1F0. */
int nflstreet_field_availability_from_ivid(const uint8_t *bytes, size_t size,
                                           int8_t raw_field_id, uint8_t *c9,
                                           uint8_t ifid_by_ocid[2]);

#endif
