#ifndef NFLSTREET_CHANNEL_CALLBACKS_H
#define NFLSTREET_CHANNEL_CALLBACKS_H
#include "actor_channels.h"
/* GNNE69 rev 0, DOL SHA1 46e3de854998a0f66c9be8c72a1164d22cd29eb4.
   Candidate: instruction-ordered native C transcription; original execution
   verification is deliberately left to the owner. No original module needed.
   The private stack preserves callback temporaries across invocations. */
#define NFLSTREET_CALLBACK_STACK_LO 0x80f38000u
#define NFLSTREET_CALLBACK_STACK_HI 0x80f47000u
typedef struct {
 uint8_t stack[NFLSTREET_CALLBACK_STACK_HI-NFLSTREET_CALLBACK_STACK_LO];
 uint32_t failure_pc,failure_address;unsigned events,groups,operands;
 int ok,failure_budget_exhausted;
} nflstreet_channel_callbacks;
void nflstreet_channel_callbacks_init(nflstreet_channel_callbacks *);
/* Install these in channels.event/group, and a callback context in user.
   The callback context is separate from channels and must outlive it.
   Check callbacks.ok after EVERY update (actor_channels resets its own ok
   between phases). Failures retain the first address/PC; never silently skip. */
uint32_t nflstreet_channel_callbacks_event(void *,nflstreet_actor_channels *,uint32_t actor,uint32_t command,uint32_t key);
void nflstreet_channel_callbacks_group(void *,nflstreet_actor_channels *,uint32_t function,uint32_t resources,uint32_t key,uint32_t set,uint32_t channels,uint32_t actor,uint32_t mode);
#endif
