#include "field_cache.h"

#include <stdint.h>
#include <string.h>

static void bounded_copy(char *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        dst[i] = (char)src[i];
        if (src[i] == 0) return;
    }
    dst[n - 1] = '\0';
}

int nflstreet_field_cache_read(const uint8_t *records, size_t records_size,
                               uint32_t record_count, uint32_t index,
                               nflstreet_field_cache_output *output) {
    if (!records || !output || !record_count ||
        record_count > NFLSTREET_FIELD_CACHE_RECORD_COUNT || index >= record_count ||
        records_size < (size_t)record_count * NFLSTREET_FIELD_CACHE_RECORD_SIZE)
        return 0;
    uintptr_t r = (uintptr_t)records;
    uintptr_t o = (uintptr_t)output;
    size_t out_size = sizeof *output;
    if ((r <= o && o - r < records_size) || (o < r && r - o < out_size))
        return 0;

    const uint8_t *record = records + (size_t)index * NFLSTREET_FIELD_CACHE_RECORD_SIZE;
    if (record[0xca] == 0) return 0;

    /* The original copy helper stops at the first NUL; bytes after it in each
       caller-owned destination remain whatever the caller had there. */
    nflstreet_field_cache_output next = *output;
    bounded_copy(next.text0, record, sizeof next.text0);
    bounded_copy(next.text21, record + 0x21, sizeof next.text21);
    bounded_copy(next.text42, record + 0x42, sizeof next.text42);
    bounded_copy(next.text99, record + 99, sizeof next.text99);
    next.value_c9 = record[0xc9];
    next.selected_index = index;
    *output = next;
    return 1;
}
