#ifndef NFLSTREET_DOL_SECTIONS_H
#define NFLSTREET_DOL_SECTIONS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFLSTREET_DOL_TEXT_SECTIONS 7
#define NFLSTREET_DOL_DATA_SECTIONS 11

typedef struct {
    uint32_t offset;
    uint32_t address;
    uint32_t size;
} nflstreet_dol_section;

typedef struct {
    nflstreet_dol_section text[NFLSTREET_DOL_TEXT_SECTIONS];
    nflstreet_dol_section data[NFLSTREET_DOL_DATA_SECTIONS];
    uint32_t bss_address;
    uint32_t bss_size;
    uint32_t entry_point;
} nflstreet_dol_header;

/* Parse a DOL header from the first 256 bytes of a DOL image.
   Returns 1 on success, 0 on invalid/truncated header.
   On success, 'header' is filled with validated section descriptors.
   Performs bounds checks: sections must be within the DOL image,
   addresses must be 4-byte aligned, sizes must be 4-byte aligned,
   sections must not overlap, BSS must not overlap any loaded section. */
int nflstreet_dol_parse_header(const uint8_t *dol, size_t dol_size, nflstreet_dol_header *header);

/* Load all DOL sections into a target memory buffer.
   'target' must be large enough to hold all sections at their linked addresses
   (typically the full 24 MiB MEM1 span 0x80000000..0x81800000).
   'target_base' is the guest base address (0x80000000 for MEM1).
   Returns 1 on success, 0 on overlap/out-of-bounds/truncated sections.
   Zeroes the BSS region on success. */
int nflstreet_dol_load_sections(const uint8_t *dol, size_t dol_size,
                                 const nflstreet_dol_header *header,
                                 uint8_t *target, size_t target_size,
                                 uint32_t target_base);

/* Validate that the DOL matches the expected GNNE69 rev 0 SHA-1.
   Returns 1 if the DOL SHA-1 matches the pinned baseline, 0 otherwise. */
int nflstreet_dol_verify_revision(const uint8_t *dol, size_t dol_size);

#ifdef __cplusplus
}
#endif

#endif /* NFLSTREET_DOL_SECTIONS_H */