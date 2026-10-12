#ifndef NFLSTREET_EVENT_DESCRIPTOR_H
#define NFLSTREET_EVENT_DESCRIPTOR_H

#include <stdint.h>

/* Bounded 0x8007C000 event construction for the observed descriptors.
   Writes the 12-byte event, flags word, and input-mode byte only.
   Code-14 service calls and other actor fields are outside this interface.
   Rejection preserves every output. No actor or event-phase scheduling. */
int nflstreet_event_from_descriptor(const uint8_t actor_header[16],
                                    const uint8_t descriptor[6],
                                    const uint32_t *roster_handles,
                                    uint16_t actors_per_team,
                                    uint16_t total_actors,
                                    uint8_t event[12],
                                    uint32_t *flags, uint8_t *input_mode);

#endif
