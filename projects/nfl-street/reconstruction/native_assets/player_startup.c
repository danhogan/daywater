#include "player_startup.h"

static int appearance(void *context, uint8_t *actor, size_t actor_size,
    const uint8_t *descriptor, size_t descriptor_size)
{
    return nflstreet_player_appearance_initialize(actor, actor_size,
        descriptor, descriptor_size,
        (const nflstreet_player_appearance_context *)context);
}

int nflstreet_player_initialize(uint8_t *actor, size_t actor_size,
    const nflstreet_player_constructor_input *input,
    nflstreet_actor_model_allocator allocate, void *allocation_context,
    const nflstreet_player_appearance_context *appearance_context)
{
    if (!appearance_context) return 0;
    return nflstreet_player_construct(actor, actor_size, input, allocate,
        allocation_context, appearance, (void *)appearance_context);
}
