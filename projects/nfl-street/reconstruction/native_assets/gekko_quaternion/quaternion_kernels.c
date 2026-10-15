#include "quaternion_kernels.h"

#include <string.h>

#if defined(_MSC_VER)
#define PPC_NOINLINE __declspec(noinline)
#elif defined(__clang__)
#define PPC_NOINLINE __attribute__((noinline))
#else
#define PPC_NOINLINE __attribute__((noinline, noclone))
#endif

static float ppc_fmadds(float left, float right, float addend);

typedef struct {
    float lo;
    float hi;
} pair;

static pair mul(pair a, pair b)
{
    return (pair){a.lo * b.lo, a.hi * b.hi};
}

static pair muls(pair a, float scalar)
{
    return (pair){a.lo * scalar, a.hi * scalar};
}

static pair madd(pair a, pair b, pair c)
{
    return (pair){ppc_fmadds(a.lo, b.lo, c.lo), ppc_fmadds(a.hi, b.hi, c.hi)};
}

void nflstreet_80188bb0_pair(uint16_t angle, const float coefficients[6],
    float result[2])
{
    const uint16_t next = (uint16_t)(angle + 0x4000u);
    pair f1 = {(float)(int16_t)angle * 0x1p-15f,
               (float)(int16_t)next * 0x1p-15f};
    pair f5 = mul(f1, f1);                     /* ps_mul f5,f1,f1 */
    const pair f2 = {coefficients[0], coefficients[1]};
    const pair f3 = {coefficients[2], coefficients[3]};
    const pair f6 = mul(f1, f5);               /* t^3 */
    f5 = mul(f5, f5);                          /* t^4 */
    const pair f7 = {f1.lo, f6.lo};
    const pair f11 = {f1.hi, f6.hi};
    const pair f4 = {coefficients[4], coefficients[5]};
    const pair f8 = muls(f7, f5.lo);
    const pair f12 = muls(f11, f5.hi);
    f1 = mul(f2, f7);
    pair f0 = mul(f2, f11);
    const pair f9 = muls(f8, f5.lo);
    const pair f13 = muls(f12, f5.hi);
    f1 = madd(f3, f8, f1);
    f0 = madd(f3, f12, f0);
    f1 = madd(f4, f9, f1);
    f0 = madd(f4, f13, f0);
    result[0] = f1.lo + f1.hi;                 /* ps_sum0 */
    result[1] = f0.lo + f0.hi;
}

static int32_t signed24(int32_t value)
{
    value &= 0x00ffffff;
    return value & 0x00800000 ? value | (int32_t)0xff000000 : value;
}

float nflstreet_8018889c_sine(int32_t angle24, int cosine,
    const float coefficients[8])
{
    /* 80188A70/80188AEC form the signed integer with rlwinm/srawi, then
       convert through the 4330 double trick and execute frsp before this
       routine's first fmuls.  Every 24-bit integer is exactly representable
       as a single, so the C conversion has the same value at that point. */
    int32_t raw = signed24(angle24);
    if (cosine) raw = signed24(raw + 0x400000);
    const float t = (float)raw * 0x1p-23f;
    pair f2 = {t, t};                         /* fmuls f2,f1,f2 */
    pair f7 = mul(f2, f2);
    const pair f3 = {coefficients[0], coefficients[1]};
    const pair f4 = {coefficients[2], coefficients[3]};
    const pair f5 = {coefficients[4], coefficients[5]};
    const pair f6 = {coefficients[6], coefficients[7]};
    const pair f9 = mul(f2, f7);
    f7 = mul(f7, f7);
    const pair f8 = {f2.lo, f9.hi};           /* ps_merge01 */
    const pair f10 = mul(f8, f7);
    pair f1 = mul(f3, f8);
    const pair f11 = mul(f10, f7);
    f1 = madd(f4, f10, f1);
    const pair f12 = mul(f11, f7);
    f1 = madd(f5, f11, f1);
    f1 = madd(f6, f12, f1);
    return f1.lo + f1.hi;                     /* ps_sum0 */
}

/* Every constant below was read as its BE IEEE-754 word from the saved DOL
   image (r13=803A9A40).  The instructions are f* s, not host double math. */
static float msl_f32(uint32_t word)
{
    float value;
    memcpy(&value, &word, sizeof value);
    return value;
}

static uint32_t msl_bits(float value)
{
    uint32_t word;
    memcpy(&word, &value, sizeof word);
    return word;
}

/* ppc_fdivs first divides the (double-extended) FPR operands, then writes a
   single-precision result.  A native float division is permitted to perform
   a different intermediate operation, so retain the explicit rounding point. */
static PPC_NOINLINE float ppc_fdivs(float numerator, float denominator)
{
    volatile double quotient = (double)numerator / (double)denominator;
    return (float)quotient;
}

/* Like fdivs, fmuls operates on double FPR operands and then rounds the
   result to single precision.  Spell out the FPR operation: native float
   multiplication selects a different last bit on this host for a real tie
   reached by 801A383C's first Slerp weight. */
static PPC_NOINLINE float ppc_fmuls(float left, float right)
{
    volatile double product = (double)left * (double)right;
    return (float)product;
}

static PPC_NOINLINE float ppc_fsubs(float left, float right)
{
    volatile double difference = (double)left - (double)right;
    return (float)difference;
}

static PPC_NOINLINE float ppc_fadds(float left, float right)
{
    volatile double sum = (double)left + (double)right;
    return (float)sum;
}

static uint64_t msl_bits64(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static double msl_f64(uint64_t bits)
{
    double value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

/* This is DolRecomp's single_fma_value/correct_single_fma_tie rule, reduced
   to the finite normal values reached by the quaternion routines.  Gekko
   single FMAs select the adjacent IEEE value for an exact halfway result in
   cases where a host FMA's usual tie-to-even choice does not. */
static PPC_NOINLINE float ppc_fmadds(float left, float right, float addend)
{
    /* A product of two binary32 values is exact in binary64 (at most 48
       significant bits).  The following binary64 add therefore supplies the
       fused binary32 result without using host libm. */
    volatile double product = (double)left * (double)right;
    double result = product + (double)addend;
    uint64_t bits = msl_bits64(result);
    if ((bits & UINT64_C(0x000000001fffffff)) == UINT64_C(0x0000000010000000)) {
        const double left_over = (double)addend - result;
        const double rounded_back = result + left_over;
        const double error = (product + left_over) + ((double)addend - rounded_back);
        if (error != 0.0)
            result = msl_f64((error > 0.0) == (result > 0.0) ? bits + 1u : bits - 1u);
    }
    return (float)result;
}

/* 802165C8.  This is the MSL restoring square-root loop, including its
   final tie-to-even increment; it is deliberately integer-only. */
/* The quiet NaN the verified builds produce for 0/0 (0x7FC00000), spelled
   out so every compiler returns the same bits. */
static float msl_quiet_nan(void)
{
    const uint32_t bits = 0x7fc00000u;
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static float msl_sqrt(float x)
{
    uint32_t ix = msl_bits(x);
    if ((ix & 0x7fffffffu) == 0) return x;
    if (ix & 0x80000000u) return msl_quiet_nan();
    int m = (int)(ix >> 23) - 127;
    ix = (ix & 0x007fffffu) | 0x00800000u;
    if (m & 1) ix += ix;
    m >>= 1;
    ix += ix;
    uint32_t q = 0, s = 0, r = 0x01000000u;
    do {
        const uint32_t t = s + r;
        if (t <= ix) {
            s = t + r;
            ix -= t;
            q += r;
        }
        ix += ix;
        r >>= 1;
    } while (r != 0);
    if (ix != 0) q += q & 1u;
    return msl_f32((q >> 1) + 0x3f000000u + ((uint32_t)m << 23));
}

/* 80215844, the MSL acosf entry used by 80188D08.  801A383C supplies a
   finite cosine in [0,1], which reaches precisely these two source paths. */
static float msl_acos(float x)
{
    const float p1 = msl_f32(0x3811ef08u), p2 = msl_f32(0x3a4f7f04u);
    const float p3 = msl_f32(0xbd241146u), p4 = msl_f32(0x3e4e0aa8u);
    const float p5 = msl_f32(0xbea6b090u), p6 = msl_f32(0x3e2aaaabu);
    const float q1 = msl_f32(0x3d9dc62eu), q2 = msl_f32(0xbf303361u);
    const float q3 = msl_f32(0x4001572du), q4 = msl_f32(0xc019d139u);
    const float one = msl_f32(0x3f800000u);
    const float half = msl_f32(0x3f000000u);
    const float tiny = msl_f32(0x33a22168u);
    const float pio2 = msl_f32(0x3fc90fdau);
    if (x <= 0.5f) {
        const float z = x * x;
        float p = ppc_fmadds(z, p1, p2);
        p = ppc_fmadds(z, p, p3); p = ppc_fmadds(z, p, p4);
        p = ppc_fmadds(z, p, p5); p = ppc_fmadds(z, p, p6);
        float q = ppc_fmadds(z, q1, q2);
        q = ppc_fmadds(z, q, q3); q = ppc_fmadds(z, q, q4);
        const float r = ppc_fdivs(ppc_fmuls(z, p), ppc_fmadds(z, q, one));
        return pio2 - (x - (tiny - x * r));
    }
    const float z = (one - x) * half;
    const float s = msl_sqrt(z);
    const float df = msl_f32(msl_bits(s) & 0xfffff000u);
    const float c = (z - df * df) / (s + df);
    float p = ppc_fmadds(z, p1, p2);
    p = ppc_fmadds(z, p, p3); p = ppc_fmadds(z, p, p4);
    p = ppc_fmadds(z, p, p5); p = ppc_fmadds(z, p, p6);
    float q = ppc_fmadds(z, q1, q2);
    q = ppc_fmadds(z, q, q3); q = ppc_fmadds(z, q, q4);
    const float r = ppc_fdivs(ppc_fmuls(z, p), ppc_fmadds(z, q, one));
    const float w = ppc_fmadds(r, s, c);
    const float result = df + w;
    return result + result;
}

float nflstreet_80215844_acos(float x)
{
    return msl_acos(x);
}

float nflstreet_802165c8_sqrt(float x)
{
    return msl_sqrt(x);
}

/* 80188D98.  The two operands are the PPC f1/f2 lanes, respectively.  Its
   result has the same signed-24 wrapping as the rlwinm/srawi tail. */
static int32_t msl_atan2_angle(float y, float x)
{
    const float zero = msl_f32(0x00000000u);
    const float scale = msl_f32(0x4a22f983u);
    const float coefficients[] = {
        msl_f32(0x3f7ffff5u), msl_f32(0xbeaaa61cu),
        msl_f32(0x3e4c40a6u), msl_f32(0xbe0e6c63u),
        msl_f32(0x3dc577dfu), msl_f32(0xbd6501c4u),
        msl_f32(0x3cb31652u), msl_f32(0xbb84d7e7u),
        msl_f32(0x3f490fdbu),
    };
    const float ay = msl_f32(msl_bits(y) & 0x7fffffffu);
    const float ax = msl_f32(msl_bits(x) & 0x7fffffffu);
    const float sum = ppc_fadds(ay, ax);
    float t = ppc_fdivs(ppc_fsubs(ay, ax), sum);
    int32_t base = 0;

    /* These comparisons intentionally use the original signed lanes, not
       their absolute values.  The four paths are 80188DC4..80188DF0. */
    if (y >= zero) {
        /* fcmpu makes an unordered x take the non-"x >= 0" edge. */
        if (!(x >= zero)) {
            base = 0x00400000;
            t = -t;
        }
    } else if (x < zero) {
        base = 0x00800000;
    } else {
        base = (int32_t)0xffc00000u;
        t = -t;
    }
    const float t2 = ppc_fmuls(t, t);
    float power = ppc_fmuls(t, t2);             /* t^3 */
    float p = ppc_fmadds(t, coefficients[0],
        ppc_fmuls(power, coefficients[1]));
    power = ppc_fmuls(power, t2);               /* t^5 */
    p = ppc_fmadds(power, coefficients[2], p);
    power = ppc_fmuls(power, t2);
    p = ppc_fmadds(power, coefficients[3], p);
    power = ppc_fmuls(power, t2);
    p = ppc_fmadds(power, coefficients[4], p);
    power = ppc_fmuls(power, t2);
    p = ppc_fmadds(power, coefficients[5], p);
    power = ppc_fmuls(power, t2);
    p = ppc_fmadds(power, coefficients[6], p);
    power = ppc_fmuls(power, t2);
    p = ppc_fmadds(power, coefficients[7], p);
    p = ppc_fadds(p, coefficients[8]);
    const float scaled = ppc_fmuls(p, scale);
    /* fctiwz is truncation toward zero for every finite value reached by
       the quaternion conversion. */
    const int32_t converted = (int32_t)scaled;
    return signed24(converted + base);
}

/* 80188EF8 after 801A340C's 802165C8 square root. */
static float msl_inverse_or_one(float x)
{
    const float tiny = msl_f32(0x33d6bf95u);
    const float zero = msl_f32(0x00000000u);
    const float one = msl_f32(0x3f800000u);
    return x <= tiny ? zero : ppc_fdivs(one, x);
}

void nflstreet_801a340c_euler(const float q[4], int32_t result[3])
{
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float one = msl_f32(0x3f800000u);
    const float half = msl_f32(0x3f000000u);
    const float minus_two = msl_f32(0xc0000000u);
    const float tiny = msl_f32(0x33d6bf95u);
    const float yy = ppc_fmuls(y, y);
    const float zz = ppc_fmuls(z, z);
    const float xx = ppc_fmuls(x, x);
    const float pitch_sine = ppc_fmuls(
        ppc_fmadds(x, z, -ppc_fmuls(w, y)), minus_two);
    const float root = msl_sqrt(ppc_fsubs(one, ppc_fmuls(pitch_sine, pitch_sine)));

    if (!(msl_f32(msl_bits(root) & 0x7fffffffu) <= tiny)) {
        const float twice_inverse = ppc_fadds(msl_inverse_or_one(root),
            msl_inverse_or_one(root));
        const float first_y = ppc_fmuls(
            ppc_fmadds(y, z, ppc_fmuls(w, x)), twice_inverse);
        const float first_x = ppc_fmuls(ppc_fsubs(ppc_fsubs(half, xx), yy),
            twice_inverse);
        const float third_y = ppc_fmuls(
            ppc_fmadds(x, y, ppc_fmuls(w, z)), twice_inverse);
        const float third_x = ppc_fmuls(ppc_fsubs(ppc_fsubs(half, yy), zz),
            twice_inverse);
        result[0] = msl_atan2_angle(first_y, first_x);
        result[1] = msl_atan2_angle(pitch_sine, root);
        result[2] = msl_atan2_angle(third_y, third_x);
    } else {
        const float first_y = ppc_fmuls(
            ppc_fsubs(ppc_fmuls(y, z), ppc_fmuls(w, x)), minus_two);
        const float first_x = ppc_fsubs(one,
            ppc_fadds(ppc_fsubs(xx, zz), ppc_fsubs(xx, zz)));
        result[0] = msl_atan2_angle(first_y, first_x);
        result[1] = msl_atan2_angle(pitch_sine, root);
        result[2] = msl_atan2_angle(0.0f, one);
    }
}

void nflstreet_801ca900_local_matrix(const int32_t angles[3],
    const float coefficients[6], const float translation[3], float result[12])
{
    float x[2], y[2], z[2];
    nflstreet_80188bb0_pair((uint16_t)angles[0], coefficients, x);
    nflstreet_80188bb0_pair((uint16_t)angles[1], coefficients, y);
    nflstreet_80188bb0_pair((uint16_t)angles[2], coefficients, z);
    const float sx = x[0], cx = x[1], sy = y[0], cy = y[1];
    const float sz = z[0], cz = z[1];
    const float sxsy = ppc_fmuls(sx, sy);
    const float sz_sxsy = ppc_fmuls(sz, sxsy);
    const float cz_sxsy = ppc_fmuls(cz, sxsy);
    const float cxsy = ppc_fmuls(cx, sy);
    const float sz_cxsy = ppc_fmuls(sz, cxsy);
    const float cz_cxsy = ppc_fmuls(cz, cxsy);
    result[0] = ppc_fmuls(cz, cy);
    result[1] = ppc_fmadds(-sz, cx, cz_sxsy);
    result[2] = ppc_fmadds(sz, sx, cz_cxsy);
    result[3] = translation[0];
    result[4] = ppc_fmuls(sz, cy);
    result[5] = ppc_fmadds(cz, cx, sz_sxsy);
    result[6] = ppc_fmadds(-cz, sx, sz_cxsy);
    result[7] = translation[1];
    result[8] = -sy;
    result[9] = ppc_fmuls(cy, sx);
    result[10] = ppc_fmuls(cy, cx);
    result[11] = translation[2];
}

void nflstreet_801ffdf0_concat(const float parent[12], const float local[12],
    float result[12])
{
    /* 801FFDF0 computes the 3x4 affine product one PPC single FMA at a
       time.  Keep a separate output array: callers may alias neither input,
       but this also documents its actual non-aliasing call shape. */
    for (unsigned row = 0; row < 3; ++row) {
        const float a0 = parent[row * 4], a1 = parent[row * 4 + 1];
        const float a2 = parent[row * 4 + 2], a3 = parent[row * 4 + 3];
        for (unsigned column = 0; column < 3; ++column) {
            float v = ppc_fmuls(a0, local[column]);
            v = ppc_fmadds(a1, local[4 + column], v);
            v = ppc_fmadds(a2, local[8 + column], v);
            /* The low lane of f31 is zero (803A3958); the ps_madds1
               instruction also adds 0*a3 to column 2. */
            result[row * 4 + column] = column == 2 ? ppc_fmadds(0.0f, a3, v) : v;
        }
        float v = ppc_fmuls(a0, local[3]);
        v = ppc_fmadds(a1, local[7], v);
        v = ppc_fmadds(a2, local[11], v);
        result[row * 4 + 3] = ppc_fmadds(1.0f, a3, v);
    }
}

/* 80216EF4, __kernel_sinf(x,y,0), including each fmadds/fmsubs rounding. */
static float msl_kernel_sin(float x, float y)
{
    const float s1 = msl_f32(0xbe2aaaabu);
    const float s2 = msl_f32(0x3c088889u);
    const float s3 = msl_f32(0xb9500d01u);
    const float s4 = msl_f32(0x3638ef1bu);
    const float s5 = msl_f32(0xb2d72f34u);
    const float s6 = msl_f32(0x2f2ec9d3u);
    const float half = msl_f32(0x3f000000u);
    const float z = ppc_fmuls(x, x);
    float r = ppc_fmadds(z, s6, s5);
    r = ppc_fmadds(z, r, s4);
    r = ppc_fmadds(z, r, s3);
    r = ppc_fmadds(z, r, s2);
    const float v = ppc_fmuls(z, x);
    if (y == 0.0f)
        return ppc_fmadds(v, ppc_fmadds(z, r, s1), x);
    const float t = ppc_fmadds(y, half, -ppc_fmuls(v, r));
    const float u = ppc_fmadds(z, t, -y);
    return ppc_fsubs(x, ppc_fsubs(u, ppc_fmuls(v, s1)));
}

/* 80216E0C, __kernel_cosf(x,y).  qx is formed with the original bit edit,
   rather than with a numerically equivalent divide, because it participates
   in the last few single-rounding decisions. */
static float msl_kernel_cos(float x, float y)
{
    const float c1 = msl_f32(0xad47d74eu);
    const float c2 = msl_f32(0x310f74f6u);
    const float c3 = msl_f32(0xb493f27cu);
    const float c4 = msl_f32(0x37d00d01u);
    const float c5 = msl_f32(0xbab60b61u);
    const float c6 = msl_f32(0x3d2aaaabu);
    const float one = msl_f32(0x3f800000u);
    const float half = msl_f32(0x3f000000u);
    const uint32_t ax = msl_bits(x) & 0x7fffffffu;
    const float z = x * x;
    float r = ppc_fmadds(z, c1, c2);
    r = ppc_fmadds(z, r, c3);
    r = ppc_fmadds(z, r, c4);
    r = ppc_fmadds(z, r, c5);
    r = ppc_fmadds(z, r, c6);
    r = ppc_fmuls(z, r); /* f12 at 80216E8C */
    if (ax <= 0x3e999999u)
        return ppc_fsubs(one, ppc_fmadds(z, half,
            -ppc_fmadds(z, r, -ppc_fmuls(x, y))));
    const float qx = ax > 0x3f480000u ? msl_f32(0x3e900000u) :
        msl_f32(ax - 0x01000000u);
    const float hz = ppc_fmadds(z, half, -qx);
    return ppc_fsubs(ppc_fsubs(one, qx), ppc_fsubs(hz,
        ppc_fmadds(z, r, -ppc_fmuls(x, y))));
}

float nflstreet_80216b94_sine(float x)
{
    const uint32_t raw = msl_bits(x);
    const uint32_t ax = raw & 0x7fffffffu;
    const int negative = (int)(raw >> 31);
    /* 80216B94 reaches 802171C4 only beyond pi/4.  Slerp cannot furnish an
       argument beyond pi/2; reject neither endpoint nor signed zero. */
    if (ax <= 0x3f490fd8u)
        return msl_kernel_sin(x, 0.0f);
    if (ax <= 0x3fc90fdbu) {
        const float pio2_1 = msl_f32(0x3fc90f80u);
        const float pio2_1t = msl_f32(0x37354443u);
        const float pio2_2 = msl_f32(0x37354400u);
        const float pio2_2t = msl_f32(0x2e85a308u);
        const float a = msl_f32(ax);
        float z = a - pio2_1;
        const int second = (ax & 0x7ffffff0u) == 0x3fc90fd0u;
        const float tail = second ? pio2_2t : pio2_1t;
        if (second)
            z = z - pio2_2;
        const float y0 = z - tail;
        const float y1 = (z - y0) - tail;
        const float result = msl_kernel_cos(y0, y1);
        return negative ? -result : result;
    }
    /* 801A383C never reaches the MSL large-argument reducer.  Keep this
       explicit instead of silently replacing it with a host transcendental. */
    return msl_quiet_nan();
}

void nflstreet_801a36ec_quaternion(int32_t x, int32_t y, int32_t z,
    const float coefficients[8], float result[4])
{
    const float sx = nflstreet_8018889c_sine(x >> 1, 0, coefficients);
    const float cx = nflstreet_8018889c_sine(x >> 1, 1, coefficients);
    const float sy = nflstreet_8018889c_sine(y >> 1, 0, coefficients);
    const float cy = nflstreet_8018889c_sine(y >> 1, 1, coefficients);
    const float sz = nflstreet_8018889c_sine(z >> 1, 0, coefficients);
    const float cz = nflstreet_8018889c_sine(z >> 1, 1, coefficients);
    const float sxsy = sx * sy;
    const float cxsy = cx * sy;
    const float sxcy = sx * cy;
    const float cxcy = cx * cy;
    const float sz_sxsy = sz * sxsy;
    const float sz_sxcy = sz * sxcy;
    const float sz_cxsy = sz * cxsy;
    const float cz_sxsy = cz * sxsy;
    result[3] = ppc_fmadds(cz, cxcy, sz_sxsy);
    result[2] = ppc_fmadds(cz, sxcy, -sz_cxsy);
    result[0] = ppc_fmadds(sz, cxcy, -cz_sxsy);
    result[1] = ppc_fmadds(cz, cxsy, sz_sxcy);
}

void nflstreet_801a383c_slerp(float t, const float a[4], const float b[4],
    float result[4])
{
    /* 801A2F74 then 801A2CE0/80188EAC. */
    const float dot = ppc_fmadds(a[3], b[3], ppc_fmadds(a[2], b[2],
        ppc_fmadds(a[0], b[0], ppc_fmuls(a[1], b[1]))));
    const float alen = ppc_fmadds(a[3], a[3], ppc_fmadds(a[2], a[2],
        ppc_fmadds(a[0], a[0], ppc_fmuls(a[1], a[1]))));
    const float blen = ppc_fmadds(b[3], b[3], ppc_fmadds(b[2], b[2],
        ppc_fmadds(b[0], b[0], ppc_fmuls(b[1], b[1]))));
    float cosine = ppc_fmuls(dot, ppc_fmuls(ppc_fdivs(1.0f, msl_sqrt(alen)),
        ppc_fdivs(1.0f, msl_sqrt(blen))));
    float signed_a[4];
    if (!(cosine >= 0.0f)) {
        cosine = -cosine;
        for (unsigned i = 0; i < 4; ++i) signed_a[i] = -a[i];
    } else {
        for (unsigned i = 0; i < 4; ++i) signed_a[i] = a[i];
    }
    float wa, wb;
    if (cosine > 0.9900000095367431640625f) {
        wa = t;
        wb = 1.0f - t;
    } else {
        const float theta = msl_acos(cosine);
        const float inverse = ppc_fdivs(1.0f, nflstreet_80216b94_sine(theta));
        wb = ppc_fmuls(nflstreet_80216b94_sine(ppc_fmuls(theta, 1.0f - t)), inverse);
        const float arga = ppc_fmuls(theta, t);
        const float sina = nflstreet_80216b94_sine(arga);
        wa = ppc_fmuls(sina, inverse);
    }
    for (unsigned i = 0; i < 4; ++i)
        /* fmuls f0,wa,a; fmadds out,wb,b,f0 */
        result[i] = ppc_fmadds(wb, b[i], ppc_fmuls(wa, signed_a[i]));
}
