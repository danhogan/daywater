/* Bounded GNNE69 0x8008D864 target-position to actor-input reconstruction. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "actor_target_input.h"
#include "../kinematics/kinematics.h"

static uint32_t be32(const uint8_t *data)
{
    return (uint32_t)data[0] << 24 | (uint32_t)data[1] << 16 |
           (uint32_t)data[2] << 8 | data[3];
}

static float be_float(const uint8_t *data)
{
    uint32_t bits = be32(data);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static void put_be32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static float subf(float a, float b) { volatile float result = a - b; return result; }
static float mulf(float a, float b) { volatile float result = a * b; return result; }

int nflstreet_project_actor_target(const uint8_t target[12],
                                   const uint8_t position[12],
                                   uint8_t actor[0x30])
{
    float x = subf(be_float(target), be_float(position));
    float y = subf(be_float(target + 4), be_float(position + 4));
    float distance, scaled, strength;
    uint32_t bits, heading;
    if (!isfinite(x) || !isfinite(y)) return 0;
    /* 0x801DDFE0/E4: y*y, then fused x*x + y*y. */
    distance = sqrtf(fmaf(x, x, mulf(y, y)));
    if (!isfinite(distance) || distance < 1.0f) return 0;
    heading = nflstreet_angle_from_vector(y, x);
    scaled = mulf(subf(distance, 1.0f), 0.25f);
    if (scaled > 1.0f) return 0;
    /* Constants at DOL 0x80250A7C and 0x80250A80. */
    strength = fmaf(scaled, 0.8600000143051147f, 0.4000000059604645f);
    if (!isfinite(strength)) return 0;
    memcpy(&bits, &strength, sizeof bits);
    put_be32(actor, bits);
    put_be32(actor + 4, heading);
    put_be32(actor + 8, heading);
    actor[0x0e] = 1;
    if (strength > 0.7200000286102295f) actor[0x0f] = 1;
    return 1;
}

#ifndef NFLSTREET_ACTOR_TARGET_INPUT_NO_MAIN
static int hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static int read_hex(const char *text, uint8_t *out, size_t length)
{
    size_t i;
    if (strlen(text) != length * 2) return 0;
    for (i = 0; i < length; ++i) {
        int high = hex_digit(text[2 * i]), low = hex_digit(text[2 * i + 1]);
        if (high < 0 || low < 0) return 0;
        out[i] = (uint8_t)((high << 4) | low);
    }
    return 1;
}

int main(int argc, char **argv)
{
    uint8_t target[12], position[12], actor[0x30];
    unsigned i;
    if (argc != 4 || !read_hex(argv[1], target, sizeof target) ||
        !read_hex(argv[2], position, sizeof position) ||
        !read_hex(argv[3], actor, sizeof actor)) {
        fputs("usage: actor_target_input 24_TARGET_HEX 24_POSITION_HEX 96_ACTOR_HEX\n",
              stderr);
        return 2;
    }
    if (!nflstreet_project_actor_target(target, position, actor)) {
        fputs("input takes an unverified target-input branch\n", stderr);
        return 3;
    }
    for (i = 0; i < sizeof actor; ++i) printf("%02x", actor[i]);
    putchar('\n');
    return 0;
}
#endif
