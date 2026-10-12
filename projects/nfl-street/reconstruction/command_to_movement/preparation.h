#ifndef NFLSTREET_PREPARATION_H
#define NFLSTREET_PREPARATION_H

#include <stdint.h>

int nflstreet_prepare_acceleration(uint32_t header, const uint32_t actor_inputs[12],
                                   int16_t rating, uint32_t state[21]);

#endif
