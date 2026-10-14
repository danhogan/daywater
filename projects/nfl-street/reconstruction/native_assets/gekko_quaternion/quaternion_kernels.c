#include "quaternion_kernels.h"

#include <math.h>

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
    float cosine = dot * ((1.0f / sqrtf(alen)) * (1.0f / sqrtf(blen)));
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
        const float theta = acosf(cosine);
        const float inverse = 1.0f / sinf(theta);
        wb = sinf(theta * (1.0f - t)) * inverse;
        wa = sinf(theta * t) * inverse;
    }
    for (unsigned i = 0; i < 4; ++i)
        /* fmuls f0,wa,a; fmadds out,wb,b,f0 */
        result[i] = fmaf(wb, b[i], wa * signed_a[i]);
}
