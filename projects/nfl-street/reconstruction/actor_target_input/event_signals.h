#ifndef NFLSTREET_EVENT_SIGNALS_H
#define NFLSTREET_EVENT_SIGNALS_H
#include <stdint.h>

/* Effects of accepted original parser commands C004/C006/C00C. Parser context
   routing and marker timing are not part of this bounded handler. */
int nflstreet_event_flag_command(uint32_t command, uint32_t *flags);

/* Stage-three portion up to 0x8007C7BC on a handoff, 0x8007C808 on a wait,
   for observed code-4/code-14 events.
   Waits unless bit 1 is set, then clears it and reassigns the actor record.
   Remaining service effects are outside this interface. Rejection preserves
   flags, records, and both outputs. Waiting leaves record_index unchanged. */
int nflstreet_stage3_record_poll(const uint8_t event[12], uint32_t *flags,
                                 uint32_t *handles, uint8_t record_count,
                                 uint32_t current_actor_handle,
                                 uint32_t incoming_actor_flags, uint32_t dispatch_value,
                                 uint16_t actors_per_team, uint16_t total_actors,
                                 uint8_t *record_index, uint8_t *changed);
#endif
