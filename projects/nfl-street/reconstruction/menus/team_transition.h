#ifndef NFLSTREET_TEAM_TRANSITION_H
#define NFLSTREET_TEAM_TRANSITION_H

#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_TEAM_TRANSITION_OBJECT_MIN_SIZE 0x14u

typedef struct {
 uint32_t audio_requested;
 uint32_t audio_handle;
 uint32_t audio_delay;
 uint32_t audio_reserved;
 uint32_t broadcast_context;
 uint32_t broadcast_property;
 uint32_t broadcast_arg0;
 uint32_t broadcast_arg1;
} nflstreet_team_transition_effects;

/* Bounded model of 80137B84. object is the caller-owned transition object;
   clock_value is supplied from 801AF464. Effects describe calls to audio and
   broadcast services without executing either service. */
int nflstreet_team_transition_apply(uint8_t *object,size_t object_size,
 uint32_t delay,uint32_t clock_value,uint32_t audio_enabled,
 uint32_t broadcast_context,nflstreet_team_transition_effects *effects);

typedef struct {
 uint32_t completed;
 uint32_t audio_stop_requested;
 uint32_t audio_handle;
} nflstreet_team_transition_completion;

/* Only 80137854's phase3 unsigned deadline branch and 80137C80's completion
   writes. Other owner phases, global audio update, and match creation excluded.
   The host supplies the same clock domain used when applying the transition. */
int nflstreet_team_transition_advance(uint8_t *object,size_t object_size,
 uint32_t clock_value,uint32_t audio_enabled,
 nflstreet_team_transition_completion *effects);

#endif
