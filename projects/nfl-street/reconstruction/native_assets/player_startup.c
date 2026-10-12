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

typedef struct {
    nflstreet_actor_model_allocator allocate;
    void *allocation_context;
    const nflstreet_player_appearance_context *appearance_context;
    int initialized;
} player_creation;

static void create_player(void *opaque, nflstreet_factory_memory *memory,
    const void *input)
{
    player_creation *creation = opaque;
    if (memory->token != creation->appearance_context->actor_token) return;
    creation->initialized = nflstreet_player_initialize(memory->bytes,
        memory->size, input, creation->allocate, creation->allocation_context,
        creation->appearance_context);
}

int nflstreet_player_create(uint32_t allocation_owner,
    nflstreet_factory_allocator allocate_object, void *object_context,
    const nflstreet_player_constructor_input *input,
    nflstreet_actor_model_allocator allocate, void *allocation_context,
    const nflstreet_player_appearance_context *appearance_context,
    nflstreet_factory_memory *result)
{
    player_creation creation = {allocate, allocation_context, appearance_context, 0};
    nflstreet_factory_record record = {NFLSTREET_ACTOR_MODEL_STORAGE,
        allocation_owner, create_player, &creation};
    if (!result) return 0;
    *result = (nflstreet_factory_memory){0};
    if (!input || !allocate || !appearance_context) return 0;
    if (!nflstreet_actor_create(&record, 1, 0, 0, input, allocate_object,
        object_context, result)) return 0;
    return creation.initialized;
}
