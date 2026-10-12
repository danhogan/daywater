#include "odd_polynomial_pair.h"
#include "full_state_candidate.h"

#include "StaticRecompABI.h"

#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK 0x80010000u
#define COEFFICIENTS 0x80020000u
#define RESULT_0 0x80030000u
#define RESULT_1 0x80030004u
#define GUEST_ENTRY 0x80188BB0u

PPCMemWriteJournal g_mem_write_journal = NULL;
void* g_mem_write_journal_user = NULL;

static void write_word(uint8_t* ram, uint32_t address, uint32_t value)
{
    size_t offset = address - GC_RAM_BASE;
    ram[offset] = (uint8_t)(value >> 24);
    ram[offset + 1] = (uint8_t)(value >> 16);
    ram[offset + 2] = (uint8_t)(value >> 8);
    ram[offset + 3] = (uint8_t)value;
}

static uint32_t read_word(const uint8_t* ram, uint32_t address)
{
    size_t offset = address - GC_RAM_BASE;
    return ((uint32_t)ram[offset] << 24) | ((uint32_t)ram[offset + 1] << 16) |
           ((uint32_t)ram[offset + 2] << 8) | ram[offset + 3];
}

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static uint32_t random_word(uint32_t* state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
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
    CPUState* cpu = calloc(1, sizeof(*cpu));
    CPUState* candidate_cpu = calloc(1, sizeof(*candidate_cpu));
    uint8_t* ram = calloc(1, GC_MAIN_RAM_SIZE);
    uint8_t* candidate_ram = calloc(1, GC_MAIN_RAM_SIZE);
    if (!cpu || !candidate_cpu || !ram || !candidate_ram) {
        fprintf(stderr, "allocation failed\n");
        return 2;
    }
    uint32_t rng = 0x188BB0u;
    unsigned mismatches = 0;
    unsigned dispatch_failures = 0;
    unsigned exceptions = 0;
    unsigned full_state_matches = 0;
    unsigned full_state_mismatches = 0;
    unsigned full_state_skips = 0;
    for (unsigned trial = 0; trial < 100000; ++trial) {
        memset(cpu, 0, sizeof(*cpu));
        cpu->ram = ram;
        cpu->ram_size = GC_MAIN_RAM_SIZE;
        cpu->gpr[1] = STACK;
        cpu->gpr[3] = random_word(&rng) & 0xffffu;
        cpu->gpr[4] = COEFFICIENTS;
        cpu->gpr[5] = RESULT_0;
        cpu->gpr[6] = RESULT_1;
        cpu->lr = 0x81234560u;
        cpu->msr = PPC_MSR_FP;
        cpu->hid2 = PPC_HID2_LSQE;
        cpu->gqr[6] = 0x0f070007u; /* init routine 0x8015668C: signed 16-bit, 2^-15 */
        cpu->fpscr = (trial & 1u) ? 0xA2308104u : 0u;
        float c[6];
        for (unsigned i = 0; i < 6; ++i) {
            c[i] = ((int32_t)(random_word(&rng) & 0xffffu) - 32768) / 16384.0f;
            write_word(ram, COEFFICIENTS + 4u * i, float_bits(c[i]));
            write_word(candidate_ram, COEFFICIENTS + 4u * i, float_bits(c[i]));
        }
        write_word(ram, RESULT_0, 0x7fc00000u);
        write_word(ram, RESULT_1, 0x7fc00000u);
        write_word(candidate_ram, RESULT_0, 0x7fc00000u);
        write_word(candidate_ram, RESULT_1, 0x7fc00000u);
        *candidate_cpu = *cpu;
        candidate_cpu->ram = candidate_ram;
        const bool full_state_applied =
            nflstreet_odd_polynomial_cpu_candidate(candidate_cpu);
        if (!full_state_applied)
            ++full_state_skips;
        if (module->dispatch(cpu, GUEST_ENTRY) != 1) {
            ++dispatch_failures;
            continue;
        }
        if (cpu->exception != 0 || cpu->pc != cpu->lr) {
            ++exceptions;
            continue;
        }
        float t[2] = {
            ldexpf((float)(int16_t)cpu->gpr[3], -15),
            ldexpf((float)(int16_t)(cpu->gpr[3] + 0x4000u), -15),
        };
        float expected[2];
        nflstreet_odd_polynomial_pair(t, c, expected);
        uint32_t actual0 = read_word(ram, RESULT_0);
        uint32_t actual1 = read_word(ram, RESULT_1);
        if (actual0 != float_bits(expected[0]) ||
            actual1 != float_bits(expected[1])) {
            if (mismatches < 5) {
                printf("trial=%u t=(%.8g,%.8g) actual=(%08x,%08x) "
                       "candidate=(%08x,%08x)\n", trial, t[0], t[1],
                       actual0, actual1, float_bits(expected[0]),
                       float_bits(expected[1]));
            }
            ++mismatches;
        }
        if (full_state_applied) {
            CPUState reference = *cpu;
            CPUState candidate = *candidate_cpu;
            reference.ram = NULL;
            candidate.ram = NULL;
            if (memcmp(&reference, &candidate, sizeof(reference)) != 0 ||
                memcmp(ram + (STACK - 16u - GC_RAM_BASE),
                       candidate_ram + (STACK - 16u - GC_RAM_BASE), 4) != 0 ||
                read_word(candidate_ram, RESULT_0) != actual0 ||
                read_word(candidate_ram, RESULT_1) != actual1) {
                if (full_state_mismatches < 5) {
                    printf("full-state mismatch trial=%u reference_fpscr=%08x "
                           "candidate_fpscr=%08x reference_fpr0=%08x "
                           "candidate_fpr0=%08x\n", trial, cpu->fpscr,
                           candidate_cpu->fpscr, float_bits((float)cpu->fpr[0]),
                           float_bits((float)candidate_cpu->fpr[0]));
                }
                ++full_state_mismatches;
            } else {
                ++full_state_matches;
            }
        }
    }
    printf("trials=100000 mismatches=%u dispatch_failures=%u exceptions_or_return_errors=%u\n",
           mismatches, dispatch_failures, exceptions);
    printf("full_state_matches=%u full_state_mismatches=%u full_state_skips=%u\n",
           full_state_matches, full_state_mismatches, full_state_skips);
    if (memcmp(ram, candidate_ram, GC_MAIN_RAM_SIZE) != 0)
        printf("final RAM differs (expected when the narrow full-state candidate skipped calls)\n");
    free(candidate_ram);
    free(candidate_cpu);
    free(ram);
    free(cpu);
    dlclose(library);
    return (mismatches || dispatch_failures || exceptions || full_state_mismatches ||
            full_state_matches < 90000) ? 1 : 0;
}
