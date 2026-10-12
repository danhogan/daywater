#include "weighted_pair_reduction.h"
#include "StaticRecompABI.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOURCE 0x80020000u
#define FACTORS 0x80021000u
#define OUTPUT 0x80030000u
#define ENTRY 0x801FFEBCu

PPCMemWriteJournal g_mem_write_journal = NULL;
void* g_mem_write_journal_user = NULL;

static void write_word(u8* ram, u32 address, u32 value)
{
    const u32 offset = address - GC_RAM_BASE;
    ram[offset] = (u8)(value >> 24);
    ram[offset + 1u] = (u8)(value >> 16);
    ram[offset + 2u] = (u8)(value >> 8);
    ram[offset + 3u] = (u8)value;
}

static u32 read_word(const u8* ram, u32 address)
{
    const u32 offset = address - GC_RAM_BASE;
    return ((u32)ram[offset] << 24) | ((u32)ram[offset + 1u] << 16) |
           ((u32)ram[offset + 2u] << 8) | ram[offset + 3u];
}

static u32 float_bits(float value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u32 random_word(u32* state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static float random_float(u32* state)
{
    return ((int32_t)(random_word(state) & 0xffffu) - 32768) / 16384.0f;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s /path/to/gGNNE69_recomp.so\n", argv[0]);
        return 2;
    }
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 2;
    }
    StaticRecompGetModuleFn get_module = NULL;
    void* symbol = dlsym(library, STATICRECOMP_GET_MODULE_SYMBOL);
    memcpy(&get_module, &symbol, sizeof(get_module));
    if (!get_module) {
        fprintf(stderr, "module export missing\n");
        return 2;
    }
    const StaticRecompModuleDesc* module = get_module();
    if (module->cpu_abi_version != GXRUNTIME_CPU_ABI_VERSION ||
        module->cpu_state_size != sizeof(CPUState) ||
        strcmp(module->game_id, "GNNE69") != 0) {
        fprintf(stderr, "module ABI or game mismatch\n");
        return 2;
    }
    CPUState* cpu = (CPUState*)calloc(1, sizeof(CPUState));
    u8* ram = (u8*)calloc(1, GC_MAIN_RAM_SIZE);
    if (!cpu || !ram) {
        fprintf(stderr, "allocation failed\n");
        return 2;
    }
    u32 rng = ENTRY;
    unsigned matches = 0, mismatches = 0, dispatch_failures = 0;
    for (unsigned trial = 0; trial < 100000; ++trial) {
        memset(cpu, 0, sizeof(*cpu));
        cpu->ram = ram;
        cpu->ram_size = GC_MAIN_RAM_SIZE;
        cpu->gpr[3] = SOURCE;
        cpu->gpr[4] = FACTORS;
        cpu->gpr[5] = OUTPUT;
        cpu->lr = 0x81234560u;
        cpu->msr = PPC_MSR_FP;
        cpu->hid2 = PPC_HID2_LSQE;
        float source[12], factors[3], expected[3];
        for (unsigned i = 0; i < 12; ++i) {
            source[i] = random_float(&rng);
            write_word(ram, SOURCE + 4u * i, float_bits(source[i]));
        }
        for (unsigned i = 0; i < 3; ++i) {
            factors[i] = random_float(&rng);
            write_word(ram, FACTORS + 4u * i, float_bits(factors[i]));
            write_word(ram, OUTPUT + 4u * i, 0x7fc00000u);
        }
        nflstreet_weighted_pair_reduction(source, factors, expected);
        if (module->dispatch(cpu, ENTRY) != 1 || cpu->exception ||
            cpu->pc != cpu->lr) {
            ++dispatch_failures;
            continue;
        }
        bool equal = true;
        for (unsigned i = 0; i < 3; ++i) {
            const u32 actual = read_word(ram, OUTPUT + 4u * i);
            const u32 candidate = float_bits(expected[i]);
            if (actual != candidate) {
                if (mismatches < 5)
                    fprintf(stderr, "mismatch trial=%u index=%u actual=%08x "
                            "candidate=%08x\n", trial, i, actual, candidate);
                equal = false;
            }
        }
        if (equal)
            ++matches;
        else
            ++mismatches;
    }
    printf("trials=100000 output_matches=%u mismatches=%u "
           "dispatch_failures=%u\n", matches, mismatches, dispatch_failures);
    free(ram);
    free(cpu);
    dlclose(library);
    return (mismatches || dispatch_failures) ? 1 : 0;
}
