#ifndef NFLSTREET_PLAYER_STARTUP_H
#define NFLSTREET_PLAYER_STARTUP_H
#include "player_constructor.h"
#include "player_appearance_init.h"
#include "actor_factory.h"
/* Connects owned constructor and appearance state. The caller still owns the
   factory header, resource lookup/binding services and allocation lifetime.
   A rejected service returns zero without rolling back prior writes. */
int nflstreet_player_initialize(uint8_t *, size_t,
    const nflstreet_player_constructor_input *,
    nflstreet_actor_model_allocator, void *,
    const nflstreet_player_appearance_context *);
/* Creates the original type-zero player storage/header, then runs owned
   initialization. On initialization rejection, result still identifies the
   allocated object so the caller can release it and any earlier allocations. */
int nflstreet_player_create(uint32_t allocation_owner,
    nflstreet_factory_allocator, void *,
    const nflstreet_player_constructor_input *,
    nflstreet_actor_model_allocator, void *,
    const nflstreet_player_appearance_context *, nflstreet_factory_memory *);
#endif
