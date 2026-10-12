#include "odd_polynomial_pair.h"

#include <assert.h>

int main(void)
{
    const float coefficients[6] = {1, 2, 3, 4, 5, 6};
    const float t[2] = {0.5f, -1.0f};
    float output[2];
    nflstreet_odd_polynomial_pair(t, coefficients, output);
    assert(output[0] == 0.8876953125f);
    assert(output[1] == -21.0f);
    return 0;
}
