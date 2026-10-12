#include "odd_polynomial_pair.h"

#include <math.h>

static float evaluate_lane(float t, const float c[6])
{
    /* Follow the power and fused-add order seen in the paired-single code. */
    const float t2 = t * t;
    const float t3 = t * t2;
    const float t4 = t2 * t2;
    const float t5 = t * t4;
    const float t7 = t3 * t4;
    const float t9 = t5 * t4;
    const float t11 = t7 * t4;
    float low = c[0] * t;
    float high = c[1] * t3;
    low = fmaf(c[2], t5, low);
    high = fmaf(c[3], t7, high);
    low = fmaf(c[4], t9, low);
    high = fmaf(c[5], t11, high);
    return low + high;
}

void nflstreet_odd_polynomial_pair(const float t[2], const float coefficients[6],
                                   float output[2])
{
    output[0] = evaluate_lane(t[0], coefficients);
    output[1] = evaluate_lane(t[1], coefficients);
}
