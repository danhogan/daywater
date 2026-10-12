#include "three_pair_projection.h"

#include <math.h>

void nflstreet_three_pair_projection(const float source[12],
                                     const float coefficients[3],
                                     float output[3])
{
    for (unsigned row = 0; row < 3; ++row) {
        const unsigned offset = 4u * row;
        const float first = source[offset] * coefficients[0];
        const float second = source[offset + 1u] * coefficients[1];
        const float sum = first + second;
        output[row] = fmaf(source[offset + 2u], coefficients[2], sum);
    }
}
