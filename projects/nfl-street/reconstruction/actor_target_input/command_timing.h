#ifndef NFLSTREET_COMMAND_TIMING_H
#define NFLSTREET_COMMAND_TIMING_H
#include <stddef.h>
#include <stdint.h>

/* Bounded reconstruction of 0x80177374 / 0x80176E28. State and program
   retain their original big-endian layout. Unsupported commands fail
   transactionally. This implements only the observed readiness branch. */
int nflstreet_command_timing(uint8_t state[64], const uint8_t *program,
                            size_t program_size, double delta,
                            uint32_t *actor_flags, uint16_t commands[64],
                            size_t *command_count, unsigned *complete);
int nflstreet_stream_program_lookup(const uint8_t *table, size_t table_size,
                                    uint32_t index, uint32_t *program_identity);
int nflstreet_stream_setup(uint8_t state[64], uint32_t program_identity,
                           const uint8_t *program, size_t program_size,
                           uint32_t *flags, uint16_t commands[64],
                           size_t *command_count, unsigned *complete);
/* Bounded stream phase of original 0x8017413C. Resolved nonzero tag counts
   supplied from verified loop-only source programs. No callbacks accepted. */
int nflstreet_stream_rebind_phase(uint8_t state[64],uint32_t program_identity,
                                 const uint8_t *program,size_t program_size,
                                 uint16_t new_tags,uint32_t *actor_flags);
#endif
