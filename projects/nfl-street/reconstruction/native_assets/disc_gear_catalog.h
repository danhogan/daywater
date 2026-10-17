#ifndef NFLSTREET_DISC_GEAR_CATALOG_H
#define NFLSTREET_DISC_GEAR_CATALOG_H
#include "gear_texture.h"
#define NFLSTREET_DISC_GEAR_CATALOG_BYTES (16u+0x868u+48u+104u+32u+25u*4u+360u+60u+13u*4u)
/* Host composition of immutable DOL constants in the existing GTC4 format.
   No original startup execution. Empty, zero-initialized output required.
   Bytes and catalog views are owned until close; DOL/disc inputs are borrowed
   only during open. Failure leaves output untouched. Do not shallow-copy owners. */
typedef struct {
 uint8_t *bytes;size_t size;
 nflstreet_gear_texture_catalog catalog;
} nflstreet_disc_gear_catalog;
int nflstreet_disc_gear_catalog_open_dol(const uint8_t *,size_t,nflstreet_disc_gear_catalog *);
int nflstreet_disc_gear_catalog_open_disc(const char *,nflstreet_disc_gear_catalog *);
void nflstreet_disc_gear_catalog_close(nflstreet_disc_gear_catalog *);
#endif
