#ifndef NFLSTREET_DISC_DOL_H
#define NFLSTREET_DISC_DOL_H
#include <stddef.h>
#include <stdint.h>
/* Host nod metadata route, outside FST. Caller frees returned bytes.
   Rejects any main.dol other than pinned GNNE69 rev0. Outputs untouched on failure. */
int nflstreet_disc_read_dol(const char *iso, uint8_t **bytes, size_t *size);
#endif
