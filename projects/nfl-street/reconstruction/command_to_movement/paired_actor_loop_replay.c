/* Bounded GNNE69 Stewart/Thomas update pair for observed saved-match branch. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor_runtime.h"
#include "../actor_target_input/target_selection.h"

static int nibble(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static int words(const char *text, uint32_t *result, unsigned count)
{
    unsigned i, j;
    if (strlen(text) != 8 * count) return 0;
    for (i = 0; i < count; ++i) {
        uint32_t value = 0;
        for (j = 0; j < 8; ++j) {
            int digit = nibble(text[8 * i + j]);
            if (digit < 0) return 0;
            value = value << 4 | (uint32_t)digit;
        }
        result[i] = value;
    }
    return 1;
}

static int bytes(const char *text, uint8_t *result, unsigned count)
{
    unsigned i;
    for (i = 0; i < count; ++i) {
        int high = nibble(text[2 * i]), low = nibble(text[2 * i + 1]);
        if (high < 0 || low < 0) return 0;
        result[i] = (uint8_t)(high << 4 | low);
    }
    return 1;
}

static void print_words(const char *label, const uint32_t *input, unsigned count)
{
    unsigned i;
    fputs(label, stdout);
    for (i = 0; i < count; ++i) printf("%08x", input[i]);
    putchar('\n');
}

static void print_bytes(const char *label, const uint8_t *input, unsigned count)
{
    unsigned i;
    fputs(label, stdout);
    for (i = 0; i < count; ++i) printf("%02x", input[i]);
    putchar('\n');
}

static void store_word(uint8_t *output, uint32_t value)
{
    output[0] = (uint8_t)(value >> 24);
    output[1] = (uint8_t)(value >> 16);
    output[2] = (uint8_t)(value >> 8);
    output[3] = (uint8_t)value;
}

static uint32_t memory_word(const uint8_t *memory, uint32_t address)
{
    const uint8_t *p = memory + address - 0x80000000;
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}

static uint16_t memory_half(const uint8_t *memory, uint32_t address)
{
    const uint8_t *p = memory + address - 0x80000000;
    return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static int memory_range(uint32_t address, unsigned size)
{
    return address >= 0x80000000 &&
           (uint64_t)address + size <= 0x81800000;
}

static int snapshot_actor(const uint8_t *memory, uint32_t address,
                          const char *name, uint32_t *header,
                          uint32_t *bb4, uint32_t *bbc, uint32_t *actor_408,
                          uint32_t actor[12], uint32_t state[21])
{
    unsigned i;
    if (!memory_range(address, 0x14D4) ||
        memcmp(memory + address - 0x80000000 + 0xB80,
               name, strlen(name) + 1) != 0) return 0;
    *header = memory_word(memory, address);
    *bb4 = memory_half(memory, address + 0xBB4);
    *bbc = memory_half(memory, address + 0xBBC);
    *actor_408 = memory_word(memory, address + 0x408);
    for (i = 0; i < 12; ++i)
        actor[i] = memory_word(memory, address + 0x200 + 4 * i);
    for (i = 0; i < 21; ++i)
        state[i] = memory_word(memory, address + 0x1A8 + 4 * i);
    return 1;
}

static int load_snapshot(const char *path,
                         uint32_t *s_header, uint32_t *s_bb4,
                         uint32_t *s_bbc, uint32_t *s_408,
                         uint32_t s_actor[12], uint32_t s_state[21],
                         uint32_t *t_header, uint32_t *t_bb4,
                         uint32_t *t_bbc, uint32_t *t_408,
                         uint32_t t_actor[12], uint32_t t_state[21],
                         uint32_t *handle, uint32_t *per_team,
                         uint32_t *selected_slot)
{
    FILE *file = fopen(path, "rb");
    uint8_t *memory;
    uint32_t choice_manager, roster_manager, base, record;
    unsigned index;
    int ok = 0;
    if (!file) return 0;
    memory = malloc(0x1800000);
    if (!memory) { fclose(file); return 0; }
    if (fread(memory, 1, 0x1800000, file) != 0x1800000 ||
        fgetc(file) != EOF) goto done;
    choice_manager = memory_word(memory, 0x803A9A40 - 0x73D0);
    roster_manager = memory_word(memory, 0x803A9A40 - 0x7C30);
    if (!memory_range(choice_manager, 0xF4 + 8) ||
        !memory_range(roster_manager, 0xC)) goto done;
    index = memory[choice_manager - 0x80000000 + 1];
    record = choice_manager + 8 + index * 0xF4;
    if (!memory_range(record, 0xB8)) goto done;
    *handle = memory_word(memory, record + 0xB4);
    *per_team = memory_half(memory, roster_manager + 8);
    base = memory_word(memory, roster_manager);
    if (index != 0 || *per_team != 7 || *handle != 0x00010101 ||
        base != 0x803B7320) goto done;
    *selected_slot = 8;
    ok = snapshot_actor(memory, base + 7 * 0x14D4, "Stewart",
                        s_header, s_bb4, s_bbc, s_408, s_actor, s_state) &&
         snapshot_actor(memory, base + 8 * 0x14D4, "Thomas",
                        t_header, t_bb4, t_bbc, t_408, t_actor, t_state);
done:
    free(memory);
    fclose(file);
    return ok;
}

int main(int argc, char **argv)
{
    nflstreet_actor_runtime stewart, thomas;
    uint32_t s_header, s_bb4, s_bbc, s_408, s_actor[12], s_state[21];
    uint32_t t_header, t_bb4, t_bbc, t_408, t_actor[12], t_state[21];
    uint32_t selection_handle, actors_per_team, selected_slot;
    uint16_t resolved_slot;
    uint8_t vectors[8][0x20], position[12], target[12];
    static const uint8_t offset[8] = {0, 0, 0, 0, 0xc0, 0, 0, 0};
    unsigned count, step, i;
    int snapshot_mode = argc == 5 && strcmp(argv[1], "--mem1") == 0;
    const char *count_arg, *vector_arg;
    char *end;
    unsigned long parsed;
    if (snapshot_mode) {
        if (!load_snapshot(argv[2], &s_header, &s_bb4, &s_bbc, &s_408,
                           s_actor, s_state, &t_header, &t_bb4, &t_bbc,
                           &t_408, t_actor, t_state, &selection_handle,
                           &actors_per_team, &selected_slot)) return 2;
        count_arg = argv[3];
        vector_arg = argv[4];
    } else {
        if (argc != 18 ||
        !words(argv[1], &s_header, 1) || !words(argv[2], &s_bb4, 1) ||
        !words(argv[3], s_actor, 12) || !words(argv[4], s_state, 21) ||
        !words(argv[5], &s_bbc, 1) || !words(argv[6], &s_408, 1) ||
        !words(argv[7], &t_header, 1) || !words(argv[8], &t_bb4, 1) ||
        !words(argv[9], t_actor, 12) || !words(argv[10], t_state, 21) ||
        !words(argv[11], &t_bbc, 1) || !words(argv[12], &t_408, 1) ||
        !words(argv[15], &selection_handle, 1) ||
        !words(argv[16], &actors_per_team, 1) ||
        !words(argv[17], &selected_slot, 1)) return 2;
        count_arg = argv[13];
        vector_arg = argv[14];
    }
    if (
        s_bb4 > 0xffff || s_bbc > 0xffff ||
        t_bb4 > 0xffff || t_bbc > 0xffff ||
        actors_per_team == 0 || actors_per_team > UINT16_MAX / 2 ||
        selected_slot > UINT16_MAX) return 2;
    parsed = strtoul(count_arg, &end, 10);
    if (!*count_arg || *end || parsed < 2 || parsed > 8) return 2;
    count = (unsigned)parsed;
    if (strlen(vector_arg) != count * 64 + count - 1) return 2;
    for (step = 0; step < count; ++step) {
        char vector[65];
        if (step && vector_arg[step * 65 - 1] != ',') return 2;
        memcpy(vector, vector_arg + step * 65, 64);
        vector[64] = '\0';
        if (!bytes(vector, vectors[step], 0x20)) return 2;
    }
    nflstreet_actor_runtime_init(&stewart, s_header, (int16_t)s_bb4,
                                 (int16_t)s_bbc, s_408, s_actor, s_state);
    nflstreet_actor_runtime_init(&thomas, t_header, (int16_t)t_bb4,
                                 (int16_t)t_bbc, t_408, t_actor, t_state);
    if (snapshot_mode) thomas.completed = 1;
    for (step = 0; step < count; ++step) {
        if (step) {
            if (!nflstreet_actor_slot_from_handle(
                    selection_handle, (uint16_t)actors_per_team,
                    (uint16_t)(2 * actors_per_team), &resolved_slot) ||
                resolved_slot != selected_slot) return 3;
            for (i = 0; i < 3; ++i)
                store_word(position + i * 4, thomas.state[i]);
            if (!nflstreet_observed_actor_target(position, offset, target))
                return 3;
            print_bytes("target:", target, 12);
            if (!nflstreet_actor_runtime_step_target(&stewart, target)) return 3;
        } else if (!nflstreet_actor_runtime_step(&stewart, s_actor)) return 3;
        print_words("stewart_actor:", stewart.actor, 12);
        print_words("stewart_state:", stewart.state, 21);
        if (!nflstreet_actor_runtime_step_vector(&thomas, vectors[step])) return 3;
        print_words("thomas_actor:", thomas.actor, 12);
        print_words("thomas_state:", thomas.state, 21);
    }
    return 0;
}
