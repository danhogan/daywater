/* GNNE69 0x80036B28 finite-value C reconstruction, replayed from live RAM. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "kinematics.h"

static float as_float(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static uint32_t as_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static float addf(float a, float b) { volatile float result = a + b; return result; }
static float subf(float a, float b) { volatile float result = a - b; return result; }
static float mulf(float a, float b) { volatile float result = a * b; return result; }
static float divf(float a, float b) { volatile float result = a / b; return result; }

/* Initialized by the original 0x801888F0; checked in the supplied MEM1 state. */
static const uint32_t sine_coefficients[8] = {
    0x40490fdb, 0xc0a55de8, 0x402335e5, 0xbf196969,
    0x3da83c1d, 0xbbf183ad, 0x39f47a22, 0xb7b7d6e3
};

/* 0x8018889C: paired-single odd polynomial and ps_sum0, for finite inputs. */
static float sine_polynomial(int32_t angle)
{
    float t = mulf((float)angle, as_float(0x34000000));
    float t2 = mulf(t, t);
    float t3 = mulf(t, t2);
    float t4 = mulf(t2, t2);
    float t5 = mulf(t, t4);
    float t7 = mulf(t3, t4);
    float t9 = mulf(t5, t4);
    float t11 = mulf(t7, t4);
    float t13 = mulf(t9, t4);
    float t15 = mulf(t11, t4);
    float even = mulf(as_float(sine_coefficients[0]), t);
    float odd = mulf(as_float(sine_coefficients[1]), t3);
    even = fmaf(as_float(sine_coefficients[2]), t5, even);
    odd = fmaf(as_float(sine_coefficients[3]), t7, odd);
    even = fmaf(as_float(sine_coefficients[4]), t9, even);
    odd = fmaf(as_float(sine_coefficients[5]), t11, odd);
    even = fmaf(as_float(sine_coefficients[6]), t13, even);
    odd = fmaf(as_float(sine_coefficients[7]), t15, odd);
    return addf(even, odd);
}

static int32_t signed_angle(uint32_t angle)
{
    uint32_t low = angle & 0xffffff;
    return (int32_t)(low & 0x800000 ? low | 0xff000000 : low);
}

float nflstreet_sine_angle(uint32_t angle)
{
    return sine_polynomial(signed_angle(angle));
}

/* 0x801DE460 via 0x80188B6C: produce an XY vector from speed and angle. */
static void direction_vector(float speed, uint32_t angle, float out[2])
{
    float sine = nflstreet_sine_angle(angle);
    float cosine = nflstreet_sine_angle(angle + 0x400000);
    out[0] = mulf(cosine, speed);
    out[1] = mulf(sine, speed);
}

/* 0x80188D98: game angle from Y and X velocity, including polynomial order. */
uint32_t nflstreet_angle_from_vector(float y, float x)
{
    static const uint32_t coefficients[8] = {
        0x3f7ffff5, 0xbeaaa61c, 0x3e4c40a6, 0xbe0e6c63,
        0x3dc577df, 0xbd6501c4, 0x3cb31652, 0xbb84d7e7
    };
    float absolute_y = fabsf(y), absolute_x = fabsf(x);
    float ratio = divf(subf(absolute_y, absolute_x), addf(absolute_y, absolute_x));
    int32_t quadrant = 0;
    if (y >= 0.0f) {
        if (x < 0.0f) { quadrant = 0x400000; ratio = -ratio; }
    } else if (x < 0.0f) {
        quadrant = 0x800000;
    } else {
        quadrant = -0x400000;
        ratio = -ratio;
    }
    float square = mulf(ratio, ratio);
    float power = mulf(ratio, square);
    float term = mulf(power, as_float(coefficients[1]));
    float value = fmaf(ratio, as_float(coefficients[0]), term);
    unsigned i;
    for (i = 2; i < 8; ++i) {
        power = mulf(power, square);
        value = fmaf(power, as_float(coefficients[i]), value);
    }
    value = addf(value, as_float(0x3f490fdb));
    value = mulf(value, as_float(0x4a22f983));
    return (uint32_t)((int32_t)value + quadrant) & 0xffffff;
}

static float clear_small(float value)
{
    return fabsf(value) < as_float(0x33d6bf95) ? 0.0f : value;
}

static float getf(const uint32_t state[21], unsigned offset)
{
    return as_float(state[offset / 4]);
}

static void putf(uint32_t state[21], unsigned offset, float value)
{
    state[offset / 4] = as_bits(value);
}

/* Grounded acceleration and zero-acceleration braking branches. */
int nflstreet_step_kinematics(uint32_t state[21])
{
    float velocity[3], acceleration[2];
    float maximum = getf(state, 0x24);
    float speed = getf(state, 0x1c);
    float acceleration_magnitude = getf(state, 0x34);
    uint32_t angle = state[0x20 / 4];
    float length;
    unsigned i;
    if (getf(state, 0x08) != 0.0f || !isfinite(maximum) || maximum < 0.0f ||
        !isfinite(speed) || speed < 0.0f ||
        !isfinite(acceleration_magnitude) || acceleration_magnitude < 0.0f ||
        !isfinite(getf(state, 0x00)) || !isfinite(getf(state, 0x04)) ||
        !isfinite(getf(state, 0x28)) || !isfinite(getf(state, 0x2c)) ||
        !isfinite(getf(state, 0x3c)) || getf(state, 0x3c) < 0.0f ||
        !isfinite(getf(state, 0x30)))
        return 0;

    for (i = 0; i < 3; ++i) state[0x0c / 4 + i] = state[i];
    state[0x40 / 4] = state[0x24 / 4];
    state[0x44 / 4] = state[0x1c / 4];
    if (subf(speed, getf(state, 0x48)) < as_float(0x33d6bf95)) speed = 0.0f;
    if (acceleration_magnitude == 0.0f || maximum == 0.0f) {
        /* Original 0x80036B28 zero-acceleration/zero-maximum branch.
           Clamp the decrement to current speed, then clamp speed to maximum.
           Heading is preserved; no direction recomputation at zero speed. */
        float decrement = getf(state, 0x3c);
        if (speed < decrement) {
            decrement = speed;
            putf(state, 0x3c, decrement);
        }
        speed = subf(speed, decrement);
        if (speed > maximum) speed = maximum;
        direction_vector(speed, angle, velocity);
    } else {
        float fraction = divf(speed, maximum);
        float drag = mulf(mulf(fraction, fraction), acceleration_magnitude);
        direction_vector(acceleration_magnitude, state[0x38 / 4], acceleration);
        speed = subf(speed, drag);
        if (speed < 0.0f) speed = 0.0f;
        direction_vector(speed, angle, velocity);
        velocity[0] = addf(acceleration[0], velocity[0]);
        velocity[1] = addf(acceleration[1], velocity[1]);
        /* 0x801DDFE0/0x801DDFE4: y*y, then fused x*x + y*y. */
        length = sqrtf(fmaf(velocity[0], velocity[0],
                            mulf(velocity[1], velocity[1])));
        if (!isfinite(length) || length < as_float(0x33d6bf95))
            return 0;
        if (length > maximum) {
            /* 0x80036C64..0x80036CD0: cap the 2D vector to maximum. */
            float scale = divf(maximum, length);
            velocity[0] = clear_small(mulf(velocity[0], scale));
            velocity[1] = clear_small(mulf(velocity[1], scale));
            speed = maximum;
        } else {
            speed = length;
        }
        angle = nflstreet_angle_from_vector(velocity[1], velocity[0]);
    }
    velocity[2] = getf(state, 0x30);
    if (getf(state, 0x08) > 0.0f)
        velocity[2] = subf(velocity[2], as_float(0x3b4364f6));
    for (i = 0; i < 3; ++i) {
        velocity[i] = clear_small(velocity[i]);
        putf(state, 0x28 + i * 4, velocity[i]);
        putf(state, i * 4, addf(getf(state, i * 4), velocity[i]));
    }
    putf(state, 0x1c, speed);
    state[0x50 / 4] = 0;
    state[0x4c / 4] = angle & 0xffffff;
    state[0x20 / 4] = angle & 0xffffff;
    return 1;
}

#ifndef NFLSTREET_KINEMATICS_NO_MAIN
static int hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

int main(int argc, char **argv)
{
    uint32_t state[21];
    unsigned i;
    if (argc != 2 || strlen(argv[1]) != 0xa8) {
        fputs("usage: kinematics_replay 168_HEX_DIGITS\n", stderr);
        return 2;
    }
    for (i = 0; i < 21; ++i) {
        unsigned j;
        uint32_t word = 0;
        for (j = 0; j < 8; ++j) {
            int digit = hex_digit(argv[1][8 * i + j]);
            if (digit < 0) return 2;
            word = (word << 4) | (uint32_t)digit;
        }
        state[i] = word;
    }
    if (!nflstreet_step_kinematics(state)) {
        fputs("input takes an unverified kinematics branch\n", stderr);
        return 3;
    }
    for (i = 0; i < 21; ++i) printf("%08x", state[i]);
    putchar('\n');
    return 0;
}
#endif
