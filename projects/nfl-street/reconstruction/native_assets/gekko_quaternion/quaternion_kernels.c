#include "quaternion_kernels.h"

#include <math.h>
#include <string.h>

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
    return (pair){fmaf(a.lo, b.lo, c.lo), fmaf(a.hi, b.hi, c.hi)};
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

/* 802165C8.  This is the MSL restoring square-root loop, including its
   final tie-to-even increment; it is deliberately integer-only. */
static float msl_sqrt(float x)
{
    uint32_t ix = msl_bits(x);
    if ((ix & 0x7fffffffu) == 0) return x;
    if (ix & 0x80000000u) return 0.0f / 0.0f;
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
        float p = fmaf(z, p1, p2);
        p = fmaf(z, p, p3); p = fmaf(z, p, p4);
        p = fmaf(z, p, p5); p = fmaf(z, p, p6);
        float q = fmaf(z, q1, q2);
        q = fmaf(z, q, q3); q = fmaf(z, q, q4);
        const float r = (z * p) / fmaf(z, q, one);
        return pio2 - (x - (tiny - x * r));
    }
    const float z = (one - x) * half;
    const float s = msl_sqrt(z);
    const float df = msl_f32(msl_bits(s) & 0xfffff000u);
    const float c = (z - df * df) / (s + df);
    float p = fmaf(z, p1, p2);
    p = fmaf(z, p, p3); p = fmaf(z, p, p4);
    p = fmaf(z, p, p5); p = fmaf(z, p, p6);
    float q = fmaf(z, q1, q2);
    q = fmaf(z, q, q3); q = fmaf(z, q, q4);
    const float r = (z * p) / fmaf(z, q, one);
    const float w = fmaf(r, s, c);
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

/* 80216EF4, __kernel_sinf(x,y,0), including each fmadds/fmsubs rounding. */
static float msl_kernel_sin(float x, float y)
{
    const float s1 = msl_f32(0x2f2ec9d3u);
    const float s2 = msl_f32(0xb2d72f34u);
    const float s3 = msl_f32(0x3638ef1bu);
    const float s4 = msl_f32(0xb9500d01u);
    const float s5 = msl_f32(0x3c088889u);
    const float s6 = msl_f32(0xbe2aaaabu);
    const float half = msl_f32(0x3f000000u);
    const float z = x * x;
    float r = fmaf(z, s1, s2);
    r = fmaf(z, r, s3);
    r = fmaf(z, r, s4);
    r = fmaf(z, r, s5);
    const float v = z * x;
    r = v * r;
    const float t = fmaf(y, half, -r);
    const float u = fmaf(z, t, -y);
    return x - (u - v * s6);
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
    float r = fmaf(z, c1, c2);
    r = fmaf(z, r, c3);
    r = fmaf(z, r, c4);
    r = fmaf(z, r, c5);
    r = fmaf(z, r, c6);
    r = z * r; /* f12 at 80216E8C */
    if (ax <= 0x3e999999u)
        return one - fmaf(z, half, -fmaf(z, r, -x * y));
    const float qx = ax > 0x3f480000u ? msl_f32(0x3e900000u) :
        msl_f32(ax - 0x01000000u);
    const float hz = fmaf(z, half, -qx);
    return (one - qx) - (hz - fmaf(z, r, -x * y));
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
    return 0.0f / 0.0f;
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
    result[3] = fmaf(cz, cxcy, sz_sxsy);
    result[2] = fmaf(cz, sxcy, -sz_cxsy);
    result[0] = fmaf(sz, cxcy, -cz_sxsy);
    result[1] = fmaf(cz, cxsy, sz_sxcy);
}

void nflstreet_801a383c_slerp(float t, const float a[4], const float b[4],
    float result[4])
{
    /* 801A2F74 then 801A2CE0/80188EAC. */
    const float dot = fmaf(a[3], b[3], fmaf(a[2], b[2],
        fmaf(a[0], b[0], a[1] * b[1])));
    const float alen = fmaf(a[3], a[3], fmaf(a[2], a[2],
        fmaf(a[0], a[0], a[1] * a[1])));
    const float blen = fmaf(b[3], b[3], fmaf(b[2], b[2],
        fmaf(b[0], b[0], b[1] * b[1])));
    float cosine = dot * ((1.0f / msl_sqrt(alen)) * (1.0f / msl_sqrt(blen)));
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
        const float inverse = 1.0f / nflstreet_80216b94_sine(theta);
        wb = nflstreet_80216b94_sine(theta * (1.0f - t)) * inverse;
        wa = nflstreet_80216b94_sine(theta * t) * inverse;
    }
    for (unsigned i = 0; i < 4; ++i)
        /* fmuls f0,wa,a; fmadds out,wb,b,f0 */
        result[i] = fmaf(wb, b[i], wa * signed_a[i]);
}
