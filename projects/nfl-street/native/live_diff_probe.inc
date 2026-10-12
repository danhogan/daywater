/* Experimental generated-chunk include. Never ship in the default module. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct NFLStreetLiveProbe {
    CPUState candidate;
    u8* shadow;
    u8* locked_cache;
    u8* locked_cache_shadow;
    u32 stack;
    u32 stack_size;
    u32 output_a;
    u32 output_a_size;
    u32 output_b;
    u32 output_b_size;
    bool active;
} NFLStreetLiveProbe;

static void* nflstreet_probe_external_pointer(CPUState* ctx, u32 address, u32 size)
{
    NFLStreetLiveProbe* probe = (NFLStreetLiveProbe*)ctx->external_user_data;
    const u32 base = 0xE0000000u;
    const u32 capacity = 0x00040000u;
    if (address < base || size > capacity || address - base > capacity - size)
        return NULL;
    return probe->locked_cache_shadow + (address - base);
}

static u64 nflstreet_probe_external_read(CPUState* ctx, u32 address, u8 size)
{
    const u8* bytes = (const u8*)nflstreet_probe_external_pointer(ctx, address, size);
    u64 value = 0;
    if (!bytes)
        return 0;
    for (u8 i = 0; i < size; ++i)
        value = (value << 8) | bytes[i];
    return value;
}

static void nflstreet_probe_external_write(CPUState* ctx, u32 address,
                                             u64 value, u8 size)
{
    u8* bytes = (u8*)nflstreet_probe_external_pointer(ctx, address, size);
    if (!bytes)
        return;
    for (u8 i = 0; i < size; ++i)
        bytes[size - 1u - i] = (u8)(value >> (8u * i));
}

static bool nflstreet_live_probe_prepare(NFLStreetLiveProbe* probe, CPUState* ctx,
                                          bool (*candidate)(CPUState*),
                                          u32 stack, u32 stack_size,
                                          u32 output_a, u32 output_a_size,
                                          u32 output_b, u32 output_b_size)
{
    static u8* shadow;
    static u8* cache_shadow;
    static u32 capacity;
    probe->active = false;
    if (!ctx->ram || ctx->ram_size == 0 || ctx->ram_size > 32u * 1024u * 1024u)
        return false;
    if (capacity < ctx->ram_size) {
        u8* replacement = (u8*)realloc(shadow, ctx->ram_size);
        if (!replacement)
            return false;
        shadow = replacement;
        capacity = ctx->ram_size;
    }
    memcpy(shadow, ctx->ram, ctx->ram_size);
    u8* cache = NULL;
    if (ctx->external_pointer)
        cache = (u8*)ctx->external_pointer(ctx, 0xE0000000u, 0x00040000u);
    if (cache && !cache_shadow)
        cache_shadow = (u8*)malloc(0x00040000u);
    if (cache && !cache_shadow)
        return false;
    if (cache)
        memcpy(cache_shadow, cache, 0x00040000u);
    probe->locked_cache = cache;
    probe->locked_cache_shadow = cache_shadow;
    probe->candidate = *ctx;
    probe->candidate.ram = shadow;
    if (cache) {
        probe->candidate.external_read = nflstreet_probe_external_read;
        probe->candidate.external_write = nflstreet_probe_external_write;
        probe->candidate.external_pointer = nflstreet_probe_external_pointer;
        probe->candidate.external_user_data = probe;
    }
    PPCMemWriteJournal saved_journal = g_mem_write_journal;
    void* saved_user = g_mem_write_journal_user;
    g_mem_write_journal = NULL;
    g_mem_write_journal_user = NULL;
    const bool accepted = candidate(&probe->candidate);
    g_mem_write_journal = saved_journal;
    g_mem_write_journal_user = saved_user;
    if (!accepted)
        return false;
    probe->shadow = shadow;
    probe->stack = stack;
    probe->stack_size = stack_size;
    probe->output_a = output_a;
    probe->output_a_size = output_a_size;
    probe->output_b = output_b;
    probe->output_b_size = output_b_size;
    probe->active = true;
    return true;
}

static bool nflstreet_live_probe_range_equal(const NFLStreetLiveProbe* probe,
                                              const CPUState* ctx, u32 address, u32 size)
{
    if (!size)
        return true;
    const u32 offset = address - GC_RAM_BASE;
    if (address >= GC_RAM_BASE && size <= ctx->ram_size &&
        offset <= ctx->ram_size - size)
        return memcmp(probe->shadow + offset, ctx->ram + offset, size) == 0;
    if (address >= 0xE0000000u && size <= 0x00040000u &&
        address - 0xE0000000u <= 0x00040000u - size &&
        probe->locked_cache && probe->locked_cache_shadow) {
        const u32 cache_offset = address - 0xE0000000u;
        return memcmp(probe->locked_cache_shadow + cache_offset,
                      probe->locked_cache + cache_offset, size) == 0;
    }
    return false;
}

static bool nflstreet_live_probe_equal(NFLStreetLiveProbe* probe, CPUState* ctx,
                                       u32 pc, unsigned sequence)
{
    probe->candidate.ram = ctx->ram;
    probe->candidate.external_read = ctx->external_read;
    probe->candidate.external_write = ctx->external_write;
    probe->candidate.external_pointer = ctx->external_pointer;
    probe->candidate.external_user_data = ctx->external_user_data;
    const bool cpu_equal = memcmp(&probe->candidate, ctx, sizeof(*ctx)) == 0;
    const bool stack_equal = nflstreet_live_probe_range_equal(
        probe, ctx, probe->stack, probe->stack_size);
    const bool a_equal = nflstreet_live_probe_range_equal(
        probe, ctx, probe->output_a, probe->output_a_size);
    const bool b_equal = nflstreet_live_probe_range_equal(
        probe, ctx, probe->output_b, probe->output_b_size);
    if (!cpu_equal || !stack_equal || !a_equal || !b_equal) {
        size_t cpu_offset = 0;
        const u8* a = (const u8*)&probe->candidate;
        const u8* b = (const u8*)ctx;
        while (cpu_offset < sizeof(*ctx) && a[cpu_offset] == b[cpu_offset])
            ++cpu_offset;
        fprintf(stderr,
                "[nflstreet-live-diff] mismatch pc=%08x seq=%u cpu_offset=%zu "
                "stack=%u out_a=%u out_b=%u candidate_pc=%08x original_pc=%08x "
                "candidate_fpscr=%08x original_fpscr=%08x\n",
                pc, sequence, cpu_offset, stack_equal, a_equal, b_equal,
                probe->candidate.pc, ctx->pc, probe->candidate.fpscr, ctx->fpscr);
        return false;
    }
    return true;
}
