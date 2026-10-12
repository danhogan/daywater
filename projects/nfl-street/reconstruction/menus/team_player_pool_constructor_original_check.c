/* Controlled original-DOL PLYRBODY pool constructor trace. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <moderngekko/module_abi.h>
#include "team_preview.h"

#define RAM_SIZE (24u * 1024u * 1024u)
#define R13 0x803a9a40u
#define STACK 0x817df000u
#define POOL_TOKEN 0x81710000u
#define BUFFER_BASE 0x802ab684u
#define TYPE0_TABLE 0x802ab1e8u
#define BODY_POINTERS 0x802760b8u

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}
static void put32(uint8_t *p, uint32_t x) {
    p[0] = (uint8_t)(x >> 24); p[1] = (uint8_t)(x >> 16);
    p[2] = (uint8_t)(x >> 8); p[3] = (uint8_t)x;
}
static uint8_t *at(uint8_t *ram, uint32_t address) {
    if (address < 0x80000000u || address - 0x80000000u >= RAM_SIZE) return NULL;
    return ram + address - 0x80000000u;
}
static int read_file(const char *path, uint8_t **out, size_t *size) {
    FILE *f = fopen(path, "rb"); if (!f) return 0;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long n = ftell(f);
    if (n <= 0 || fseek(f, 0, SEEK_SET)) { fclose(f); return 0; }
    uint8_t *p = malloc((size_t)n);
    if (!p || fread(p, 1, (size_t)n, f) != (size_t)n) {
        free(p); fclose(f); return 0;
    }
    fclose(f); *out = p; *size = (size_t)n; return 1;
}
static int load_dol_sections(uint8_t *ram, const uint8_t *dol, size_t size) {
    if (size < 0xe4) return 0;
    for (unsigned i = 0; i < 18; ++i) {
        uint32_t off = be32(dol + (i < 7 ? 0x00 : 0x1c) + 4 * (i < 7 ? i : i - 7));
        uint32_t addr = be32(dol + (i < 7 ? 0x48 : 0x64) + 4 * (i < 7 ? i : i - 7));
        uint32_t len = be32(dol + (i < 7 ? 0x90 : 0xac) + 4 * (i < 7 ? i : i - 7));
        if (!len) continue;
        uint8_t *dst = at(ram, addr);
        if (!dst || off > size || len > size - off) return 0;
        memcpy(dst, dol + off, len);
    }
    uint32_t bss = be32(dol + 0xd8), bss_size = be32(dol + 0xdc);
    if (bss_size) {
        uint8_t *dst = at(ram, bss);
        if (!dst || bss_size > RAM_SIZE - (bss - 0x80000000u)) return 0;
        memset(dst, 0, bss_size);
    }
    return 1;
}

typedef struct {
    uint32_t pool_token, index, supplied_buffer;
} preload_call;
static preload_call preload[32];
static uint32_t record_init[32], record_attach[32], record_attach_buffer[32];
static unsigned preload_count, init_count, lock_count, attach_count;

static void service_return(CPUState *c) { c->gpr[3] = 0; c->pc = c->lr; }
static void dispatch_with_boundaries(const ModernGekkoModuleDesc *m, CPUState *c) {
    switch (c->pc) {
    case 0x801a75b0u: /* supplied archive preload service */
        if (preload_count < 32) preload[preload_count++] = (preload_call){c->gpr[3], c->gpr[4], c->gpr[5]};
        service_return(c); return;
    case 0x801c65a0u: /* resource-record constructor service */
        if (init_count < 32) record_init[init_count++] = c->gpr[3];
        service_return(c); return;
    case 0x801c5ec8u: /* resource record attach prelude */
        lock_count++; service_return(c); return;
    case 0x801c67e4u: /* resource-record buffer attach service */
        if (attach_count < 32) {
            record_attach[attach_count] = c->gpr[3];
            record_attach_buffer[attach_count] = c->gpr[4];
            attach_count++;
        }
        service_return(c); return;
    default:
        c->downcount = c->cycle_budget = 100000000;
        m->dispatch(c, c->pc);
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: pool-constructor-check <original-module.so> <hash-checked-original.dol>\n");
        return 2;
    }
    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen failed\n"); return 3; }
    ModernGekkoGetModuleFn get = (ModernGekkoGetModuleFn)dlsym(lib, MODERNGEKKO_GET_MODULE_SYMBOL);
    if (!get) return 3;
    const ModernGekkoModuleDesc *m = get();
    if (!m || m->cpu_state_size != sizeof(CPUState)) return 3;

    uint8_t *dol = NULL; size_t dol_size = 0;
    if (!read_file(argv[2], &dol, &dol_size)) return 4;
    uint8_t *ram = calloc(1, RAM_SIZE);
    if (!ram || !load_dol_sections(ram, dol, dol_size)) return 5;
    free(dol);

    /* 800DFBE4 supplies three arrays of three valid output-buffer pointers. */
    for (unsigned i = 0; i < 9; ++i) {
        uint8_t *slot = at(ram, BUFFER_BASE + i * 4);
        if (!slot) return 6;
        put32(slot, 0x81730000u + i * 0x100u);
    }
    uint8_t *token_slot = at(ram, R13 - 0x5ae4u);
    uint8_t *zero_gate = at(ram, R13 - 0x6234u);
    if (!token_slot || !zero_gate) return 6;
    put32(token_slot, POOL_TOKEN);
    put32(zero_gate, 0);

    CPUState c = {0};
    c.ram = ram; c.ram_size = RAM_SIZE; c.pc = 0x800dfbe4u;
    c.lr = 0; c.gpr[1] = STACK; c.gpr[2] = 0x803b9a40u; c.gpr[13] = R13;
    c.downcount = c.cycle_budget = 100000000;
    for (unsigned steps = 0; c.pc && steps < 1000000; ++steps) {
        if (c.exception) break;
        dispatch_with_boundaries(m, &c);
    }
    if (c.pc || c.exception) {
        fprintf(stderr, "original constructor stopped pc=%08x exception=%08x\n", c.pc, c.exception);
        return 7;
    }

    const uint32_t expected_desc[3] = {0x80271c98u, 0x80271dd8u, 0x80271f18u};
    uint32_t *groups = calloc(9, sizeof(uint32_t));
    if (!groups) return 8;
    for (unsigned group = 0; group < 3; ++group) {
        uint32_t native_entries[3];
        if (!nflstreet_team_preview_body_entries(group, native_entries)) return 8;
        uint32_t desc = be32(at(ram, BODY_POINTERS + group * 4));
        if (desc != expected_desc[group]) {
            fprintf(stderr, "body descriptor pointer mismatch group=%u got=%08x expected=%08x\n",
                    group, desc, expected_desc[group]); return 9;
        }
        uint32_t first = be32(at(ram, desc + 0x0c));
        if (first != 1 + group * 3) {
            fprintf(stderr, "body descriptor first-index mismatch group=%u got=%u\n", group, first); return 10;
        }
        for (unsigned row = 0; row < 3; ++row) {
            uint32_t index = be32(at(ram, desc + 0x0c + row * 0x50));
            groups[group * 3 + row] = index;
            if (index != native_entries[row]) {
                fprintf(stderr, "body descriptor row mismatch group=%u row=%u index=%u\n",
                        group, row, index); return 11;
            }
        }
        if (be32(at(ram, desc + 0x0c + 3 * 0x50)) != 0xffffu) {
            fprintf(stderr, "unexpected body descriptor terminator group=%u\n", group); return 12;
        }
    }
    uint32_t override_entries[3], guard_entries[3] = {99,99,99};
    if (!nflstreet_team_preview_body_entries(7, override_entries) ||
        override_entries[0] != groups[6] || override_entries[1] != groups[7] ||
        override_entries[2] != groups[8] ||
        nflstreet_team_preview_body_entries(3, guard_entries) ||
        guard_entries[0] != 99 || guard_entries[1] != 99 || guard_entries[2] != 99)
        return 16;
    if (preload_count != 9 || init_count != 9 || lock_count != 9 || attach_count != 9) {
        fprintf(stderr, "service counts preload=%u init=%u prelude=%u attach=%u\n",
                preload_count, init_count, lock_count, attach_count); return 13;
    }
    for (unsigned i = 0; i < 9; ++i) {
        uint32_t outbuf = 0x81730000u + i * 0x100u;
        uint32_t descriptor_record = expected_desc[i / 3] + 0x1cu + (i % 3) * 0x50u;
        if (preload[i].pool_token != POOL_TOKEN || preload[i].index != groups[i] ||
            preload[i].supplied_buffer != outbuf ||
            record_init[i] != descriptor_record ||
            record_attach[i] != descriptor_record || record_attach_buffer[i] != outbuf) {
            fprintf(stderr, "constructor event mismatch row=%u index=%u buffer=%08x init=%08x attach=%08x/%08x\n",
                    i, preload[i].index, preload[i].supplied_buffer,
                    record_init[i], record_attach[i], record_attach_buffer[i]); return 14;
        }
    }
    /* Verify each type-0 group record summarizes its three-entry descriptor. */
    for (unsigned key = 0; key < 3; ++key) {
        uint32_t rec = TYPE0_TABLE + key * 0x14u;
        uint32_t name_address = be32(at(ram, rec));
        uint8_t *name = at(ram, name_address);
        if (!name || memcmp(name, "PLYRBODY", 8) != 0 || name[8] != 0 ||
            be32(at(ram, rec + 4)) != 0 || be32(at(ram, rec + 8)) != POOL_TOKEN ||
            be32(at(ram, rec + 0x0c)) != 3 ||
            be32(at(ram, rec + 0x10)) != expected_desc[key]) {
            fprintf(stderr, "type-0 body record mismatch key=%u count=%u tail=%08x\n", key,
                    be32(at(ram, rec + 0x0c)), be32(at(ram, rec + 0x10))); return 15;
        }
    }
    printf("Untouched DOL 800DFBE4/801EB3F4 constructor passed: 3 PLYRBODY groups, descriptor indices 1..9 grouped 1/4/7, 9 supplied buffers and captured service calls; no rendering claim.\n");
    free(groups); free(ram); dlclose(lib); return 0;
}
