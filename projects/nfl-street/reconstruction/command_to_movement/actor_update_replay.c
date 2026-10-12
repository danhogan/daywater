/* GNNE69 observed preparation -> command -> movement path, without Dolphin. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../kinematics/kinematics.h"
#include "actor_update.h"
#include "command_to_movement.h"
#include "preparation.h"

int nflstreet_actor_update(uint32_t header, int16_t rating,
                           const uint32_t actor[12], uint32_t state[21],
                           int stage)
{
    uint32_t heading;
    if (stage < 0 || stage > 2 ||
        !nflstreet_prepare_acceleration(header, actor, rating, state))
        return 0;
    if (stage != 1) {
        uint32_t acceleration_bits = state[0x34 / 4];
        if (!nflstreet_result_heading(actor, state, &heading)) return 0;
        nflstreet_store_command(state, actor[1], heading, acceleration_bits);
        if (stage == 0 && !nflstreet_step_kinematics(state)) return 0;
    }
    return 1;
}

#ifndef NFLSTREET_ACTOR_UPDATE_NO_MAIN
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
    uint32_t header, rating, actor[12], state[21];
    unsigned i;
    int stage = 0;
    if ((argc != 5 && argc != 6) ||
        !parse_words(argv[1], &header, 1) ||
        !parse_words(argv[2], &rating, 1) || rating > 0xffff ||
        !parse_words(argv[3], actor, 12) ||
        !parse_words(argv[4], state, 21)) {
        fputs("usage: actor_update_replay 8_HEADER_HEX 8_RATING_HEX "
              "96_ACTOR_HEX 168_STATE_HEX [--preparation-only|--command-only]\n",
              stderr);
        return 2;
    }
    if (argc == 6) {
        if (strcmp(argv[5], "--preparation-only") == 0) stage = 1;
        else if (strcmp(argv[5], "--command-only") == 0) stage = 2;
        else return 2;
    }
    if (!nflstreet_actor_update(header, (int16_t)rating, actor, state, stage)) {
        fputs("input takes an unverified actor-update branch\n", stderr);
        return 3;
    }
    for (i = 0; i < 21; ++i) printf("%08x", state[i]);
    putchar('\n');
    return 0;
}
#endif
