#include "weighted_pair_reduction.h"

#include <math.h>

void nflstreet_weighted_pair_reduction(const float source[12],
                                       const float coefficients[3],
                                       float output[3])
{
    for (unsigned row = 0; row < 3; ++row) {
        const unsigned offset = 4u * row;
        const float first = source[offset] * coefficients[0];
        const float second = source[offset + 1u] * coefficients[1];
        const float low = fmaf(source[offset + 2u], coefficients[2], first);
        const float high = fmaf(source[offset + 3u], 1.0f, second);
        output[row] = low + high;
    }
}
