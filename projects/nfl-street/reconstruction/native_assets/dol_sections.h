#ifndef NFLSTREET_DOL_SECTIONS_H
#define NFLSTREET_DOL_SECTIONS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define NFLSTREET_DOL_TEXT_SECTIONS 7
#define NFLSTREET_DOL_DATA_SECTIONS 11
typedef struct { uint32_t offset, address, size; } nflstreet_dol_section;
typedef struct {
    nflstreet_dol_section text[7], data[11];
    uint32_t bss_address, bss_size, entry_point;
} nflstreet_dol_header;
/* Structural parsing only; does not enforce game revision. Validates source ranges,
   guest address wrap and pairwise file/guest section overlap. BSS may envelope
   initialized sections (as in the pinned game); byte sizes need not be aligned.
   Output untouched on failure. */
int nflstreet_dol_parse_header(const uint8_t *, size_t, nflstreet_dol_header *);
/* Host preparation, NOT reconstructed original boot: validate all descriptors
   before writes, zero BSS, then copy initialized sections over it. Revalidates
   caller-supplied header and target bounds. Failure leaves target unchanged.
   Source, descriptors and target must not alias. Revision pin is caller's job. */
int nflstreet_dol_load_sections(const uint8_t *, size_t, const nflstreet_dol_header *, uint8_t *, size_t, uint32_t);
/* Whole-image SHA1 pinned to GNNE69 rev0. */
int nflstreet_dol_verify_revision(const uint8_t *, size_t);
#ifdef __cplusplus
}
#endif
#endif
