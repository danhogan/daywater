#include "full_state_candidate.h"

#include <math.h>
#include <string.h>

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

static Pair load_pair(CPUState* ctx, u32 address)
{
    return (Pair){from_bits(mem_read32(ctx, address)),
                  from_bits(mem_read32(ctx, address + 4u))};
}

static void write_pair(CPUState* ctx, unsigned reg, Pair value)
{
    ctx->fpr[reg] = value.lo;
    ctx->ps1[reg] = value.hi;
}

static Pair pair_mul(Pair left, Pair right)
{
    return (Pair){left.lo * right.lo, left.hi * right.hi};
}

static Pair pair_sum0(Pair value)
{
    return (Pair){value.lo + value.hi, value.hi};
}

static Pair pair_madd(Pair left, Pair right, Pair accumulator)
{
    return (Pair){fmaf(left.lo, right.lo, accumulator.lo),
                  fmaf(left.hi, right.hi, accumulator.hi)};
}

bool nflstreet_three_pair_cpu_candidate(CPUState* ctx)
{
    const u32 source = ctx->gpr[3];
    const u32 factors = ctx->gpr[4];
    const u32 target = ctx->gpr[5];
    if (!ctx->ram || ctx->ram_size < 48 ||
        !accessible(ctx, source, 48) || !accessible(ctx, factors, 12) ||
        !accessible(ctx, target, 12) ||
        (ctx->msr & PPC_MSR_FP) == 0 || (ctx->hid2 & PPC_HID2_LSQE) == 0 ||
        ctx->gqr[0] != 0 || ctx->exception != 0)
        return false;

    Pair inputs[6];
    for (unsigned i = 0; i < 6; ++i)
        inputs[i] = load_pair(ctx, source + 8u * i);
    const Pair first = load_pair(ctx, factors);
    const Pair second = {from_bits(mem_read32(ctx, factors + 8u)), 1.0f};
    bool all_normal = valid_pair(first) && valid_pair(second);
    for (unsigned i = 0; i < 6; ++i)
        all_normal = all_normal && valid_pair(inputs[i]);

    Pair products[3], sums[3], results[3];
    for (unsigned i = 0; i < 3; ++i) {
        products[i] = pair_mul(inputs[2u * i], first);
        sums[i] = pair_sum0(products[i]);
        results[i] = pair_madd(inputs[2u * i + 1u], second, sums[i]);
        all_normal = all_normal && valid_pair(products[i]) &&
                     valid_pair(sums[i]) && valid_pair(results[i]);
    }
    if (!all_normal)
        return false;

    ctx->pc = 0x801FFF10u;
    ctx->downcount -= 21;
    write_pair(ctx, 0, inputs[0]);
    write_pair(ctx, 1, inputs[1]);
    write_pair(ctx, 2, inputs[2]);
    write_pair(ctx, 3, inputs[3]);
    write_pair(ctx, 4, inputs[4]);
    write_pair(ctx, 5, inputs[5]);
    write_pair(ctx, 6, first);
    write_pair(ctx, 7, second);
    write_pair(ctx, 8, sums[0]);
    write_pair(ctx, 9, results[0]);
    write_pair(ctx, 10, sums[1]);
    write_pair(ctx, 11, results[1]);
    write_pair(ctx, 12, sums[2]);
    write_pair(ctx, 13, results[2]);
    ctx->fpscr = (ctx->fpscr & ~(0x1Fu << 12)) | (classify(results[2].lo) << 12);
    mem_write32(ctx, target, to_bits(results[0].lo));
    mem_write32(ctx, target + 4u, to_bits(results[1].lo));
    mem_write32(ctx, target + 8u, to_bits(results[2].lo));
    ctx->pc = ctx->lr & ~3u;
    return true;
}
