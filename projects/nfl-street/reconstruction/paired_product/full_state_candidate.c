#include "paired_product_full_state_candidate.h"

#include <math.h>
#include <string.h>
#if defined(NFLSTREET_PRODUCT_DIAGNOSTIC) || defined(NFLSTREET_PRODUCT_GUARD_DIAGNOSTIC)
#include <stdio.h>
#endif

typedef struct Pair {
    float lo;
    float hi;
} Pair;

static float from_bits(u32 bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 to_bits(float value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u64 double_bits(double value)
{
    u64 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static Pair pair_muls(Pair a, float scalar)
{
    return (Pair){a.lo * scalar, a.hi * scalar};
}

static Pair pair_madds(Pair a, float scalar, Pair b)
{
    return (Pair){fmaf(a.lo, scalar, b.lo), fmaf(a.hi, scalar, b.hi)};
}

static void write_pair(CPUState* ctx, unsigned reg, Pair value)
{
    ctx->fpr[reg] = value.lo;
    ctx->ps1[reg] = value.hi;
}

static bool normal_or_zero(float value)
{
    return isfinite(value) && (value == 0.0f || fabsf(value) >= 0x1p-126f);
}

static bool valid_pair(Pair value)
{
    return normal_or_zero(value.lo) && normal_or_zero(value.hi);
}

static u32 classify(float value)
{
    const u32 bits = to_bits(value);
    const u32 sign = bits >> 31;
    const u32 exponent = bits & 0x7F800000u;
    const u32 fraction = bits & 0x007FFFFFu;
    if (exponent == 0x7F800000u)
        return fraction ? 0x11u : (sign ? 0x09u : 0x05u);
    if (exponent == 0)
        return fraction ? (sign ? 0x18u : 0x14u) : (sign ? 0x12u : 0x02u);
    return sign ? 0x08u : 0x04u;
}

static bool in_ram(const CPUState* ctx, u32 address, u32 size)
{
    return address >= GC_RAM_BASE && address - GC_RAM_BASE <= ctx->ram_size - size;
}

static bool in_locked_cache(CPUState* ctx, u32 address, u32 size)
{
    const u32 base = 0xE0000000u;
    const u32 capacity = 0x00040000u;
    return address >= base && size <= capacity &&
           address - base <= capacity - size &&
           ctx->external_read && ctx->external_write && ctx->external_pointer &&
           ctx->external_pointer(ctx, address, size) != NULL;
}

static bool accessible(CPUState* ctx, u32 address, u32 size)
{
    return in_ram(ctx, address, size) || in_locked_cache(ctx, address, size);
}

static bool overlaps(u32 a, u32 a_size, u32 b, u32 b_size)
{
    return a < b + b_size && b < a + a_size;
}

static Pair load_pair(CPUState* ctx, u32 address)
{
    return (Pair){from_bits(mem_read32(ctx, address)),
                  from_bits(mem_read32(ctx, address + 4u))};
}

static void store_pair(CPUState* ctx, u32 address, Pair value)
{
    mem_write32(ctx, address, to_bits(value.lo));
    mem_write32(ctx, address + 4u, to_bits(value.hi));
}

bool nflstreet_paired_product_cpu_candidate(CPUState* ctx)
{
    const u32 old_sp = ctx->gpr[1];
    const u32 new_sp = old_sp - 64u;
    const u32 source = ctx->gpr[3];
    const u32 factors = ctx->gpr[4];
    const u32 target = ctx->gpr[5];
    const u32 constant_address = 0x803A3958u;
    if (!ctx->ram || ctx->ram_size < 64 ||
        !in_ram(ctx, new_sp, 48) || !accessible(ctx, source, 48) ||
        !accessible(ctx, factors, 48) || !accessible(ctx, target, 48) ||
        !in_ram(ctx, constant_address, 8) ||
        overlaps(new_sp, 48, source, 48) ||
        overlaps(new_sp, 48, factors, 48) ||
        overlaps(new_sp, 48, target, 48) ||
        overlaps(new_sp, 48, constant_address, 8) ||
        (ctx->msr & PPC_MSR_FP) == 0 || (ctx->hid2 & PPC_HID2_LSQE) == 0 ||
        ctx->gqr[0] != 0 || ctx->exception != 0) {
#ifdef NFLSTREET_PRODUCT_GUARD_DIAGNOSTIC
        fprintf(stderr,
                "[nflstreet-product-guard] precondition sp=%08x r3=%08x "
                "r4=%08x r5=%08x gqr0=%08x exception=%08x\n",
                old_sp, source, factors, target, ctx->gqr[0], ctx->exception);
#endif
        return false;
    }

    Pair a[6], b[6];
    for (unsigned i = 0; i < 6; ++i) {
        a[i] = load_pair(ctx, source + 8u * i);
        b[i] = load_pair(ctx, factors + 8u * i);
    }
    const Pair k = load_pair(ctx, constant_address);
    bool all_normal = valid_pair(k);
    for (unsigned i = 0; i < 6; ++i)
        all_normal = all_normal && valid_pair(a[i]) && valid_pair(b[i]);
#ifdef NFLSTREET_PRODUCT_GUARD_DIAGNOSTIC
    if (!all_normal) {
        fprintf(stderr, "[nflstreet-product-guard] input\n");
        return false;
    }
#endif

    Pair f12 = pair_muls(b[0], a[0].lo);
    Pair f13 = pair_muls(b[1], a[0].lo);
    Pair f14 = pair_muls(b[0], a[2].lo);
    Pair f15 = pair_muls(b[1], a[2].lo);
    all_normal = all_normal && valid_pair(f12) && valid_pair(f13) &&
                 valid_pair(f14) && valid_pair(f15);
    f12 = pair_madds(b[2], a[0].hi, f12);
    f14 = pair_madds(b[2], a[2].hi, f14);
    f13 = pair_madds(b[3], a[0].hi, f13);
    f15 = pair_madds(b[3], a[2].hi, f15);
    all_normal = all_normal && valid_pair(f12) && valid_pair(f13) &&
                 valid_pair(f14) && valid_pair(f15);
    f12 = pair_madds(b[4], a[1].lo, f12);
    f13 = pair_madds(b[5], a[1].lo, f13);
    f14 = pair_madds(b[4], a[3].lo, f14);
    f15 = pair_madds(b[5], a[3].lo, f15);
    all_normal = all_normal && valid_pair(f12) && valid_pair(f13) &&
                 valid_pair(f14) && valid_pair(f15);

    Pair f2 = pair_muls(b[0], a[4].lo);
    f13 = pair_madds(k, a[1].hi, f13);
    Pair f0 = pair_muls(b[1], a[4].lo);
    f15 = pair_madds(k, a[3].hi, f15);
    all_normal = all_normal && valid_pair(f2) && valid_pair(f0) &&
                 valid_pair(f13) && valid_pair(f15);
    f2 = pair_madds(b[2], a[4].hi, f2);
    f0 = pair_madds(b[3], a[4].hi, f0);
    all_normal = all_normal && valid_pair(f2) && valid_pair(f0);
    f2 = pair_madds(b[4], a[5].lo, f2);
    f0 = pair_madds(b[5], a[5].lo, f0);
    all_normal = all_normal && valid_pair(f2) && valid_pair(f0);
    f0 = pair_madds(k, a[5].hi, f0);
    if (!all_normal || !valid_pair(f0)) {
#ifdef NFLSTREET_PRODUCT_GUARD_DIAGNOSTIC
        fprintf(stderr, "[nflstreet-product-guard] arithmetic\n");
#endif
        return false;
    }

    const double old_f14 = ctx->fpr[14];
    const double old_f15 = ctx->fpr[15];
    const double old_f31 = ctx->fpr[31];
    ctx->pc = 0x801FFDF0u;
    ctx->downcount -= 51;
    mem_write32(ctx, new_sp, old_sp);
    ctx->gpr[1] = new_sp;
    mem_write64(ctx, new_sp + 8u, double_bits(old_f14));
    mem_write64(ctx, new_sp + 16u, double_bits(old_f15));
    mem_write64(ctx, new_sp + 40u, double_bits(old_f31));
    ctx->gpr[6] = constant_address;
    write_pair(ctx, 0, f0);
    write_pair(ctx, 1, a[1]);
    write_pair(ctx, 2, f2);
    write_pair(ctx, 3, a[3]);
    write_pair(ctx, 4, a[4]);
    write_pair(ctx, 5, a[5]);
    for (unsigned i = 0; i < 6; ++i)
        write_pair(ctx, 6u + i, b[i]);
    write_pair(ctx, 12, f12);
    write_pair(ctx, 13, f13);
    write_pair(ctx, 14, f14);
    write_pair(ctx, 15, f15);
    write_pair(ctx, 31, k);
    ctx->fpr[14] = old_f14;
    ctx->fpr[15] = old_f15;
    ctx->fpr[31] = old_f31;
    ctx->fpscr = (ctx->fpscr & ~(0x1Fu << 12)) | (classify(f0.lo) << 12);
    store_pair(ctx, target, f12);
    store_pair(ctx, target + 16u, f14);
    store_pair(ctx, target + 8u, f13);
    store_pair(ctx, target + 24u, f15);
    store_pair(ctx, target + 32u, f2);
    store_pair(ctx, target + 40u, f0);
    ctx->gpr[1] = old_sp;
    ctx->pc = ctx->lr & ~3u;
#ifdef NFLSTREET_PRODUCT_DIAGNOSTIC
    static bool reported_first_hit;
    if (!reported_first_hit) {
        reported_first_hit = true;
        fprintf(stderr, "[nflstreet-fastpath] first-hit pc=801ffdf0\n");
    }
#endif
    return true;
}
