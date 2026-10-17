#include "dol_sections.h"
#include <string.h>
#include <stdint.h>

/* SHA-1 of GNNE69 rev 0 main.dol */
static const uint8_t EXPECTED_DOL_SHA1[20] = {
    0x46, 0xe3, 0xde, 0x85, 0x49, 0x98, 0xa0, 0xf6,
    0x6c, 0x9b, 0xe8, 0xc7, 0x2a, 0x11, 0x64, 0xd2,
    0x2c, 0xd2, 0x9e, 0xb4
};

/* Simple SHA-1 implementation for verification */
static void sha1_init(uint32_t h[5]) {
    h[0] = 0x67452301;
    h[1] = 0xEFCDAB89;
    h[2] = 0x98BADCFE;
    h[3] = 0x10325476;
    h[4] = 0xC3D2E1F0;
}

static uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static void sha1_process_block(uint32_t h[5], const uint8_t block[64]) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = ((uint32_t)block[i*4] << 24) |
               ((uint32_t)block[i*4+1] << 16) |
               ((uint32_t)block[i*4+2] << 8) |
               (uint32_t)block[i*4+3];
    }
    for (int i = 16; i < 80; ++i) {
        w[i] = rotl32(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t temp = rotl32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotl32(b, 30);
        b = a;
        a = temp;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

static int sha1_equals(const uint8_t *data, size_t len, const uint8_t expected[20]) {
    uint32_t h[5];
    sha1_init(h);
    size_t offset = 0;
    while (len >= 64) {
        sha1_process_block(h, data + offset);
        offset += 64;
        len -= 64;
    }
    uint8_t block[64] = {0};
    size_t remaining = len;
    memcpy(block, data + offset, remaining);
    block[remaining] = 0x80;
    if (remaining > 55) {
        sha1_process_block(h, block);
        memset(block, 0, 64);
    }
    uint64_t bit_len = (uint64_t)(offset + remaining) * 8;
    block[56] = (uint8_t)(bit_len >> 56);
    block[57] = (uint8_t)(bit_len >> 48);
    block[58] = (uint8_t)(bit_len >> 40);
    block[59] = (uint8_t)(bit_len >> 32);
    block[60] = (uint8_t)(bit_len >> 24);
    block[61] = (uint8_t)(bit_len >> 16);
    block[62] = (uint8_t)(bit_len >> 8);
    block[63] = (uint8_t)bit_len;
    sha1_process_block(h, block);
    uint8_t digest[20];
    for (int i = 0; i < 5; ++i) {
        digest[i*4] = (uint8_t)(h[i] >> 24);
        digest[i*4+1] = (uint8_t)(h[i] >> 16);
        digest[i*4+2] = (uint8_t)(h[i] >> 8);
        digest[i*4+3] = (uint8_t)h[i];
    }
    return memcmp(digest, expected, 20) == 0;
}

int nflstreet_dol_verify_revision(const uint8_t *dol, size_t dol_size) {
    return dol && sha1_equals(dol, dol_size, EXPECTED_DOL_SHA1);
}

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static const nflstreet_dol_section *section_at(const nflstreet_dol_header *h, unsigned i) {
    return i < 7 ? &h->text[i] : &h->data[i - 7];
}
static int valid_header(const nflstreet_dol_header *h, size_t dol_size) {
    if (dol_size < 256) return 0;
    if ((uint64_t)h->bss_address + h->bss_size > UINT64_C(0x100000000)) return 0;
    for (unsigned i = 0; i < 18; ++i) {
        const nflstreet_dol_section *a = section_at(h, i);
        if (!a->size) continue;
        if (a->offset < 256 || a->offset > dol_size || a->size > dol_size - a->offset) return 0;
        if ((uint64_t)a->address + a->size > UINT64_C(0x100000000)) return 0;
        for (unsigned j = 0; j < i; ++j) {
            const nflstreet_dol_section *b = section_at(h, j);
            if (!b->size) continue;
            if ((uint64_t)a->address < (uint64_t)b->address + b->size &&
                (uint64_t)b->address < (uint64_t)a->address + a->size) return 0;
            if ((uint64_t)a->offset < (uint64_t)b->offset + b->size &&
                (uint64_t)b->offset < (uint64_t)a->offset + a->size) return 0;
        }
    }
    return 1;
}
int nflstreet_dol_parse_header(const uint8_t *dol, size_t dol_size, nflstreet_dol_header *header) {
    if (!dol || !header || dol_size < 256) return 0;
    nflstreet_dol_header h = {0};
    for (unsigned i = 0; i < 18; ++i) {
        nflstreet_dol_section *a = i < 7 ? &h.text[i] : &h.data[i - 7];
        a->offset = read_be32(dol + i * 4);
        a->address = read_be32(dol + 0x48 + i * 4);
        a->size = read_be32(dol + 0x90 + i * 4);
    }
    h.bss_address = read_be32(dol + 0xd8);
    h.bss_size = read_be32(dol + 0xdc);
    h.entry_point = read_be32(dol + 0xe0);
    if (!valid_header(&h, dol_size)) return 0;
    *header = h;
    return 1;
}
static int fits(uint32_t address, uint32_t size, uint32_t base, size_t capacity) {
    return address >= base && (uint64_t)address - base <= capacity &&
        size <= capacity - ((uint64_t)address - base);
}
int nflstreet_dol_load_sections(const uint8_t *dol, size_t dol_size,
    const nflstreet_dol_header *header, uint8_t *target, size_t target_size, uint32_t target_base) {
    if (!dol || !header || !target || !valid_header(header, dol_size)) return 0;
    if (header->bss_size && !fits(header->bss_address, header->bss_size, target_base, target_size)) return 0;
    for (unsigned i = 0; i < 18; ++i) {
        const nflstreet_dol_section *a = section_at(header, i);
        if (a->size && !fits(a->address, a->size, target_base, target_size)) return 0;
    }
    /* Host preparation: the real DOL BSS envelope contains initialized sections.
       Zero its holes, then restore initialized bytes; this is not an original boot routine. */
    if (header->bss_size) memset(target + (header->bss_address - target_base), 0, header->bss_size);
    for (unsigned i = 0; i < 18; ++i) {
        const nflstreet_dol_section *a = section_at(header, i);
        if (a->size) memcpy(target + (a->address - target_base), dol + a->offset, a->size);
    }
    return 1;
}
