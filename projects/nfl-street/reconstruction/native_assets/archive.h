#ifndef NFLSTREET_ARCHIVE_H
#define NFLSTREET_ARCHIVE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const uint8_t *bytes;
    size_t size, directory, compression, data, data_size;
    uint16_t count;
} nflstreet_archive;
typedef struct {
    const uint8_t *bytes;
    size_t size;
    uint32_t codec, decoded_size;
} nflstreet_archive_entry;
/* Original TERF/DIR1 directory semantics, bounded host views. No ownership. */
int nflstreet_archive_open(const uint8_t *, size_t, nflstreet_archive *);
int nflstreet_archive_get(const nflstreet_archive *, unsigned, nflstreet_archive_entry *);
/* A compressed entry is never exposed as if it were decoded. */
int nflstreet_archive_plain(const nflstreet_archive_entry *, const uint8_t **, size_t *);
/* Allocate and decode an entry (codec 0 or 5). Caller frees *bytes.
   Failure leaves output pointers untouched. Maximum decoded size: 128 MiB. */
int nflstreet_archive_decode(const nflstreet_archive_entry *,uint8_t **,size_t *);
/* Original HSH1 algorithm-one uppercase polynomial hash; ambiguous hash
   matches and other hash encodings are unsupported. No XML-name substitution. */
int nflstreet_archive_find(const nflstreet_archive *,const char *,unsigned *);
#endif
