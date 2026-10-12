#include "full_state_candidate.h"

#include <math.h>
#include <string.h>
#ifdef NFLSTREET_FASTPATH_DIAGNOSTIC
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

static Pair pair_mul(Pair a, Pair b)
{
    return (Pair){a.lo * b.lo, a.hi * b.hi};
}

static Pair pair_muls(Pair a, float scalar)
{
    return (Pair){a.lo * scalar, a.hi * scalar};
}

static Pair pair_madd(Pair a, Pair c, Pair b)
{
    return (Pair){fmaf(a.lo, c.lo, b.lo), fmaf(a.hi, c.hi, b.hi)};
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

bool nflstreet_odd_polynomial_cpu_candidate(CPUState* ctx)
{
#ifdef NFLSTREET_FASTPATH_DIAGNOSTIC
    static bool reported_first_attempt;
    if (!reported_first_attempt) {
        reported_first_attempt = true;
        fprintf(stderr,
                "[nflstreet-fastpath] first-attempt msr=%08x hid2=%08x "
                "gqr0=%08x gqr6=%08x fpscr=%08x exception=%08x "
                "sp=%08x r4=%08x r5=%08x r6=%08x ram_size=%u\n",
                ctx->msr, ctx->hid2, ctx->gqr[0], ctx->gqr[6], ctx->fpscr,
                ctx->exception, ctx->gpr[1], ctx->gpr[4], ctx->gpr[5],
                ctx->gpr[6], ctx->ram_size);
    }
#endif
    const u32 old_sp = ctx->gpr[1];
    const u32 new_sp = old_sp - 16u;
    if (!ctx->ram || ctx->ram_size < 32 ||
        !in_ram(ctx, new_sp, 4) || !in_ram(ctx, ctx->gpr[4], 24) ||
        !in_ram(ctx, ctx->gpr[5], 4) || !in_ram(ctx, ctx->gpr[6], 4) ||
        (new_sp < ctx->gpr[4] + 24u && ctx->gpr[4] < new_sp + 4u) ||
        (ctx->msr & PPC_MSR_FP) == 0 || (ctx->hid2 & PPC_HID2_LSQE) == 0 ||
        ctx->gqr[0] != 0 || ctx->gqr[6] != 0x0F070007u ||
        ctx->exception != 0)
        return false;

    const u32 r3 = ctx->gpr[3];
    const u32 r7 = r3 + 0x4000u;
    Pair f1 = {(float)(s16)r3 / 32768.0f, (float)(s16)r7 / 32768.0f};
    bool all_normal = valid_pair(f1);
    Pair f5 = pair_mul(f1, f1);
    all_normal = all_normal && valid_pair(f5);
    Pair f2 = {from_bits(mem_read32(ctx, ctx->gpr[4])),
               from_bits(mem_read32(ctx, ctx->gpr[4] + 4u))};
    Pair f3 = {from_bits(mem_read32(ctx, ctx->gpr[4] + 8u)),
               from_bits(mem_read32(ctx, ctx->gpr[4] + 12u))};
    Pair f6 = pair_mul(f1, f5);
    all_normal = all_normal && valid_pair(f6);
    f5 = pair_mul(f5, f5);
    all_normal = all_normal && valid_pair(f5);
    const Pair f7 = {f1.lo, f6.lo};
    const Pair f11 = {f1.hi, f6.hi};
    Pair f4 = {from_bits(mem_read32(ctx, ctx->gpr[4] + 16u)),
               from_bits(mem_read32(ctx, ctx->gpr[4] + 20u))};
    const Pair f8 = pair_muls(f7, f5.lo);
    const Pair f12 = pair_muls(f11, f5.hi);
    all_normal = all_normal && valid_pair(f8) && valid_pair(f12);
    f1 = pair_mul(f2, f7);
    Pair f0 = pair_mul(f2, f11);
    all_normal = all_normal && valid_pair(f1) && valid_pair(f0);
    const Pair f9 = pair_muls(f8, f5.lo);
    const Pair f13 = pair_muls(f12, f5.hi);
    all_normal = all_normal && valid_pair(f9) && valid_pair(f13);
    f1 = pair_madd(f3, f8, f1);
    f0 = pair_madd(f3, f12, f0);
    all_normal = all_normal && valid_pair(f1) && valid_pair(f0);
    f1 = pair_madd(f4, f9, f1);
    f0 = pair_madd(f4, f13, f0);
    all_normal = all_normal && valid_pair(f1) && valid_pair(f0);
    f1.lo = f1.lo + f1.hi;
    f0.lo = f0.lo + f0.hi;

    const Pair values[] = {f0, f1, f2, f3, f4, f5, f6, f7, f8,
                           f9, f11, f12, f13};
    if (!all_normal)
        return false;
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        if (!valid_pair(values[i]))
            return false;
    }

    ctx->pc = 0x80188BB0u;
    ctx->downcount -= 29;
    mem_write32(ctx, new_sp, old_sp);
    ctx->gpr[1] = new_sp;
    ctx->gpr[7] = r7;
    mem_write16(ctx, new_sp, (u16)r3);
    mem_write16(ctx, new_sp + 2u, (u16)r7);
    write_pair(ctx, 0, f0);
    write_pair(ctx, 1, f1);
    write_pair(ctx, 2, f2);
    write_pair(ctx, 3, f3);
    write_pair(ctx, 4, f4);
    write_pair(ctx, 5, f5);
    write_pair(ctx, 6, f6);
    write_pair(ctx, 7, f7);
    write_pair(ctx, 8, f8);
    write_pair(ctx, 9, f9);
    write_pair(ctx, 11, f11);
    write_pair(ctx, 12, f12);
    write_pair(ctx, 13, f13);
    ctx->fpscr = (ctx->fpscr & ~(0x1Fu << 12)) | (classify(f0.lo) << 12);
    ctx->gpr[1] = old_sp;
    mem_write32(ctx, ctx->gpr[5], to_bits(f1.lo));
    mem_write32(ctx, ctx->gpr[6], to_bits(f0.lo));
    ctx->pc = ctx->lr & ~3u;
#ifdef NFLSTREET_FASTPATH_DIAGNOSTIC
    static bool reported_first_hit;
    if (!reported_first_hit) {
        reported_first_hit = true;
        fprintf(stderr, "[nflstreet-fastpath] first-hit pc=80188bb0\n");
    }
#endif
    return true;
}
