/* GNNE69 0x800F705C: observed acceleration-preparation branch. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../kinematics/kinematics.h"
#include "preparation.h"

#ifndef NFLSTREET_PREPARATION_NO_MAIN
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

#endif

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

static float addf(float a, float b) { volatile float v = a + b; return v; }
static float subf(float a, float b) { volatile float v = a - b; return v; }
static float mulf(float a, float b) { volatile float v = a * b; return v; }
static float divf(float a, float b) { volatile float v = a / b; return v; }

static uint32_t angle_distance(uint32_t a, uint32_t b)
{
    uint32_t distance = (a - b) & 0xffffff;
    uint32_t opposite = 0x1000000 - distance;
    return distance < opposite ? distance : opposite;
}

int nflstreet_prepare_acceleration(uint32_t header, const uint32_t actor_inputs[12],
                                   int16_t rating, uint32_t state[21])
{
    uint32_t difference = angle_distance(state[0x20 / 4], actor_inputs[1]);
    float attenuation = 0.0f;
    float prior = as_float(state[0x34 / 4]);
    if ((header & 0xff) != 1 || (actor_inputs[8] & 0x00ff0000) != 0 ||
        (state[0x20 / 4] | actor_inputs[1]) & 0xff000000 ||
        !isfinite(prior) || prior <= 0.0f)
        return 0;
    if (difference > 0) {
        float sine = difference < 0x400000 ?
            nflstreet_sine_angle(difference) : as_float(0x3f800000);
        float maximum = as_float(state[0x24 / 4]);
        float speed = as_float(state[0x1c / 4]);
        float ratio;
        float blend;
        if (!isfinite(maximum) || maximum <= 0.0f ||
            !isfinite(speed) || speed < 0.0f) return 0;
        ratio = divf(speed, maximum);
        if (ratio > 1.0f) ratio = 1.0f;
        blend = mulf(as_float(0x3ecccccd), as_float(0x3b808081));
        blend = fmaf(blend, (float)rating, as_float(0x3f19999a));
        attenuation = mulf(subf(as_float(0x3f19999a), as_float(0x3f000000)),
                           blend);
        attenuation = addf(attenuation, as_float(0x3f000000));
        attenuation = mulf(mulf(sine, ratio), attenuation);
        if (attenuation > 1.0f) attenuation = 1.0f;
    }
    state[0x34 / 4] = as_bits(mulf(prior,
        subf(as_float(0x3faccccd), attenuation)));
    return 1;
}

#ifndef NFLSTREET_PREPARATION_NO_MAIN
int main(int argc, char **argv)
{
    uint32_t header, actor[12], state[21], rating_word;
    unsigned i;
    if (argc != 5 || !parse_words(argv[1], &header, 1) ||
        !parse_words(argv[2], &rating_word, 1) || rating_word > 0xffff ||
        !parse_words(argv[3], actor, 12) ||
        !parse_words(argv[4], state, 21)) {
        fputs("usage: preparation_replay 8_HEADER_HEX 8_RATING_HEX "
              "96_ACTOR_HEX 168_STATE_HEX\n", stderr);
        return 2;
    }
    if (!nflstreet_prepare_acceleration(header, actor, (int16_t)rating_word, state)) {
        fputs("input takes an unverified preparation branch\n", stderr);
        return 3;
    }
    for (i = 0; i < 21; ++i) printf("%08x", state[i]);
    putchar('\n');
    return 0;
}
#endif
