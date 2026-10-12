/*
 * Calls untouched GNNE69 DOL functions 801A8088 -> 801A8B20 -> 801AA884
 * through a ModernGekko module. The HSH1 table and query TSV are supplied
 * privately at runtime; this checker does not print query strings.
 * Build on the local Linux ModernGekko host with its public module ABI header.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <moderngekko/module_abi.h>

#define RAM_SIZE (24u * 1024u * 1024u)
#define DESC 0x81700000u
#define DESC_HAIR 0x81700040u
#define ELEMENT 0x81700100u
#define ELEMENT_HAIR 0x81700140u
#define SEED 0x81700200u
#define SEED_HAIR 0x81700240u
#define HASHES 0x81701000u
#define HASHES_HAIR 0x81708000u
#define KEY 0x81710000u
#define STACK 0x817df000u
#define LOWER_TABLE 0x802612c9u
#define R13 0x803a9a40u

static unsigned calls_801a8088;

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}
static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static void put32(uint8_t *p, uint32_t x) {
    p[0] = (uint8_t)(x >> 24); p[1] = (uint8_t)(x >> 16);
    p[2] = (uint8_t)(x >> 8); p[3] = (uint8_t)x;
}
static void put16(uint8_t *p, uint16_t x) { p[0] = (uint8_t)(x >> 8); p[1] = (uint8_t)x; }
static uint8_t *at(uint8_t *ram, uint32_t address) {
    if (address < 0x80000000u || address - 0x80000000u >= RAM_SIZE) return NULL;
    return ram + address - 0x80000000u;
}
static int run(const ModernGekkoModuleDesc *m, CPUState *c, uint8_t *ram,
               uint32_t descriptor, const char *key, int32_t *out) {
    uint8_t *kp = at(ram, KEY);
    if (!kp || strlen(key) >= 256) return 0;
    memset(kp, 0, 256); memcpy(kp, key, strlen(key));
    memset(c, 0, sizeof(*c)); c->ram = ram; c->ram_size = RAM_SIZE;
    c->pc = 0x801a8088u; c->lr = 0; c->gpr[1] = STACK;
    calls_801a8088++;
    c->gpr[2] = 0x803b9a40u; c->gpr[13] = R13; c->gpr[3] = descriptor;
    c->gpr[4] = KEY; c->downcount = c->cycle_budget = 100000000;
    for (unsigned steps = 0; c->pc && steps < 100000; ++steps) {
        uint32_t pc = c->pc;
        /* The call uses the real DOL routine; these are only resource-pool
         * mutex and diagnostics boundaries absent from the isolated oracle. */
        if (pc == 0x801af944u || pc == 0x801af9ccu || pc == 0x801af57cu) {
            c->gpr[3] = 0; c->pc = c->lr;
        } else {
            c->downcount = c->cycle_budget = 100000000;
            m->dispatch(c, pc);
        }
        if (c->exception) return 0;
    }
    if (c->pc || c->gpr[1] != STACK) return 0;
    *out = (int32_t)c->gpr[3]; return 1;
}

static int read_file(const char *path, uint8_t **out, size_t *n) {
    FILE *f = fopen(path, "rb"); if (!f) return 0;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long z = ftell(f); if (z <= 0 || fseek(f, 0, SEEK_SET)) { fclose(f); return 0; }
    uint8_t *p = malloc((size_t)z); if (!p || fread(p, 1, (size_t)z, f) != (size_t)z) {
        free(p); fclose(f); return 0;
    }
    fclose(f); *out = p; *n = (size_t)z; return 1;
}
static int install_hsh(uint8_t *ram, const char *archive, uint32_t desc_addr,
                       uint32_t element_addr, uint32_t seed_addr,
                       uint32_t hashes_addr, unsigned *count_out) {
    uint8_t *data = NULL; size_t n = 0;
    if (!read_file(archive, &data, &n) || n < 12 || memcmp(data, "TERF", 4)) return 0;
    size_t p = be32(data + 4); uint16_t count = 0; uint32_t seed = 0; const uint8_t *hashes = NULL;
    while (p + 8 <= n) {
        uint32_t z = be32(data + p + 4); if (z < 8 || z > n - p) break;
        if (!memcmp(data + p, "HSH1", 4)) {
            if (z < 24 || hashes) { free(data); return 0; }
            /* TERF's archive-wide element count is in its header at 0x0e;
             * HSH1 stores the algorithm parameters and fixed-width hashes. */
            count = be16(data + 14); seed = be32(data + p + 20);
            if ((uint64_t)24 + (uint64_t)count * 4 > z) { free(data); return 0; }
            hashes = data + p + 24;
        }
        p += z;
    }
    if (!hashes || !count || hashes_addr - 0x80000000u + count * 4u > RAM_SIZE) { free(data); return 0; }
    uint8_t *d = at(ram, desc_addr), *e = at(ram, element_addr), *s = at(ram, seed_addr), *h = at(ram, hashes_addr);
    put16(d + 0x14, count); put32(d + 0x1c, 1); put32(d + 0x44, element_addr);
    put32(d + 0x48, seed_addr); put32(d + 0x4c, hashes_addr);
    put32(e, 1); put32(e + 8, 4); put32(s, seed);
    memcpy(h, hashes, (size_t)count * 4);
    free(data); *count_out = count; return 1;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: lookup-check <original-module.so> <plaface.dat> <plarosterhair.dat> <private-query.tsv>\n"); return 2;
    }
    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL); if (!lib) { fprintf(stderr, "dlopen failed\n"); return 3; }
    ModernGekkoGetModuleFn get = (ModernGekkoGetModuleFn)dlsym(lib, MODERNGEKKO_GET_MODULE_SYMBOL);
    if (!get) return 3;
    const ModernGekkoModuleDesc *m = get(); if (!m || m->cpu_state_size != sizeof(CPUState)) return 3;
    uint8_t *ram = calloc(1, RAM_SIZE); if (!ram) return 4;
    unsigned face_count = 0, hair_count = 0;
    if (!install_hsh(ram, argv[2], DESC, ELEMENT, SEED, HASHES, &face_count) ||
        !install_hsh(ram, argv[3], DESC_HAIR, ELEMENT_HAIR, SEED_HAIR, HASHES_HAIR, &hair_count)) return 4;
    *at(ram, R13 - 0x6538u) = 1; /* original resource system ready */
    uint8_t *lower = at(ram, LOWER_TABLE); if (!lower) return 4;
    for (unsigned ch = 'a'; ch <= 'z'; ++ch) lower[ch] = 2;
    FILE *f = fopen(argv[4], "rb"); if (!f) return 4;
    char *line = NULL; size_t cap = 0; ssize_t len; unsigned checks = 0, upper = 0, lower_checks = 0, misses = 0;
    CPUState c;
    while ((len = getline(&line, &cap, f)) >= 0) {
        if (!len || line[0] == '#') continue;
        char *tab = strchr(line, '\t'); if (!tab) { fprintf(stderr,"bad query columns\n"); return 5; } *tab++ = 0;
        char *key_start = strchr(tab, '\t'); if (!key_start) { fprintf(stderr,"bad query columns\n"); return 5; } *key_start++ = 0;
        char *end = strchr(key_start, '\t'); if (!end) { fprintf(stderr,"bad query columns\n"); return 5; } *end++ = 0;
        char *nl = strpbrk(end, "\r\n"); if (nl) *nl = 0;
        char *expected_end = NULL; long expected = strtol(end, &expected_end, 10);
        unsigned hash_count = !strcmp(line, "face") ? face_count : !strcmp(line, "hair") ? hair_count : 0;
        uint32_t descriptor = !strcmp(line, "face") ? DESC : !strcmp(line, "hair") ? DESC_HAIR : 0;
        if (!expected_end || *expected_end || !hash_count || expected < -1 || expected >= hash_count) {
            fprintf(stderr,"bad query row line=%s expected=%ld count=%u\n", line, expected, hash_count); return 5;
        }
        int32_t got = -99;
        if (!run(m, &c, ram, descriptor, key_start, &got) || got != expected) {
            fprintf(stderr, "original lookup mismatch case=%s expected=%ld actual=%d\n", line, expected, got); return 6;
        }
        checks++; if (!strcmp(tab, "upper")) upper++; if (!strcmp(tab, "lower")) lower_checks++; if (!strcmp(tab, "miss")) misses++;
    }
    fclose(f); free(line);
    if (!checks || !upper || !lower_checks || !misses || calls_801a8088 != checks) return 7;
    printf("Untouched DOL 801A8088 lookups passed: %u cases (upper=%u lower=%u misses=%u), HSH1 entries face=%u hair=%u; mutex and diagnostics boundaries stubbed.\n",
           checks, upper, lower_checks, misses, face_count, hair_count);
    free(ram); dlclose(lib); return 0;
}
