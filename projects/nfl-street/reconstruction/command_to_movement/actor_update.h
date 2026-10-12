#ifndef NFLSTREET_ACTOR_UPDATE_H
#define NFLSTREET_ACTOR_UPDATE_H

#include <stdint.h>

/* stage: 0 = full movement, 1 = preparation, 2 = command. */
int nflstreet_actor_update(uint32_t header, int16_t rating,
                           const uint32_t actor[12], uint32_t state[21],
                           int stage);

#endif
