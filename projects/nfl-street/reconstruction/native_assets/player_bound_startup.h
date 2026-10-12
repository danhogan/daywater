#ifndef NFLSTREET_PLAYER_BOUND_STARTUP_H
#define NFLSTREET_PLAYER_BOUND_STARTUP_H
#include "player_startup.h"
#include "player_model_binding.h"
/* Replaces the appearance BIND_MODEL service with owned model-binding code.
   All other appearance services and the remaining resource/render services
   are supplied by their explicit owners. Allocation ownership is unchanged. */
int nflstreet_player_bound_create(uint32_t allocation_owner,
    nflstreet_factory_allocator, void *,
    const nflstreet_player_constructor_input *,
    nflstreet_actor_model_allocator, void *,
    const nflstreet_player_appearance_context *,
    const nflstreet_player_model_binding_context *, nflstreet_factory_memory *);
#endif
