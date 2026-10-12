#ifndef NFLSTREET_ANIMATION_CHANNELS_H
#define NFLSTREET_ANIMATION_CHANNELS_H
#include <stddef.h>
#include <stdint.h>
/* Original big-endian layouts. Identities belong to supplied source data;
   these functions never read emulated memory. Unsupported paths are atomic. */
int nflstreet_animation_allocate(uint8_t *channels, size_t size, unsigned count,
                                int32_t *index);
int nflstreet_animation_manager_allocate(uint8_t manager[404], int32_t *index);
int nflstreet_animation_blend(uint8_t blend[20], float current, float target,
                             uint32_t duration);
int nflstreet_animation_start(uint8_t *channels, size_t size,
                             const uint8_t header[16], uint16_t bank_key,
                             const uint8_t bank[32], const uint8_t *groups,
                             size_t groups_size, uint16_t clip, uint16_t group,
                             uint16_t row_override, uint32_t program_identity,
                             const uint8_t *program, size_t program_size,
                             float weight, uint32_t *actor_flags, int32_t *index);
/* Pre-callback phase of 0x80177EF4. Transition table is resolved source data,
   not a captured slot decision. Full-pool eviction and group renaming are
   deliberately unsupported. Does not invoke the game-specific callback. */
int nflstreet_animation_group_prepare(uint8_t manager[404], uint16_t group,
                                     float target, const uint8_t *transitions,
                                     size_t transitions_size, int32_t *index);
#endif
