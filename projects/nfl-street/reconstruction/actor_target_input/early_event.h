#ifndef NFLSTREET_EARLY_EVENT_H
#define NFLSTREET_EARLY_EVENT_H
#include <stdint.h>

/* Captured map/service inputs are explicit; no RAM readers or game policy.
   Return 1 for the reconstructed branch, 0 for unsupported inputs. A stage-2
   contact descriptor requires the separate original positioning service. */
int nflstreet_early_event(uint8_t event[12], uint32_t *flags,
                          uint8_t request[3], uint8_t control[16],
                          int owns_record, const uint8_t descriptor[6],
                          const uint8_t incoming_descriptor[6],
                          const uint8_t geometry[48],
                          const uint8_t incoming_geometry[48],
                          const uint8_t target[12], const uint8_t actor_mode[2],
                          uint32_t tracking_enabled, const uint8_t field[12],
                          uint8_t incoming_handle_tag, uint32_t incoming_408);
#endif
