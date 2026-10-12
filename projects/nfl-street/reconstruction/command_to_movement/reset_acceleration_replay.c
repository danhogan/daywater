/* GNNE69 0x800F7C90: bounded normal-movement speed/acceleration setup. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "reset_acceleration.h"

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

static float subf(float a, float b) { volatile float v = a - b; return v; }
static float mulf(float a, float b) { volatile float v = a * b; return v; }

int nflstreet_reset_acceleration(uint32_t header, int16_t rating_bbc,
                                 uint32_t actor_408, const uint32_t actor[12],
                                 uint32_t state[21])
{
    float strength = as_float(actor[0x10 / 4]); /* actor +0x210 */
    float factor, base_accel, maximum, cap;
    if ((header & 0xff) != 1 || actor_408 != 0 ||
        ((actor[0x1c / 4] >> 8) & 0xff) != 1 ||
        actor[0x14 / 4] != actor[0x18 / 4] ||
        actor[0x14 / 4] != actor[0x04 / 4] ||
        !isfinite(strength) || strength <= 0.0f || strength > 1.0f ||
        state[0x34 / 4] != 0 || state[0x3c / 4] != 0x3b6bedfa ||
        state[0x48 / 4] != 0)
        return 0;

    /* DOL 0x800F7D04..0x800F7D34 and 0x800F7E70..0x800F7ED4. */
    factor = fmaf((float)rating_bbc, as_float(0x3b008081),
                  as_float(0x3f000000));
    maximum = mulf(as_float(0x3e430c31), strength);
    maximum = mulf(maximum, factor);
    cap = mulf(as_float(0x3e430c31), as_float(0x40a00000));
    if (maximum > cap) maximum = cap;

    /* DOL 0x800F7F8C..0x800F8008; f29 is +1 on this guarded path. */
    base_accel = fmaf(subf(as_float(0x3c4ccccd), as_float(0x3c1374bc)),
                      strength, as_float(0x3c1374bc));
    base_accel = mulf(base_accel, factor);
    state[0x24 / 4] = as_bits(maximum);
    state[0x34 / 4] = as_bits(base_accel);
    return 1;
}

#ifndef NFLSTREET_RESET_NO_MAIN
static int digit(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static int parse_words(const char *source, uint32_t *out, unsigned count)
{
    unsigned i, j;
    if (strlen(source) != count * 8) return 0;
    for (i = 0; i < count; ++i) {
        uint32_t word = 0;
        for (j = 0; j < 8; ++j) {
            int n = digit(source[i * 8 + j]);
            if (n < 0) return 0;
            word = (word << 4) | (uint32_t)n;
        }
        out[i] = word;
    }
    return 1;
}

int main(int argc, char **argv)
{
    uint32_t header, rating, actor_408, actor[12], state[21];
    unsigned i;
    if (argc != 6 || !parse_words(argv[1], &header, 1) ||
        !parse_words(argv[2], &rating, 1) || rating > 0xffff ||
        !parse_words(argv[3], &actor_408, 1) ||
        !parse_words(argv[4], actor, 12) ||
        !parse_words(argv[5], state, 21)) {
        fputs("usage: reset_acceleration_replay 8_HEADER_HEX 8_RATING_HEX "
              "8_ACTOR408_HEX 96_ACTOR_HEX 168_STATE_HEX\n", stderr);
        return 2;
    }
    if (!nflstreet_reset_acceleration(header, (int16_t)rating,
                                       actor_408, actor, state)) {
        fputs("input takes an unverified movement-setup branch\n", stderr);
        return 3;
    }
    for (i = 0; i < 21; ++i) printf("%08x", state[i]);
    putchar('\n');
    return 0;
}
#endif
