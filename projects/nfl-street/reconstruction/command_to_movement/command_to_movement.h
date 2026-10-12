#ifndef NFLSTREET_COMMAND_TO_MOVEMENT_H
#define NFLSTREET_COMMAND_TO_MOVEMENT_H

#include <stdint.h>

int nflstreet_result_heading(const uint32_t actor[12], const uint32_t state[21],
                             uint32_t *result);
void nflstreet_store_command(uint32_t state[21], uint32_t heading,
                             uint32_t result, uint32_t acceleration_bits);

#endif
