#include "event_signals.h"
#include "target_selection.h"

static uint32_t be32(const uint8_t *data)
{
    return (uint32_t)data[0] << 24 | (uint32_t)data[1] << 16 |
           (uint32_t)data[2] << 8 | data[3];
}

int nflstreet_event_flag_command(uint32_t command, uint32_t *flags)
{
    if (!flags) return 0;
    if (command == 0xc004) *flags |= 4;
    else if (command == 0xc006) *flags |= 1;
    else if (command == 0xc00b) *flags |= 0x100;
    else if (command == 0xc00c) *flags |= 0x40000;
    else return 0;
    return 1;
}

int nflstreet_stage3_record_poll(const uint8_t event[12], uint32_t *flags,
                                 uint32_t *handles, uint8_t record_count,
                                 uint32_t current_actor_handle,
                                 uint32_t incoming_actor_flags, uint32_t dispatch_value,
                                 uint16_t actors_per_team, uint16_t total_actors,
                                 uint8_t *record_index, uint8_t *changed)
{
    uint16_t slot;
    uint32_t code;
    if (!event || !flags || !handles || !record_count || !record_index || !changed ||
        event[4] != 0 || event[5] != 3 || dispatch_value != 3 ||
        !nflstreet_actor_slot_from_handle(current_actor_handle, actors_per_team,
                                          total_actors, &slot) ||
        !nflstreet_actor_slot_from_handle(be32(event), actors_per_team,
                                          total_actors, &slot)) return 0;
    code = be32(event + 8);
    if (code != 4 && code != 14) return 0;
    if (!(*flags & 1)) {
        *changed = 0;
        return 1;
    }
    if (!nflstreet_record_handle_handoff(handles, record_count, current_actor_handle,
                                         be32(event), actors_per_team, total_actors,
                                         record_index)) return 0;
    *flags &= ~UINT32_C(1);
    /* 0x8003D81C -> 0x8003D704 clears old actor flags when the
       incoming actor lacks bit 0x400. Other transfer effects are separate. */
    if ((*flags & 0x400) && !(incoming_actor_flags & 0x400))
        *flags &= ~UINT32_C(0x4400);
    *changed = 1;
    return 1;
}
