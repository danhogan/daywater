#ifndef NFLSTREET_ACTOR_TARGET_INPUT_H
#define NFLSTREET_ACTOR_TARGET_INPUT_H

#include <stdint.h>

/* Guarded GNNE69 0x8008D864 path for measured midrange XY distances. */
int nflstreet_project_actor_target(const uint8_t target[12],
                                   const uint8_t position[12],
                                   uint8_t actor[0x30]);

#endif
