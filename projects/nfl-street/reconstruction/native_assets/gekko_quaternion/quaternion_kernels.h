#ifndef NFLSTREET_GEKKO_QUATERNION_KERNELS_H
#define NFLSTREET_GEKKO_QUATERNION_KERNELS_H

#include <stdint.h>

/* The numerical lanes of 80188BB0 after its GQR loads have supplied a signed
   16-bit angle pair and six single-precision coefficients.  This keeps the
   instruction order: paired multiplies round each lane, ps_madd is fused,
   and ps_sum0 is a final single add. */
void nflstreet_80188bb0_pair(uint16_t angle, const float coefficients[6],
    float result[2]);

/* 80188A70/80188AEC's eight-coefficient single-lane polynomial after the
   24-bit signed angle has been reduced to [-1, 1). */
float nflstreet_8018889c_sine(int32_t angle24, int cosine,
    const float coefficients[8]);

/* MSL's 80216B94 sine wrapper for finite arguments in [-pi/2, pi/2].  This
   is the complete range exercised by 801A383C: the reduction and both
   polynomial helpers retain the DOL's single-precision rounding points. */
float nflstreet_80216b94_sine(float x);
float nflstreet_80215844_acos(float x);
float nflstreet_802165c8_sqrt(float x);

/* 801A36EC.  Angles are the original signed 24-bit fixed-point values, in
   X/Y/Z argument order; the result is X/Y/Z/W. */
void nflstreet_801a36ec_quaternion(int32_t x, int32_t y, int32_t z,
    const float coefficients[8], float result[4]);

/* 801A340C.  Converts X/Y/Z/W quaternion lanes to the three signed 24-bit
   fixed-point Euler angles consumed by 801A36EC. */
void nflstreet_801a340c_euler(const float quaternion[4], int32_t result[3]);

/* Candidate transcription of 801A383C.  Its transcendental calls are host
   placeholders until the 80215844/80216B94 kernels are reconstructed. */
void nflstreet_801a383c_slerp(float t, const float a[4], const float b[4],
    float result[4]);

#endif
