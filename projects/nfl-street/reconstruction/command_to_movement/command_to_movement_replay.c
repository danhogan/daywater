/* GNNE69 0x80036B18 command store followed by guarded 0x80036B28 movement. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../kinematics/kinematics.h"
#include "command_to_movement.h"

#ifndef NFLSTREET_COMMAND_NO_MAIN
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

static uint32_t angle_distance(uint32_t a, uint32_t b)
{
    uint32_t distance = (a - b) & 0xffffff;
    uint32_t opposite = 0x1000000 - distance;
    return distance < opposite ? distance : opposite;
}

static uint32_t turn_toward(uint32_t current, uint32_t target)
{
    const uint32_t limit = 0x5b05b; /* r13-0x70DC in the supplied MEM1 state */
    if (angle_distance(target, current) <= limit) return target;
    if (((target - current) & 0xffffff) > 0x800000)
        return (current - limit) & 0xffffff;
    return (current + limit) & 0xffffff;
}

int nflstreet_result_heading(const uint32_t actor[12], const uint32_t state[21],
                             uint32_t *result)
{
    uint32_t current = state[0x18 / 4];
    uint32_t prior = state[0x20 / 4];
    uint32_t requested = actor[0x08 / 4];
    if ((current | prior | requested | actor[0x04 / 4]) & 0xff000000)
        return 0;
    /* Original 0x800F6FA4 does not inspect the animation context when
       speed or maximum is zero: it turns toward the requested heading. */
    if (state[0x1c / 4] == 0 || state[0x24 / 4] == 0) {
        *result = turn_toward(current, requested);
        return 1;
    }
    uint32_t toward_prior = turn_toward(current, prior);
    uint32_t toward_requested = turn_toward(current, requested);
    if (state[0x1c / 4] != 0 && state[0x24 / 4] != 0 &&
        (angle_distance(current, prior) < 0x2e38e4 ||
         angle_distance(requested, prior) > 0x2e38e2)) {
        *result = toward_prior;
        return 1;
    }
    /* The remaining branch reads actor +0x318. Accept it only if both
       possible targets produce the same observable command. */
    if (toward_prior != toward_requested) return 0;
    *result = toward_prior;
    return 1;
}

void nflstreet_store_command(uint32_t state[21], uint32_t heading,
                             uint32_t result, uint32_t acceleration_bits)
{
    state[0x18 / 4] = result;
    state[0x38 / 4] = heading;
    state[0x34 / 4] = acceleration_bits;
}

#ifndef NFLSTREET_COMMAND_NO_MAIN
int main(int argc, char **argv)
{
    uint32_t actor[12], state[21], result, acceleration_bits;
    unsigned i;
    if ((argc != 4 && argc != 5) ||
        (argc == 5 && strcmp(argv[4], "--command-only") != 0) ||
        !parse_words(argv[1], actor, 12) ||
        !parse_words(argv[2], state, 21) ||
        !parse_words(argv[3], &acceleration_bits, 1)) {
        fputs("usage: command_to_movement_replay 96_ACTOR_HEX 168_STATE_HEX "
              "8_ACCEL_HEX [--command-only]\n", stderr);
        return 2;
    }
    if (!nflstreet_result_heading(actor, state, &result)) {
        fputs("result heading depends on an unobserved actor branch\n", stderr);
        return 3;
    }
    nflstreet_store_command(state, actor[1], result, acceleration_bits);
    if (argc == 4 && !nflstreet_step_kinematics(state)) {
        fputs("input takes an unverified movement branch\n", stderr);
        return 3;
    }
    for (i = 0; i < 21; ++i) printf("%08x", state[i]);
    putchar('\n');
    return 0;
}
#endif
