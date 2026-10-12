#ifndef NFLSTREET_TARGET_SELECTION_H
#define NFLSTREET_TARGET_SELECTION_H

#include <stdint.h>

/* GNNE69 0x8008D764 branch observed for Stewart targeting Thomas.
   Returns 0 for offsets or coordinates outside the measured branch. */
int nflstreet_observed_actor_target(const uint8_t other_position[12],
                                    const uint8_t offset_xy[8],
                                    uint8_t target[12]);

/* DOL 0x80020944 -> 0x8003A4A0 handle branch; caller supplies roster bounds. */
int nflstreet_actor_slot_from_handle(uint32_t handle,
                                     uint16_t actors_per_team,
                                     uint16_t total_actors,
                                     uint16_t *slot);

/* Bounded record lookup (0x800BE4F8) followed by the observed handle copy.
   This only changes the handle word; event scheduling and the assignment
   wrapper's other side effects are outside this interface. On rejection,
   handles and record_index are unchanged. Only observed tag-1 actors and
   empty record handles are supported. */
int nflstreet_record_handle_handoff(uint32_t *handles, uint8_t record_count,
                                    uint32_t current_actor_handle,
                                    uint32_t new_actor_handle,
                                    uint16_t actors_per_team,
                                    uint16_t total_actors,
                                    uint8_t *record_index);

#endif
