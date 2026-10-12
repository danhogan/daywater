#include "paired_product.h"

#include <assert.h>

int main(void)
{
    const float input[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    const float factors[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    const float constant[2] = {1, 1};
    const float expected[12] = {6, 6, 10, 10, 18, 18,
                                26, 26, 30, 30, 42, 42};
    float output[12];
    nflstreet_paired_product(input, factors, constant, output);
    for (unsigned i = 0; i < 12; ++i)
        assert(output[i] == expected[i]);
    return 0;
}
