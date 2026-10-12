/* Differential test of the guarded 0xE0000000 locked-cache path. */
#include "paired_product_full_state_candidate.h"
#include "StaticRecompABI.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTRY 0x801FFDF0u
#define LC_BASE 0xE0000000u
#define LC_SIZE 0x00040000u
#define FACTORS 0x80021000u
#define CONSTANT 0x803A3958u
#define STACK 0x80010000u

PPCMemWriteJournal g_mem_write_journal = NULL;
void* g_mem_write_journal_user = NULL;

static void word(u8* bytes, size_t offset, u32 value)
{
    bytes[offset] = (u8)(value >> 24);
    bytes[offset + 1] = (u8)(value >> 16);
    bytes[offset + 2] = (u8)(value >> 8);
    bytes[offset + 3] = (u8)value;
}

static void* lc_pointer(CPUState* cpu, u32 address, u32 size)
{
    if (address < LC_BASE || size > LC_SIZE ||
        address - LC_BASE > LC_SIZE - size)
        return NULL;
    return (u8*)cpu->external_user_data + (address - LC_BASE);
}

static u64 lc_read(CPUState* cpu, u32 address, u8 size)
{
    const u8* bytes = (const u8*)lc_pointer(cpu, address, size);
    u64 value = 0;
    if (!bytes)
        return 0;
    for (u8 i = 0; i < size; ++i)
        value = (value << 8) | bytes[i];
    return value;
}

static void lc_write(CPUState* cpu, u32 address, u64 value, u8 size)
{
    u8* bytes = (u8*)lc_pointer(cpu, address, size);
    if (!bytes)
        return;
    for (u8 i = 0; i < size; ++i)
        bytes[size - 1u - i] = (u8)(value >> (8u * i));
}

static u32 random_word(u32* state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static u32 random_float_bits(u32* state)
{
    float value = ((int32_t)(random_word(state) & 0xffffu) - 32768) / 16384.0f;
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
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
    CPUState* original = (CPUState*)calloc(1, sizeof(CPUState));
    CPUState* candidate = (CPUState*)calloc(1, sizeof(CPUState));
    u8* ram = (u8*)calloc(1, GC_MAIN_RAM_SIZE);
    u8* candidate_ram = (u8*)calloc(1, GC_MAIN_RAM_SIZE);
    u8* cache = (u8*)calloc(1, LC_SIZE);
    u8* candidate_cache = (u8*)calloc(1, LC_SIZE);
    if (!original || !candidate || !ram || !candidate_ram ||
        !cache || !candidate_cache) {
        fprintf(stderr, "allocation failed\n");
        return 2;
    }
    u32 rng = 0xE0000000u;
    unsigned matches = 0, skips = 0, mismatches = 0, dispatch_failures = 0;
    for (unsigned trial = 0; trial < 10000; ++trial) {
        memset(original, 0, sizeof(*original));
        original->ram = ram;
        original->ram_size = GC_MAIN_RAM_SIZE;
        original->external_read = lc_read;
        original->external_write = lc_write;
        original->external_pointer = lc_pointer;
        original->external_user_data = cache;
        original->gpr[1] = STACK;
        original->gpr[3] = LC_BASE;
        original->gpr[4] = FACTORS;
        original->gpr[5] = LC_BASE + ((trial & 1u) ? 0u : 0x80u);
        original->lr = 0x81234560u;
        original->msr = PPC_MSR_FP;
        original->hid2 = PPC_HID2_LSQE;
        original->fpscr = (trial & 1u) ? 0xA2308104u : 0u;
        for (unsigned reg = 14; reg <= 15; ++reg) {
            original->fpr[reg] = (int32_t)random_word(&rng) / 65536.0;
            original->ps1[reg] = (int32_t)random_word(&rng) / 65536.0;
        }
        original->fpr[31] = (int32_t)random_word(&rng) / 65536.0;
        original->ps1[31] = (int32_t)random_word(&rng) / 65536.0;
        for (unsigned i = 0; i < 12; ++i) {
            const u32 input = random_float_bits(&rng);
            const u32 factor = random_float_bits(&rng);
            word(cache, 4u * i, input);
            word(candidate_cache, 4u * i, input);
            word(ram, FACTORS - GC_RAM_BASE + 4u * i, factor);
            word(candidate_ram, FACTORS - GC_RAM_BASE + 4u * i, factor);
        }
        for (unsigned i = 0; i < 2; ++i) {
            const u32 constant = random_float_bits(&rng);
            word(ram, CONSTANT - GC_RAM_BASE + 4u * i, constant);
            word(candidate_ram, CONSTANT - GC_RAM_BASE + 4u * i, constant);
        }
        for (unsigned i = 0; i < 64; ++i) {
            const u8 value = (u8)random_word(&rng);
            ram[STACK - GC_RAM_BASE - 64u + i] = value;
            candidate_ram[STACK - GC_RAM_BASE - 64u + i] = value;
        }
        *candidate = *original;
        candidate->ram = candidate_ram;
        candidate->external_user_data = candidate_cache;
        if (!nflstreet_paired_product_cpu_candidate(candidate)) {
            ++skips;
            continue;
        }
        if (module->dispatch(original, ENTRY) != 1 || original->exception ||
            original->pc != original->lr) {
            ++dispatch_failures;
            continue;
        }
        CPUState a = *original, b = *candidate;
        a.ram = b.ram = NULL;
        a.external_user_data = b.external_user_data = NULL;
        if (memcmp(&a, &b, sizeof(a)) != 0 ||
            memcmp(ram + STACK - GC_RAM_BASE - 64u,
                   candidate_ram + STACK - GC_RAM_BASE - 64u, 48u) != 0 ||
            memcmp(cache, candidate_cache, 0xC0u) != 0) {
            if (mismatches < 5)
                fprintf(stderr, "mismatch trial=%u target=%08x fpscr=%08x/%08x\n",
                        trial, original->gpr[5], original->fpscr, candidate->fpscr);
            ++mismatches;
        } else {
            ++matches;
        }
    }
    printf("locked_cache_trials=10000 matches=%u skips=%u mismatches=%u "
           "dispatch_failures=%u\n", matches, skips, mismatches, dispatch_failures);
    free(candidate_cache);
    free(cache);
    free(candidate_ram);
    free(ram);
    free(candidate);
    free(original);
    dlclose(library);
    return (matches != 10000 || skips || mismatches || dispatch_failures) ? 1 : 0;
}
