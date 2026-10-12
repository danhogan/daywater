#ifndef NFLSTREET_RESET_ACCELERATION_H
#define NFLSTREET_RESET_ACCELERATION_H

#include <stdint.h>

int nflstreet_reset_acceleration(uint32_t header, int16_t rating_bbc,
                                 uint32_t actor_408, const uint32_t actor[12],
                                 uint32_t state[21]);

#endif
