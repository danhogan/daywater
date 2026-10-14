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
