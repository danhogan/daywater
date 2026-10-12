#include "paired_product.h"

#include <math.h>

void nflstreet_paired_product(const float input[12], const float factors[12],
                              const float constant[2], float output[12])
{
    for (unsigned row = 0; row < 3; ++row) {
        const unsigned i = 4 * row;
        for (unsigned lane = 0; lane < 2; ++lane) {
            float first = factors[lane] * input[i];
            first = fmaf(factors[4 + lane], input[i + 1], first);
            first = fmaf(factors[8 + lane], input[i + 2], first);
            output[i + lane] = first;

            float second = factors[2 + lane] * input[i];
            second = fmaf(factors[6 + lane], input[i + 1], second);
            second = fmaf(factors[10 + lane], input[i + 2], second);
            second = fmaf(constant[lane], input[i + 3], second);
            output[i + 2 + lane] = second;
        }
    }
}
