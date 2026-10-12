#ifndef NFLSTREET_ACTOR_RUNTIME_H
#define NFLSTREET_ACTOR_RUNTIME_H

#include <stdint.h>

/* Guarded GNNE69 revision 0 actor movement state for the observed branch. */
typedef struct {
    uint32_t header;
    int16_t rating_bb4;
    int16_t rating_bbc;
    uint32_t actor_408;
    uint32_t actor[12];
    uint32_t state[21];
    unsigned completed;
} nflstreet_actor_runtime;

void nflstreet_actor_runtime_init(nflstreet_actor_runtime *runtime,
                                  uint32_t header, int16_t rating_bb4,
                                  int16_t rating_bbc, uint32_t actor_408,
                                  const uint32_t actor[12],
                                  const uint32_t state[21]);

/* Returns 0 when the input takes a branch not covered by this reconstruction. */
int nflstreet_actor_runtime_step(nflstreet_actor_runtime *runtime,
                                 const uint32_t actor[12]);

/* Grounded zero-status handler: reset, zero requested strength/acceleration,
   preserve maximum/heading, then original braking kinematics. */
int nflstreet_actor_runtime_brake(nflstreet_actor_runtime *runtime);

/* Full-strength cardinal directions: R, F, L. Initial heading must match. */
int nflstreet_actor_runtime_step_cardinal(nflstreet_actor_runtime *runtime,
                                          char direction);

/* Takes the game's normalized 0x20-byte cardinal vector for this branch. */
int nflstreet_actor_runtime_step_vector(nflstreet_actor_runtime *runtime,
                                        const uint8_t input[0x20]);

/* Later-call target-position source observed for roster slot 7. */
int nflstreet_actor_runtime_step_target(nflstreet_actor_runtime *runtime,
                                        const uint8_t target[12]);

/* Observed slot-7 branch: form a target from the passed slot-8 position. */
int nflstreet_actor_runtime_step_observed_follow(
    nflstreet_actor_runtime *runtime, const uint8_t other_position[12]);

#endif
