#ifndef NFLSTREET_GEKKO_QUATERNION_KERNELS_H
#define NFLSTREET_GEKKO_QUATERNION_KERNELS_H

#include <stdint.h>

/* The numerical lanes of 80188BB0 after its GQR loads have supplied a signed
   16-bit angle pair and six single-precision coefficients.  This keeps the
   instruction order: paired multiplies round each lane, ps_madd is fused,
   and ps_sum0 is a final single add. */
void nflstreet_80188bb0_pair(uint16_t angle, const float coefficients[6],
    float result[2]);

#endif
