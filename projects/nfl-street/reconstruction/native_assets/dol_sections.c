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
    return sha1_equals(dol, dol_size, EXPECTED_DOL_SHA1);
}

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

int nflstreet_dol_parse_header(const uint8_t *dol, size_t dol_size, nflstreet_dol_header *header) {
    if (dol_size < 256) return 0;
    
    /* Text section offsets (0x00), addresses (0x48), sizes (0x90) */
    /* Data section offsets (0xD8), addresses (0x120), sizes (0x168) */
    /* BSS address (0x1B0), BSS size (0x1B4), entry point (0x1B8) */
    
    memset(header, 0, sizeof(*header));
    
    /* Parse text sections */
    for (int i = 0; i < NFLSTREET_DOL_TEXT_SECTIONS; ++i) {
        uint32_t off = read_be32(dol + 0x00 + i * 4);
        uint32_t addr = read_be32(dol + 0x48 + i * 4);
        uint32_t size = read_be32(dol + 0x90 + i * 4);
        header->text[i].offset = off;
        header->text[i].address = addr;
        header->text[i].size = size;
    }
    
    /* Parse data sections */
    for (int i = 0; i < NFLSTREET_DOL_DATA_SECTIONS; ++i) {
        uint32_t off = read_be32(dol + 0xD8 + i * 4);
        uint32_t addr = read_be32(dol + 0x120 + i * 4);
        uint32_t size = read_be32(dol + 0x168 + i * 4);
        header->data[i].offset = off;
        header->data[i].address = addr;
        header->data[i].size = size;
    }
    
    header->bss_address = read_be32(dol + 0x1B0);
    header->bss_size = read_be32(dol + 0x1B4);
    header->entry_point = read_be32(dol + 0x1B8);
    
    /* Validate all sections */
    for (int i = 0; i < NFLSTREET_DOL_TEXT_SECTIONS; ++i) {
        if (header->text[i].size == 0) continue;
        if (header->text[i].offset < 256) return 0;
        if (header->text[i].offset > dol_size) return 0;
        if (header->text[i].size > dol_size - header->text[i].offset) return 0;
        if (header->text[i].address % 4 != 0) return 0;
        if (header->text[i].size % 4 != 0) return 0;
    }
    
    for (int i = 0; i < NFLSTREET_DOL_DATA_SECTIONS; ++i) {
        if (header->data[i].size == 0) continue;
        if (header->data[i].offset < 256) return 0;
        if (header->data[i].offset > dol_size) return 0;
        if (header->data[i].size > dol_size - header->data[i].offset) return 0;
        if (header->data[i].address % 4 != 0) return 0;
        if (header->data[i].size % 4 != 0) return 0;
    }
    
    if (header->bss_address % 4 != 0) return 0;
    if (header->bss_size % 4 != 0) return 0;
    
    /* Check for overlapping sections */
    for (int i = 0; i < NFLSTREET_DOL_TEXT_SECTIONS; ++i) {
        if (header->text[i].size == 0) continue;
        uint32_t start_i = header->text[i].address;
        uint32_t end_i = start_i + header->text[i].size;
        for (int j = i + 1; j < NFLSTREET_DOL_TEXT_SECTIONS; ++j) {
            if (header->text[j].size == 0) continue;
            uint32_t start_j = header->text[j].address;
            uint32_t end_j = start_j + header->text[j].size;
            if (start_i < end_j && start_j < end_i) return 0;
        }
        for (int j = 0; j < NFLSTREET_DOL_DATA_SECTIONS; ++j) {
            if (header->data[j].size == 0) continue;
            uint32_t start_j = header->data[j].address;
            uint32_t end_j = start_j + header->data[j].size;
            if (start_i < end_j && start_j < end_i) return 0;
        }
    }
    
    for (int i = 0; i < NFLSTREET_DOL_DATA_SECTIONS; ++i) {
        if (header->data[i].size == 0) continue;
        uint32_t start_i = header->data[i].address;
        uint32_t end_i = start_i + header->data[i].size;
        for (int j = i + 1; j < NFLSTREET_DOL_DATA_SECTIONS; ++j) {
            if (header->data[j].size == 0) continue;
            uint32_t start_j = header->data[j].address;
            uint32_t end_j = start_j + header->data[j].size;
            if (start_i < end_j && start_j < end_i) return 0;
        }
    }
    
    /* Check BSS doesn't overlap loaded sections */
    if (header->bss_size > 0) {
        uint32_t bss_start = header->bss_address;
        uint32_t bss_end = bss_start + header->bss_size;
        for (int i = 0; i < NFLSTREET_DOL_TEXT_SECTIONS; ++i) {
            if (header->text[i].size == 0) continue;
            uint32_t start_i = header->text[i].address;
            uint32_t end_i = start_i + header->text[i].size;
            if (bss_start < end_i && start_i < bss_end) return 0;
        }
        for (int i = 0; i < NFLSTREET_DOL_DATA_SECTIONS; ++i) {
            if (header->data[i].size == 0) continue;
            uint32_t start_i = header->data[i].address;
            uint32_t end_i = start_i + header->data[i].size;
            if (bss_start < end_i && start_i < bss_end) return 0;
        }
    }
    
    return 1;
}

int nflstreet_dol_load_sections(const uint8_t *dol, size_t dol_size,
                                 const nflstreet_dol_header *header,
                                 uint8_t *target, size_t target_size,
                                 uint32_t target_base) {
    /* Load text sections */
    for (int i = 0; i < NFLSTREET_DOL_TEXT_SECTIONS; ++i) {
        if (header->text[i].size == 0) continue;
        uint32_t addr = header->text[i].address;
        uint32_t off = header->text[i].offset;
        uint32_t size = header->text[i].size;
        if (addr < target_base) return 0;
        if (addr - target_base > target_size) return 0;
        if (size > target_size - (addr - target_base)) return 0;
        memcpy(target + (addr - target_base), dol + off, size);
    }
    
    /* Load data sections */
    for (int i = 0; i < NFLSTREET_DOL_DATA_SECTIONS; ++i) {
        if (header->data[i].size == 0) continue;
        uint32_t addr = header->data[i].address;
        uint32_t off = header->data[i].offset;
        uint32_t size = header->data[i].size;
        if (addr < target_base) return 0;
        if (addr - target_base > target_size) return 0;
        if (size > target_size - (addr - target_base)) return 0;
        memcpy(target + (addr - target_base), dol + off, size);
    }
    
    /* Zero BSS */
    if (header->bss_size > 0) {
        uint32_t addr = header->bss_address;
        uint32_t size = header->bss_size;
        if (addr < target_base) return 0;
        if (addr - target_base > target_size) return 0;
        if (size > target_size - (addr - target_base)) return 0;
        memset(target + (addr - target_base), 0, size);
    }
    
    return 1;
}