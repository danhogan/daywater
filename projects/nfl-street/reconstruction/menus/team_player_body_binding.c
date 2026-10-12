#include "team_player_body_binding.h"

#include <stdlib.h>
#include <string.h>

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}
static unsigned be16(const uint8_t *p) { return (unsigned)p[0] << 8 | p[1]; }
static int span(size_t size, size_t offset, size_t length) {
    return offset <= size && length <= size - offset;
}

static int model_info(const uint8_t *b, size_t size, char out[128]) {
    if (!b || size < 16 || memcmp(b, "TMdl", 4) || be32(b + 4) > size || b[15]) return 0;
    unsigned count = be16(b + 8);
    size_t directory = be16(b + 10);
    if (count > 4096 || !span(size, directory, (size_t)count * 16)) return 0;
    const uint8_t *found = NULL;
    size_t found_size = 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *row = b + directory + (size_t)i * 16;
        if (memcmp(row, "Info", 4)) continue;
        if (found) return 0;
        size_t offset = be32(row + 4), length = be32(row + 8);
        if (!length || !span(size, offset, length)) return 0;
        found = b + offset;
        found_size = length;
    }
    if (!found) return 0;
    size_t n = 0;
    while (n < found_size && n < 127 && found[n]) ++n;
    if (!n || n == found_size || n == 127) return 0;
    memcpy(out, found, n);
    out[n] = 0;
    return 1;
}

int nflstreet_team_body_binding_open(const uint8_t *archive_bytes,
                                     size_t archive_size,
                                     uint32_t tbyp,
                                     uint32_t variant,
                                     nflstreet_team_body_binding *out) {
    if (!archive_bytes || !out || variant > 2) return 0;
    uint32_t key = tbyp == 7 ? 2 : tbyp;
    if (key > 2) return 0;
    nflstreet_archive archive;
    nflstreet_archive_entry entry;
    if (!nflstreet_archive_open(archive_bytes, archive_size, &archive)) return 0;
    unsigned index = 1 + key * 3 + variant;
    if (!nflstreet_archive_get(&archive, index, &entry)) return 0;
    uint8_t *decoded = NULL;
    size_t decoded_size = 0;
    if (!nflstreet_archive_decode(&entry, &decoded, &decoded_size)) return 0;
    nflstreet_team_body_binding result = {0};
    result.tbyp = tbyp;
    result.pool_key = key;
    result.variant = variant;
    result.archive_index = index;
    result.bytes = decoded;
    result.size = decoded_size;
    if (!model_info(decoded, decoded_size, result.info_name)) {
        nflstreet_team_body_binding_close(&result);
        return 0;
    }
    *out = result;
    return 1;
}

void nflstreet_team_body_binding_close(nflstreet_team_body_binding *binding) {
    if (!binding) return;
    free(binding->bytes);
    memset(binding, 0, sizeof *binding);
}
