#ifndef NFLSTREET_DISC_SLOT_MATERIALS_H
#define NFLSTREET_DISC_SLOT_MATERIALS_H
#include "slot_materials.h"
/* Host composition, not original boot execution. Zero-initialize before use.
   bytes owns every catalog view; source DOL/disc can be released immediately.
   Nonempty outputs reject; every failure leaves output untouched. Destroy once
   consumers and any borrowed material-name pointers are finished. */
typedef struct {
 uint8_t *bytes;size_t size;nflstreet_slot_material_catalog catalog;
} nflstreet_disc_slot_materials;
int nflstreet_disc_slot_materials_from_dol(const uint8_t *,size_t,nflstreet_disc_slot_materials *);
int nflstreet_disc_slot_materials_from_disc(const char *,nflstreet_disc_slot_materials *);
void nflstreet_disc_slot_materials_destroy(nflstreet_disc_slot_materials *);
#endif
