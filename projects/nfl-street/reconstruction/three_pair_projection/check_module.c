#include "three_pair_projection.h"
#include "full_state_candidate.h"
#include "StaticRecompABI.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOURCE 0x80020000u
#define FACTORS 0x80021000u
#define OUTPUT 0x80030000u
#define ENTRY 0x801FFF10u

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
    CPUState* candidate_cpu = (CPUState*)calloc(1, sizeof(CPUState));
    u8* ram = (u8*)calloc(1, GC_MAIN_RAM_SIZE);
    u8* candidate_ram = (u8*)calloc(1, GC_MAIN_RAM_SIZE);
    if (!cpu || !candidate_cpu || !ram || !candidate_ram) {
        fprintf(stderr, "allocation failed\n");
        return 2;
    }
    u32 rng = 0x801FFF10u;
    unsigned matches = 0, mismatches = 0, skips = 0, dispatch_failures = 0;
    unsigned output_mismatches = 0;
    for (unsigned trial = 0; trial < 100000; ++trial) {
        memset(cpu, 0, sizeof(*cpu));
        cpu->ram = ram;
        cpu->ram_size = GC_MAIN_RAM_SIZE;
        cpu->gpr[3] = SOURCE;
        cpu->gpr[4] = FACTORS;
        cpu->gpr[5] = (trial & 1u) ? SOURCE + 0x20u : OUTPUT;
        cpu->lr = 0x81234560u;
        cpu->msr = PPC_MSR_FP;
        cpu->hid2 = PPC_HID2_LSQE;
        cpu->fpscr = (trial & 1u) ? 0xA2308104u : 0u;
        float source[12], factors[3], expected[3];
        for (unsigned i = 0; i < 12; ++i) {
            source[i] = random_float(&rng);
            const u32 value = float_bits(source[i]);
            write_word(ram, SOURCE + 4u * i, value);
            write_word(candidate_ram, SOURCE + 4u * i, value);
        }
        for (unsigned i = 0; i < 3; ++i) {
            factors[i] = random_float(&rng);
            const u32 value = float_bits(factors[i]);
            write_word(ram, FACTORS + 4u * i, value);
            write_word(candidate_ram, FACTORS + 4u * i, value);
            if (!(trial & 1u)) {
                write_word(ram, OUTPUT + 4u * i, 0x7fc00000u);
                write_word(candidate_ram, OUTPUT + 4u * i, 0x7fc00000u);
            }
        }
        nflstreet_three_pair_projection(source, factors, expected);
        *candidate_cpu = *cpu;
        candidate_cpu->ram = candidate_ram;
        const bool applied = nflstreet_three_pair_cpu_candidate(candidate_cpu);
        if (!applied)
            ++skips;
        if (module->dispatch(cpu, ENTRY) != 1 || cpu->exception ||
            cpu->pc != cpu->lr) {
            ++dispatch_failures;
            continue;
        }
        for (unsigned i = 0; i < 3; ++i) {
            const u32 actual = read_word(ram, cpu->gpr[5] + 4u * i);
            if (actual != float_bits(expected[i])) {
                if (output_mismatches < 5)
                    fprintf(stderr, "output mismatch trial=%u index=%u actual=%08x "
                            "candidate=%08x\n", trial, i, actual,
                            float_bits(expected[i]));
                ++output_mismatches;
            }
        }
        if (applied) {
            CPUState a = *cpu, b = *candidate_cpu;
            a.ram = b.ram = NULL;
            if (memcmp(&a, &b, sizeof(a)) != 0 ||
                memcmp(ram + SOURCE - GC_RAM_BASE,
                       candidate_ram + SOURCE - GC_RAM_BASE, 48u) != 0 ||
                memcmp(ram + OUTPUT - GC_RAM_BASE,
                       candidate_ram + OUTPUT - GC_RAM_BASE, 12u) != 0) {
                if (mismatches < 5)
                    fprintf(stderr, "full-state mismatch trial=%u fpscr=%08x/%08x\n",
                            trial, cpu->fpscr, candidate_cpu->fpscr);
                ++mismatches;
            } else {
                ++matches;
            }
        }
    }
    printf("trials=100000 full_state_matches=%u skips=%u mismatches=%u "
           "output_mismatches=%u dispatch_failures=%u\n", matches, skips,
           mismatches, output_mismatches, dispatch_failures);
    free(candidate_ram);
    free(ram);
    free(candidate_cpu);
    free(cpu);
    dlclose(library);
    return (matches < 90000 || mismatches || output_mismatches ||
            dispatch_failures) ? 1 : 0;
}
