/* Consecutive GNNE69 actor updates with the observed intercall reset. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor_update.h"
#include "actor_runtime.h"

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

static int parse_bytes(const char *source, uint8_t *out, unsigned count)
{
    unsigned i;
    if (strlen(source) != count * 2) return 0;
    for (i = 0; i < count; ++i) {
        int high = digit(source[2 * i]), low = digit(source[2 * i + 1]);
        if (high < 0 || low < 0) return 0;
        out[i] = (uint8_t)((high << 4) | low);
    }
    return 1;
}

int main(int argc, char **argv)
{
    uint32_t header, rating, actor[12], actor_sequence[8][12];
    uint8_t vector_sequence[8][0x20];
    uint8_t target_sequence[7][12];
    uint8_t other_position_sequence[7][12];
    uint32_t state[21], base_acceleration;
    uint32_t reset_rating = 0, actor_408 = 0;
    nflstreet_actor_runtime runtime;
    const char *stick_path = NULL;
    int vector_mode = 0;
    int target_mode = 0;
    int follow_mode = 0;
    unsigned updates, step, i;
    char *end = NULL;
    unsigned long parsed;
    if ((argc != 6 && argc != 8 && argc != 9) ||
        !parse_words(argv[1], &header, 1) ||
        !parse_words(argv[2], &rating, 1) || rating > 0xffff ||
        !parse_words(argv[3], actor, 12) ||
        !parse_words(argv[4], state, 21) ||
        (argc >= 8 && (!parse_words(argv[6], &reset_rating, 1) ||
                       reset_rating > 0xffff ||
                       !parse_words(argv[7], &actor_408, 1)))) {
        fputs("usage: actor_update_loop_replay 8_HEADER_HEX 8_RATING_HEX "
              "96_ACTOR_HEX 168_STATE_HEX UPDATE_COUNT "
              "[8_RESET_RATING_HEX 8_ACTOR408_HEX "
              "[ACTOR_BLOCKS_OR_RFL_PATH_OR_V:INPUT_VECTORS_OR_T:TARGETS"
              "_OR_P:OTHER_POSITIONS]]\n", stderr);
        return 2;
    }
    parsed = strtoul(argv[5], &end, 10);
    if (*argv[5] == '\0' || *end != '\0' || parsed < 2 || parsed > 8)
        return 2;
    updates = (unsigned)parsed;
    if (argc == 9) {
        const char *blocks = argv[8];
        if (strlen(blocks) == updates) {
            stick_path = blocks;
            for (step = 0; step < updates; ++step)
                if (blocks[step] != 'R' && blocks[step] != 'F' &&
                    blocks[step] != 'L') return 2;
        } else if (blocks[0] == 'V' && blocks[1] == ':') {
            const char *vectors = blocks + 2;
            vector_mode = 1;
            if (strlen(vectors) != updates * 64 + updates - 1) return 2;
            for (step = 0; step < updates; ++step) {
                char vector[65];
                memcpy(vector, vectors + step * 65, 64);
                vector[64] = '\0';
                if ((step > 0 && vectors[step * 65 - 1] != ',') ||
                    !parse_bytes(vector, vector_sequence[step], 0x20)) return 2;
            }
        } else if ((blocks[0] == 'T' || blocks[0] == 'P') &&
                   blocks[1] == ':') {
            const char *targets = blocks + 2;
            target_mode = blocks[0] == 'T';
            follow_mode = blocks[0] == 'P';
            if (strlen(targets) != (updates - 1) * 24 + updates - 2) return 2;
            for (step = 0; step + 1 < updates; ++step) {
                char target[25];
                memcpy(target, targets + step * 25, 24);
                target[24] = '\0';
                if ((step > 0 && targets[step * 25 - 1] != ',') ||
                    !(target_mode ? parse_bytes(target, target_sequence[step], 12) :
                      parse_bytes(target, other_position_sequence[step], 12)))
                    return 2;
            }
        } else {
            if (strlen(blocks) != updates * 96 + updates - 1) return 2;
            for (step = 0; step < updates; ++step) {
                char block[97];
                memcpy(block, blocks + step * 97, 96);
                block[96] = '\0';
                if ((step > 0 && blocks[step * 97 - 1] != ',') ||
                    !parse_words(block, actor_sequence[step], 12)) return 2;
            }
        }
        if (!stick_path && !vector_mode && !target_mode && !follow_mode &&
            memcmp(actor, actor_sequence[0], sizeof(actor)) != 0)
            return 2;
    }
    base_acceleration = state[0x34 / 4];
    if (argc >= 8)
        nflstreet_actor_runtime_init(&runtime, header, (int16_t)rating,
                                     (int16_t)reset_rating, actor_408,
                                     actor, state);
    for (step = 0; step < updates; ++step) {
        if (argc >= 8) {
            const uint32_t *current_actor = argc == 9 && !stick_path &&
                                            !vector_mode && !target_mode &&
                                            !follow_mode ?
                                            actor_sequence[step] : actor;
            int ok = follow_mode ?
                     (step == 0 ? nflstreet_actor_runtime_step(&runtime, actor) :
                      nflstreet_actor_runtime_step_observed_follow(
                          &runtime, other_position_sequence[step - 1])) :
                     target_mode ?
                     (step == 0 ? nflstreet_actor_runtime_step(&runtime, actor) :
                      nflstreet_actor_runtime_step_target(&runtime,
                                                          target_sequence[step - 1])) :
                     vector_mode ?
                     nflstreet_actor_runtime_step_vector(&runtime,
                                                          vector_sequence[step]) :
                     stick_path ?
                     nflstreet_actor_runtime_step_cardinal(&runtime,
                                                             stick_path[step]) :
                     nflstreet_actor_runtime_step(&runtime, current_actor);
            if (!ok) {
                fputs("input takes an unverified actor-runtime branch\n", stderr);
                return 3;
            }
            memcpy(state, runtime.state, sizeof(state));
        } else {
            if (step > 0) state[0x34 / 4] = base_acceleration;
            if (!nflstreet_actor_update(header, (int16_t)rating,
                                        actor, state, 0)) {
                fputs("input takes an unverified actor-update branch\n", stderr);
                return 3;
            }
        }
        if (vector_mode || target_mode || follow_mode) {
            fputs("actor:", stdout);
            for (i = 0; i < 12; ++i) printf("%08x", runtime.actor[i]);
            putchar('\n');
        }
        for (i = 0; i < 21; ++i) printf("%08x", state[i]);
        putchar('\n');
    }
    return 0;
}
