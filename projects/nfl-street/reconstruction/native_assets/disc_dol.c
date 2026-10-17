#include "disc_dol.h"
#include "dol_sections.h"
#include <nod.h>
#include <stdlib.h>
#include <string.h>
int nflstreet_disc_read_dol(const char *iso, uint8_t **bytes, size_t *size) {
    struct NodHandle *disc = NULL, *partition = NULL;
    struct NodPartitionMeta meta;
    uint8_t *copy = NULL;
    int ok = 0;
    if (!iso || !bytes || !size) return 0;
    if (nod_disc_open(iso, NULL, &disc) != NOD_RESULT_OK ||
        nod_disc_open_partition_kind(disc, NOD_PARTITION_KIND_DATA, NULL, &partition) != NOD_RESULT_OK ||
        nod_partition_meta(partition, &meta) != NOD_RESULT_OK) goto done;
    if (!meta.raw_dol.data || !nflstreet_dol_verify_revision(meta.raw_dol.data, meta.raw_dol.size)) goto done;
    copy = malloc(meta.raw_dol.size);
    if (!copy) goto done;
    memcpy(copy, meta.raw_dol.data, meta.raw_dol.size);
    *bytes = copy; *size = meta.raw_dol.size; ok = 1;
done:
    if (partition) nod_free(partition);
    if (disc) nod_free(disc);
    return ok;
}
