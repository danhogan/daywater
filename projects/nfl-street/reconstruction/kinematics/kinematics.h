#ifndef NFLSTREET_KINEMATICS_H
#define NFLSTREET_KINEMATICS_H

#include <stdint.h>

uint32_t nflstreet_angle_from_vector(float y, float x);
float nflstreet_sine_angle(uint32_t angle);
int nflstreet_step_kinematics(uint32_t state[21]);

#endif
