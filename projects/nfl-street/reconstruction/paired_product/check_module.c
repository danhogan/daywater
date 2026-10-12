#include "paired_product.h"
#include "paired_product_full_state_candidate.h"

#include "StaticRecompABI.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT 0x80020000u
#define FACTORS 0x80021000u
#define OUTPUT 0x80030000u
#define CONSTANT 0x803A3958u
#define GUEST_ENTRY 0x801FFDF0u

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

static float random_finite(uint32_t* state)
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
    CPUState* cpu = calloc(1, sizeof(*cpu));
    CPUState* candidate_cpu = calloc(1, sizeof(*candidate_cpu));
    uint8_t* ram = calloc(1, GC_MAIN_RAM_SIZE);
    uint8_t* candidate_ram = calloc(1, GC_MAIN_RAM_SIZE);
    if (!cpu || !candidate_cpu || !ram || !candidate_ram) {
        fprintf(stderr, "allocation failed\n");
        return 2;
    }
    uint32_t rng = 0x1FFDF0u;
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
        cpu->gpr[1] = 0x80010000u;
        cpu->gpr[3] = INPUT;
        cpu->gpr[4] = FACTORS;
        cpu->gpr[5] = OUTPUT;
        cpu->lr = 0x81234560u;
        cpu->msr = PPC_MSR_FP;
        cpu->hid2 = PPC_HID2_LSQE;
        cpu->fpscr = (trial & 1u) ? 0xA2308104u : 0u;
        const unsigned saved_regs[3] = {14, 15, 31};
        for (unsigned i = 0; i < 3; ++i) {
            const unsigned reg = saved_regs[i];
            cpu->fpr[reg] = random_finite(&rng);
            cpu->ps1[reg] = random_finite(&rng);
        }
        float input[12], factors[12], constant[2], expected[12];
        for (unsigned i = 0; i < 12; ++i) {
            input[i] = random_finite(&rng);
            factors[i] = random_finite(&rng);
            write_word(ram, INPUT + 4u * i, float_bits(input[i]));
            write_word(ram, FACTORS + 4u * i, float_bits(factors[i]));
            write_word(ram, OUTPUT + 4u * i, 0x7fc00000u);
            write_word(candidate_ram, INPUT + 4u * i, float_bits(input[i]));
            write_word(candidate_ram, FACTORS + 4u * i, float_bits(factors[i]));
            write_word(candidate_ram, OUTPUT + 4u * i, 0x7fc00000u);
        }
        for (unsigned i = 0; i < 2; ++i) {
            constant[i] = random_finite(&rng);
            write_word(ram, CONSTANT + 4u * i, float_bits(constant[i]));
            write_word(candidate_ram, CONSTANT + 4u * i, float_bits(constant[i]));
        }
        *candidate_cpu = *cpu;
        candidate_cpu->ram = candidate_ram;
        const bool full_state_applied =
            nflstreet_paired_product_cpu_candidate(candidate_cpu);
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
        nflstreet_paired_product(input, factors, constant, expected);
        for (unsigned i = 0; i < 12; ++i) {
            uint32_t actual = read_word(ram, OUTPUT + 4u * i);
            uint32_t candidate = float_bits(expected[i]);
            if (actual != candidate) {
                if (mismatches < 5)
                    printf("trial=%u output=%u actual=%08x candidate=%08x\n",
                           trial, i, actual, candidate);
                ++mismatches;
            }
        }
        if (full_state_applied) {
            CPUState reference = *cpu;
            CPUState candidate = *candidate_cpu;
            reference.ram = NULL;
            candidate.ram = NULL;
            if (memcmp(&reference, &candidate, sizeof(reference)) != 0 ||
                memcmp(ram + (cpu->gpr[1] - 64u - GC_RAM_BASE),
                       candidate_ram + (cpu->gpr[1] - 64u - GC_RAM_BASE), 48) != 0 ||
                memcmp(ram + (OUTPUT - GC_RAM_BASE),
                       candidate_ram + (OUTPUT - GC_RAM_BASE), 48) != 0) {
                if (full_state_mismatches < 5)
                    printf("full-state mismatch trial=%u fpscr=%08x/%08x "
                           "fpr14=%g/%g fpr15=%g/%g fpr31=%g/%g\n", trial,
                           cpu->fpscr, candidate_cpu->fpscr,
                           cpu->fpr[14], candidate_cpu->fpr[14],
                           cpu->fpr[15], candidate_cpu->fpr[15],
                           cpu->fpr[31], candidate_cpu->fpr[31]);
                ++full_state_mismatches;
            } else {
                ++full_state_matches;
            }
        }
    }
    printf("trials=100000 mismatched_words=%u dispatch_failures=%u "
           "exceptions_or_return_errors=%u\n",
           mismatches, dispatch_failures, exceptions);
    printf("full_state_matches=%u full_state_mismatches=%u full_state_skips=%u\n",
           full_state_matches, full_state_mismatches, full_state_skips);
    free(candidate_ram);
    free(candidate_cpu);
    free(ram);
    free(cpu);
    dlclose(library);
    return (mismatches || dispatch_failures || exceptions || full_state_mismatches ||
            full_state_matches < 90000) ? 1 : 0;
}
