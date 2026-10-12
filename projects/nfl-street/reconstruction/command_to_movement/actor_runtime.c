/* Stateful host boundary for the verified GNNE69 Thomas movement branch. */
#include <string.h>

#include "actor_runtime.h"
#include "actor_update.h"
#include "reset_acceleration.h"
#include "../kinematics/kinematics.h"
#include "../input_translation/input_translation.h"
#include "../actor_target_input/actor_target_input.h"
#include "../actor_target_input/target_selection.h"

static void store_word(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

static uint32_t load_word(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | bytes[3];
}

void nflstreet_actor_runtime_init(nflstreet_actor_runtime *runtime,
                                  uint32_t header, int16_t rating_bb4,
                                  int16_t rating_bbc, uint32_t actor_408,
                                  const uint32_t actor[12],
                                  const uint32_t state[21])
{
    runtime->header = header;
    runtime->rating_bb4 = rating_bb4;
    runtime->rating_bbc = rating_bbc;
    runtime->actor_408 = actor_408;
    memcpy(runtime->actor, actor, sizeof(runtime->actor));
    memcpy(runtime->state, state, sizeof(runtime->state));
    runtime->completed = 0;
}

int nflstreet_actor_runtime_step(nflstreet_actor_runtime *runtime,
                                 const uint32_t actor[12])
{
    uint32_t next_state[21];
    memcpy(next_state, runtime->state, sizeof(next_state));
    if (runtime->completed != 0) {
        /* 0x80036AF8 clears these fields before 0x800F7C90. */
        next_state[0x48 / 4] = 0;
        next_state[0x3c / 4] = 0x3b6bedfa;
        next_state[0x34 / 4] = 0;
        if (!nflstreet_reset_acceleration(runtime->header, runtime->rating_bbc,
                                          runtime->actor_408, actor, next_state))
            return 0;
    }
    if (!nflstreet_actor_update(runtime->header, runtime->rating_bb4,
                                actor, next_state, 0))
        return 0;
    memcpy(runtime->state, next_state, sizeof(runtime->state));
    memcpy(runtime->actor, actor, sizeof(runtime->actor));
    ++runtime->completed;
    return 1;
}

int nflstreet_actor_runtime_brake(nflstreet_actor_runtime *runtime)
{
    if (!runtime || (runtime->header & 0xff) != 1 || runtime->actor_408 != 0 ||
        (runtime->actor[8] & 0x00ff0000) != 0) return 0;
    uint32_t next[21];
    memcpy(next, runtime->state, sizeof next);
    /* 0x80036AF8 then zero-status table handler 0x800F7C3C.
       This branch bypasses acceleration setup in 0x800F86BC. */
    next[0x48 / 4] = 0;
    next[0x3c / 4] = 0x3b6bedfa;
    next[0x34 / 4] = 0;
    if (!nflstreet_step_kinematics(next)) return 0;
    memcpy(runtime->state, next, sizeof next);
    runtime->actor[0] = 0;
    /* Original caller clears transient requested/active status bytes. */
    runtime->actor[3] &= 0xffff00ff;
    runtime->actor[7] &= 0xffff00ff;
    ++runtime->completed;
    return 1;
}

int nflstreet_actor_runtime_step_cardinal(nflstreet_actor_runtime *runtime,
                                          char direction)
{
    uint32_t next_actor[12], heading;
    const unsigned offsets[] = {1, 2, 5, 6};
    unsigned i;
    if (direction == 'R')
        heading = nflstreet_angle_from_vector(0.0f, 1.0f);
    else if (direction == 'F')
        heading = nflstreet_angle_from_vector(1.0f, 0.0f);
    else if (direction == 'L')
        heading = nflstreet_angle_from_vector(0.0f, -1.0f);
    else
        return 0;
    memcpy(next_actor, runtime->actor, sizeof(next_actor));
    if (runtime->completed == 0) {
        for (i = 0; i < 4; ++i)
            if (next_actor[offsets[i]] != heading) return 0;
    } else {
        for (i = 0; i < 4; ++i)
            next_actor[offsets[i]] = heading;
    }
    return nflstreet_actor_runtime_step(runtime, next_actor);
}

int nflstreet_actor_runtime_step_vector(nflstreet_actor_runtime *runtime,
                                        const uint8_t input[0x20])
{
    uint8_t fields[0x30];
    uint32_t next_actor[12];
    unsigned i;
    /* Only the observed equal-strength handoff is covered here. */
    if (runtime->actor[0] != runtime->actor[4] &&
        !(runtime->actor[0] == 0 && runtime->state[0x34 / 4] == 0 &&
          (runtime->actor[3] & 0x0000ff00) == 0 &&
          (runtime->actor[7] & 0x0000ff00) == 0)) return 0;
    for (i = 0; i < 12; ++i)
        store_word(fields + i * 4, runtime->actor[i]);
    if (!nflstreet_project_directional_input(input, fields)) return 0;
    /* 0x80014AF4/FC copy the requested heading into the active fields. */
    memcpy(fields + 0x14, fields + 4, 8);
    fields[0x1e] = 1;
    /* 0x800F8618 caps both strengths at DOL 0x8025458C (0.72f). */
    {
        uint32_t strength = load_word(fields);
        float requested, limit;
        const uint32_t limit_bits = 0x3f3851ec;
        memcpy(&requested, &strength, sizeof requested);
        memcpy(&limit, &limit_bits, sizeof limit);
        if (requested > limit) strength = limit_bits;
        store_word(fields, strength);
        store_word(fields + 0x10, strength);
    }
    for (i = 0; i < 12; ++i)
        next_actor[i] = load_word(fields + i * 4);
    return nflstreet_actor_runtime_step(runtime, next_actor);
}

int nflstreet_actor_runtime_step_target(nflstreet_actor_runtime *runtime,
                                        const uint8_t target[12])
{
    uint8_t fields[0x30], position[12];
    uint32_t next_actor[12];
    unsigned i;
    if (runtime->completed == 0 || runtime->actor[0] != runtime->actor[4])
        return 0;
    for (i = 0; i < 12; ++i)
        store_word(fields + i * 4, runtime->actor[i]);
    for (i = 0; i < 3; ++i)
        store_word(position + i * 4, runtime->state[i]);
    if (!nflstreet_project_actor_target(target, position, fields)) return 0;
    /* Observed 0x800148E0 handoff and 0x800F8254/0x800F8618 cap. */
    memcpy(fields + 0x14, fields + 4, 8);
    fields[0x0f] = 0;
    fields[0x1e] = 1;
    {
        uint32_t strength = load_word(fields);
        float requested, limit;
        const uint32_t limit_bits = 0x3f3851ec;
        memcpy(&requested, &strength, sizeof requested);
        memcpy(&limit, &limit_bits, sizeof limit);
        if (requested > limit) strength = limit_bits;
        store_word(fields, strength);
        store_word(fields + 0x10, strength);
    }
    for (i = 0; i < 12; ++i)
        next_actor[i] = load_word(fields + i * 4);
    return nflstreet_actor_runtime_step(runtime, next_actor);
}

int nflstreet_actor_runtime_step_observed_follow(
    nflstreet_actor_runtime *runtime, const uint8_t other_position[12])
{
    static const uint8_t observed_offset[8] =
        {0, 0, 0, 0, 0xc0, 0, 0, 0};
    uint8_t target[12];
    if (!nflstreet_observed_actor_target(other_position, observed_offset,
                                         target)) return 0;
    return nflstreet_actor_runtime_step_target(runtime, target);
}
