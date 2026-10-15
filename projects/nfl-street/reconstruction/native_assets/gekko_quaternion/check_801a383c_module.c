#include "quaternion_kernels.h"
#include "moderngekko/module_abi.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 0x80000000u
#define BYTES (24u * 1024u * 1024u)
#define STACK 0x80f47000u
#define A 0x80f37f00u
#define B 0x80f37f10u
#define OUT 0x80f37f20u

static uint32_t be(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
        (uint32_t)p[2] << 8 | p[3];
}

static void put(uint8_t *p, uint32_t word)
{
    p[0] = word >> 24; p[1] = word >> 16; p[2] = word >> 8; p[3] = word;
}

static uint32_t bits(float value)
{
    uint32_t word;
    memcpy(&word, &value, sizeof word);
    return word;
}

static uint32_t rnd(uint32_t *state)
{
    *state ^= *state << 13; *state ^= *state >> 17; *state ^= *state << 5;
    return *state;
}

static int run(const ModernGekkoModuleDesc *module, CPUState *cpu, uint32_t return_pc)
{
    for (unsigned i = 0; cpu->pc != return_pc && i < 10000; ++i)
        if (module->dispatch(cpu, cpu->pc) != 1 || cpu->exception)
            return 0;
    return cpu->pc == return_pc;
}

static void init(CPUState *cpu, uint8_t *ram, uint32_t pc, uint32_t return_pc)
{
    memset(cpu, 0, sizeof *cpu);
    cpu->ram = ram; cpu->ram_size = BYTES; cpu->gpr[1] = STACK;
    cpu->gpr[2] = 0x803b9a40u; cpu->gpr[13] = 0x803a9a40u;
    cpu->lr = return_pc; cpu->pc = pc; cpu->msr = 0x2000u;
    cpu->hid2 = 0xe0000000u; cpu->gqr[6] = 0x0f070007u;
    cpu->downcount = -1000000000;
}

static uint32_t unary(const ModernGekkoModuleDesc *module, uint8_t *ram,
    uint32_t pc, float input)
{
    const uint32_t return_pc = 0x81234560u;
    CPUState cpu;
    init(&cpu, ram, pc, return_pc);
    cpu.fpr[1] = input; cpu.ps1[1] = input;
    return run(module, &cpu, return_pc) ? bits((float)cpu.fpr[1]) : UINT32_MAX;
}

static int slerp(const ModernGekkoModuleDesc *module, uint8_t *ram, float t)
{
    const uint32_t return_pc = 0x81234560u;
    CPUState cpu;
    init(&cpu, ram, 0x801a383cu, return_pc);
    cpu.gpr[3] = OUT; cpu.gpr[4] = A; cpu.gpr[5] = B; cpu.gpr[6] = B;
    cpu.fpr[1] = t; cpu.ps1[1] = t;
    return run(module, &cpu, return_pc);
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    FILE *file = fopen(argv[1], "rb");
    uint8_t *ram = malloc(BYTES);
    if (!file || !ram || fread(ram, 1, BYTES, file) != BYTES) return 2;
    fclose(file);
    void *library = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    ModernGekkoGetModuleFn get = library ?
        (ModernGekkoGetModuleFn)dlsym(library, MODERNGEKKO_GET_MODULE_SYMBOL) : NULL;
    const ModernGekkoModuleDesc *module = get ? get() : NULL;
    if (!module) return 2;

    for (unsigned i = 0, state = 0x6d2b79f5u; i < 500000; ++i) {
        const uint32_t word = rnd(&state) % 0x3fc90fdcu;
        float input; memcpy(&input, &word, sizeof input);
        const uint32_t original = unary(module, ram, 0x80216b94u, input);
        const uint32_t native = bits(nflstreet_80216b94_sine(input));
        if (original != native) {
            fprintf(stderr, "80216B94 mismatch input=%08x original=%08x native=%08x\n",
                word, original, native);
            return 1;
        }
    }
    for (unsigned i = 0, state = 0x9e3779b9u; i < 500000; ++i) {
        const uint32_t word = 0x00800000u + rnd(&state) % (0x3e800001u - 0x00800000u);
        float input; memcpy(&input, &word, sizeof input);
        if (unary(module, ram, 0x802165c8u, input) !=
            bits(nflstreet_802165c8_sqrt(input))) return 1;
    }
    for (unsigned i = 0, state = 0x12345678u; i < 500000; ++i) {
        const uint32_t word = rnd(&state) % 0x3f800001u;
        float input; memcpy(&input, &word, sizeof input);
        if (unary(module, ram, 0x80215844u, input) !=
            bits(nflstreet_80215844_acos(input))) return 1;
    }
    for (unsigned n = 0, state = 0x801a383cu; n < 500000; ++n) {
        float a[4], b[4], output[4];
        const float t = (float)(rnd(&state) & 0xffffu) / 65535.0f;
        for (unsigned i = 0; i < 4; ++i) {
            a[i] = (float)(int16_t)rnd(&state) * 0x1p-14f;
            b[i] = (float)(int16_t)rnd(&state) * 0x1p-14f;
            put(ram + A - BASE + 4 * i, bits(a[i]));
            put(ram + B - BASE + 4 * i, bits(b[i]));
        }
        nflstreet_801a383c_slerp(t, a, b, output);
        if (!slerp(module, ram, t)) return 3;
        for (unsigned i = 0; i < 4; ++i)
            if (be(ram + OUT - BASE + 4 * i) != bits(output[i])) {
                fprintf(stderr, "801A383C mismatch case=%u lane=%u\n", n, i);
                return 1;
            }
    }
    puts("801A383C exact on all-RAM oracle.");
    return 0;
}
