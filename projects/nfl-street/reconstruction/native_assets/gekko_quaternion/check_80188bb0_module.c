/* Differential oracle for 80188BB0.  It starts from a saved all-RAM image,
   invokes the recompiled original with the game's GQR6 setup, and checks the
   native paired-single model on every 16-bit angle.  The saved case is first
   (the coefficients are read at 8039E100), followed by randomized finite
   coefficient sets.  No game asset is written or committed. */
#include "quaternion_kernels.h"

#include "moderngekko/module_abi.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u * 1024u * 1024u)
#define STACK 0x80f47000u
#define COEFFICIENTS 0x8039e100u
#define OUT0 0x80f37f00u
#define OUT1 0x80f37f04u

static uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}

static void put32(uint8_t *p, uint32_t word)
{
    p[0] = (uint8_t)(word >> 24); p[1] = (uint8_t)(word >> 16);
    p[2] = (uint8_t)(word >> 8); p[3] = (uint8_t)word;
}

static uint32_t bits(float value)
{
    uint32_t word;
    memcpy(&word, &value, sizeof(word));
    return word;
}

static float real(uint32_t word)
{
    float value;
    memcpy(&value, &word, sizeof(value));
    return value;
}

static uint32_t random_word(uint32_t *state)
{
    *state ^= *state << 13; *state ^= *state >> 17; *state ^= *state << 5;
    return *state;
}

static int run(const ModernGekkoModuleDesc *module, uint8_t *ram,
    uint16_t angle)
{
    CPUState cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.ram = ram; cpu.ram_size = RAM_BYTES;
    cpu.gpr[1] = STACK; cpu.gpr[3] = angle; cpu.gpr[4] = COEFFICIENTS;
    cpu.gpr[5] = OUT0; cpu.gpr[6] = OUT1; cpu.lr = 0x81234560u;
    cpu.msr = 0x2000u; cpu.hid2 = 0xe0000000u;
    cpu.gqr[6] = 0x0f070007u; /* signed 16-bit input, scale 2^-15 */
    cpu.downcount = -1000000000;
    return module->dispatch(&cpu, 0x80188bb0u) == 1 && !cpu.exception &&
           cpu.pc == cpu.lr;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s saved.mem1 gGNNE69_recomp.so\n", argv[0]);
        return 2;
    }
    FILE *file = fopen(argv[1], "rb");
    uint8_t *ram = malloc(RAM_BYTES);
    if (!file || !ram || fread(ram, 1, RAM_BYTES, file) != RAM_BYTES) return 2;
    fclose(file);
    void *library = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    ModernGekkoGetModuleFn get = library ? (ModernGekkoGetModuleFn)dlsym(library,
        MODERNGEKKO_GET_MODULE_SYMBOL) : NULL;
    const ModernGekkoModuleDesc *module = get ? get() : NULL;
    if (!module || module->cpu_abi_version != MODERNGEKKO_CPU_ABI_VERSION) return 2;

    uint32_t rng = 0x80188bb0u;
    unsigned checks = 0;
    for (unsigned set = 0; set < 5; ++set) {
        float coefficients[6];
        for (unsigned i = 0; i < 6; ++i) {
            if (!set) coefficients[i] = real(be32(ram + COEFFICIENTS - RAM_BASE + 4u * i));
            else coefficients[i] = (float)((int32_t)(random_word(&rng) & 0xffffu) - 32768) * 0x1p-14f;
            put32(ram + COEFFICIENTS - RAM_BASE + 4u * i, bits(coefficients[i]));
        }
        for (unsigned raw = 0; raw <= 0xffffu; ++raw) {
            float native[2];
            nflstreet_80188bb0_pair((uint16_t)raw, coefficients, native);
            put32(ram + OUT0 - RAM_BASE, 0x7fc00000u);
            put32(ram + OUT1 - RAM_BASE, 0x7fc00000u);
            if (!run(module, ram, (uint16_t)raw)) {
                fprintf(stderr, "original failed set=%u angle=%04x\n", set, raw);
                return 3;
            }
            const uint32_t original0 = be32(ram + OUT0 - RAM_BASE);
            const uint32_t original1 = be32(ram + OUT1 - RAM_BASE);
            if (original0 != bits(native[0]) || original1 != bits(native[1])) {
                fprintf(stderr, "set=%u angle=%04x original=%08x,%08x native=%08x,%08x\n",
                    set, raw, original0, original1, bits(native[0]), bits(native[1]));
                return 1;
            }
            ++checks;
        }
    }
    printf("80188BB0: %u exact paired-single results (saved coefficients plus four random sets).\n", checks);
    dlclose(library); free(ram); return 0;
}
