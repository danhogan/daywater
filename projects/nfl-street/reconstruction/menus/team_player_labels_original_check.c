/* Execute original 8014E9A0 with supplied raw PLAY PFNA/PLNA getters. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <moderngekko/module_abi.h>

#define RAM_SIZE (24u * 1024u * 1024u)
#define QUERY 0x81710000u
#define OUTPUT 0x81720000u
#define STACK 0x817df000u
#define R13 0x803a9a40u
#define TAG_PFNA 0x414e4650u
#define TAG_PLNA 0x414e4c50u

static uint8_t *at(uint8_t *ram, uint32_t address) {
    if (address < 0x80000000u || address - 0x80000000u >= RAM_SIZE) return NULL;
    return ram + address - 0x80000000u;
}
static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static int parse_hex(const char *s, uint8_t *out, size_t capacity, size_t *length) {
    size_t n = strlen(s);
    if (!strcmp(s, "-")) { *length = 0; return 1; }
    if ((n & 1) || n / 2 > capacity) return 0;
    for (size_t i = 0; i < n / 2; ++i) {
        int hi = hexval(s[2 * i]), lo = hexval(s[2 * i + 1]);
        if (hi < 0 || lo < 0) return 0;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    *length = n / 2; return 1;
}
static unsigned seen_pfna, seen_plna;
static uint8_t supplied_pfna[12], supplied_plna[14];

static void return_from(CPUState *c) { c->pc = c->lr; }
static void dispatch(const ModernGekkoModuleDesc *m, CPUState *c, uint8_t *ram) {
    if (c->pc == 0x801496c4u) {
        uint32_t tag = c->gpr[4], destination = c->gpr[5];
        uint8_t *dst;
        if (c->gpr[3] != QUERY || !(dst = at(ram, destination))) {
            c->exception = 0x4c41424cu; return;
        }
        if (tag == TAG_PFNA && seen_pfna++ == 0) memcpy(dst, supplied_pfna, sizeof supplied_pfna);
        else if (tag == TAG_PLNA && seen_plna++ == 0) memcpy(dst, supplied_plna, sizeof supplied_plna);
        else { c->exception = 0x4c414254u; return; }
        return_from(c); return;
    }
    c->downcount = c->cycle_budget = 100000000;
    m->dispatch(c, c->pc);
}

static int invoke(const ModernGekkoModuleDesc *m, uint8_t *ram,
                  unsigned capacity, const uint8_t pfna[12], const uint8_t plna[14],
                  const uint8_t *expected, size_t expected_length) {
    uint8_t *query = at(ram, QUERY), *out = at(ram, OUTPUT);
    if (!query || !out || capacity > 26 || expected_length > capacity) return 0;
    memcpy(supplied_pfna, pfna, 12); memcpy(supplied_plna, plna, 14);
    memset(query, 0, 32); memset(out, 0x5a, 32); seen_pfna = seen_plna = 0;
    CPUState c = {0}; c.ram = ram; c.ram_size = RAM_SIZE; c.msr = 0x2000;
    c.pc = 0x8014e9a0u; c.gpr[1] = STACK; c.gpr[2] = 0x803b9a40u;
    c.gpr[13] = R13; c.gpr[3] = QUERY; c.gpr[4] = OUTPUT; c.gpr[5] = capacity;
    c.downcount = c.cycle_budget = 100000000;
    for (unsigned step = 0; c.pc && step < 100000; ++step) {
        dispatch(m, &c, ram);
        if (c.exception) break;
    }
    if (c.pc || c.exception || c.gpr[1] != STACK || seen_pfna != 1 || seen_plna != 1)
        return 0;
    if (memcmp(out, expected, expected_length) != 0 || out[expected_length] != 0)
        return 0;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: team-player-labels-original-check <original-module.so> <private-queries.tsv>\n");
        return 2;
    }
    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen failed\n"); return 3; }
    ModernGekkoGetModuleFn get = (ModernGekkoGetModuleFn)dlsym(lib, MODERNGEKKO_GET_MODULE_SYMBOL);
    if (!get) return 3;
    const ModernGekkoModuleDesc *m = get();
    if (!m || m->cpu_state_size != sizeof(CPUState)) return 3;
    uint8_t *ram = calloc(1, RAM_SIZE);
    if (!ram) return 4;

    FILE *f = fopen(argv[2], "rb"); if (!f) return 4;
    char *line = NULL; size_t line_capacity = 0; ssize_t length;
    unsigned cases = 0, rows = 0, corners = 0;
    while ((length = getline(&line, &line_capacity, f)) >= 0) {
        if (!length || line[0] == '#') continue;
        char *save = NULL;
        char *cap_s = strtok_r(line, "\t\r\n", &save);
        char *pfna_s = strtok_r(NULL, "\t\r\n", &save);
        char *plna_s = strtok_r(NULL, "\t\r\n", &save);
        char *expected_s = strtok_r(NULL, "\t\r\n", &save);
        char *kind_s = strtok_r(NULL, "\t\r\n", &save);
        if (!cap_s || !pfna_s || !plna_s || !expected_s || !kind_s || strtok_r(NULL, "\t\r\n", &save)) return 5;
        char *end = NULL; unsigned long cap = strtoul(cap_s, &end, 10);
        if (!end || *end || cap > 26) return 5;
        uint8_t pfna[12], plna[14], expected[26]; size_t pfna_n, plna_n, expected_n;
        if (!parse_hex(pfna_s, pfna, sizeof pfna, &pfna_n) || pfna_n != 12 ||
            !parse_hex(plna_s, plna, sizeof plna, &plna_n) || plna_n != 14 ||
            !parse_hex(expected_s, expected, sizeof expected, &expected_n)) return 5;
        if (!invoke(m, ram, (unsigned)cap, pfna, plna, expected, expected_n)) {
            fprintf(stderr, "original 8014E9A0 name mismatch case=%u capacity=%lu\n", cases, cap);
            return 6;
        }
        cases++; if (!strcmp(kind_s, "play")) rows++; else if (!strcmp(kind_s, "corner")) corners++; else return 5;
    }
    fclose(f); free(line);
    if (!cases || rows != 402u * 27u || corners != 4u * 27u || cases != rows + corners) return 7;
    printf("Untouched DOL 8014E9A0 name helper passed: %u actual PLAY row-capacity probes and %u empty/name corner probes, capacities 0..26; PFNA/PLNA database getters supplied.\n", rows, corners);
    free(ram); dlclose(lib); return 0;
}
