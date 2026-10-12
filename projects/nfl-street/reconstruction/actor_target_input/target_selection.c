/* Bounded GNNE69 actor-target vector composition. */
#include "target_selection.h"

#include <math.h>
#include <string.h>

static uint32_t be32(const uint8_t *data)
{
    return (uint32_t)data[0] << 24 | (uint32_t)data[1] << 16 |
           (uint32_t)data[2] << 8 | data[3];
}

static void put_be32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static float be_float(const uint8_t *data)
{
    uint32_t bits = be32(data);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

int nflstreet_observed_actor_target(const uint8_t other_position[12],
                                    const uint8_t offset_xy[8],
                                    uint8_t target[12])
{
    static const uint8_t observed_offset[8] =
        {0, 0, 0, 0, 0xc0, 0, 0, 0};
    float x, y;
    uint32_t bits;
    if (memcmp(offset_xy, observed_offset, sizeof observed_offset) != 0 ||
        be32(other_position + 8) != 0) return 0;
    x = be_float(other_position);
    y = be_float(other_position + 4);
    if (!isfinite(x) || !isfinite(y)) return 0;
    /* The original vector-add branch uses single-precision results. */
    x = x + 0.0f;
    y = y - 2.0f;
    memcpy(&bits, &x, sizeof bits);
    put_be32(target, bits);
    memcpy(&bits, &y, sizeof bits);
    put_be32(target + 4, bits);
    memcpy(target + 8, other_position + 8, 4);
    return 1;
}

int nflstreet_actor_slot_from_handle(uint32_t handle,
                                     uint16_t actors_per_team,
                                     uint16_t total_actors,
                                     uint16_t *slot)
{
    uint32_t team, member, index;
    if (!slot || !actors_per_team || !total_actors || (handle & 0xff) != 1)
        return 0;
    team = (handle >> 8) & 0xff;
    member = (handle >> 16) & 0xff;
    index = team * actors_per_team + member;
    if (index >= total_actors || index > UINT16_MAX) return 0;
    *slot = (uint16_t)index;
    return 1;
}

int nflstreet_record_handle_handoff(uint32_t *handles, uint8_t record_count,
                                    uint32_t current_actor_handle,
                                    uint32_t new_actor_handle,
                                    uint16_t actors_per_team,
                                    uint16_t total_actors,
                                    uint8_t *record_index)
{
    uint16_t current_slot, new_slot, slot;
    unsigned i;
    if (!handles || !record_index || !record_count ||
        !nflstreet_actor_slot_from_handle(current_actor_handle, actors_per_team,
                                          total_actors, &current_slot) ||
        !nflstreet_actor_slot_from_handle(new_actor_handle, actors_per_team,
                                          total_actors, &new_slot)) return 0;
    for (i = 0; i < record_count; ++i) {
        if (!handles[i]) continue;
        if (!nflstreet_actor_slot_from_handle(handles[i], actors_per_team,
                                              total_actors, &slot)) return 0;
        if (slot == current_slot) {
            handles[i] = new_actor_handle;
            *record_index = (uint8_t)i;
            return 1;
        }
    }
    return 0;
}
