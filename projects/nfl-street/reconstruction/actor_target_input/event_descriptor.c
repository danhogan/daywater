#include "event_descriptor.h"
#include "target_selection.h"

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

int nflstreet_event_from_descriptor(const uint8_t actor_header[16],
                                    const uint8_t descriptor[6],
                                    const uint32_t *roster_handles,
                                    uint16_t actors_per_team,
                                    uint16_t total_actors,
                                    uint8_t event[12],
                                    uint32_t *flags, uint8_t *input_mode)
{
    uint8_t result[12];
    uint32_t index;
    uint16_t current_slot, resolved_slot;
    if (!actor_header || !descriptor || !roster_handles || !event ||
        !flags || !input_mode || !actors_per_team || !total_actors ||
        descriptor[0] != 0x1a || descriptor[5] != 0 ||
        !((descriptor[2] == 4 && descriptor[3] == 2 &&
           (descriptor[4] == 0 || descriptor[4] == 0x21)) ||
          (descriptor[2] == 14 && descriptor[3] == 0 && descriptor[4] == 0x21)) ||
        descriptor[1] >= actors_per_team ||
        !nflstreet_actor_slot_from_handle(be32(actor_header), actors_per_team,
                                          total_actors, &current_slot) ||
        roster_handles[current_slot] != be32(actor_header)) return 0;
    index = actor_header[2] * (uint32_t)actors_per_team + descriptor[1];
    if (index >= total_actors ||
        !nflstreet_actor_slot_from_handle(roster_handles[index], actors_per_team,
                                          total_actors, &resolved_slot) ||
        resolved_slot != index) return 0;
    memcpy(result, event, sizeof result);
    put_be32(result, roster_handles[index]);
    result[4] = result[5] = 0;
    result[6] = descriptor[3];
    /* Byte 7 is not written by the original event constructor. */
    put_be32(result + 8, descriptor[2]);
    memcpy(event, result, sizeof result);
    *flags = be32(actor_header + 12) & ~UINT32_C(5);
    *input_mode = 0;
    return 1;
}
