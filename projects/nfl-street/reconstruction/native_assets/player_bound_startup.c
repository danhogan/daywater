#include "player_bound_startup.h"
typedef struct {
    const nflstreet_player_appearance_context *appearance;
    const nflstreet_player_model_binding_context *binding;
} bound_services;

static int appearance_service(void *opaque,
    const nflstreet_player_appearance_request *request,
    uint8_t *actor, size_t size, uint32_t *out)
{
    bound_services *services = opaque;
    if (request->kind == NFLSTREET_APPEARANCE_BIND_MODEL) {
        *out = 0;
        return nflstreet_player_model_bind(actor, size, services->binding);
    }
    return services->appearance->service(services->appearance->service_context,
        request, actor, size, out);
}

int nflstreet_player_bound_create(uint32_t allocation_owner,
    nflstreet_factory_allocator allocate_object, void *object_context,
    const nflstreet_player_constructor_input *input,
    nflstreet_actor_model_allocator allocate, void *allocation_context,
    const nflstreet_player_appearance_context *appearance,
    const nflstreet_player_model_binding_context *binding,
    nflstreet_factory_memory *result)
{
    if (!result) return 0;
    *result = (nflstreet_factory_memory){0};
    if (!appearance || !appearance->service || !binding) return 0;
    bound_services services = {appearance, binding};
    nflstreet_player_appearance_context connected = *appearance;
    connected.service = appearance_service;
    connected.service_context = &services;
    return nflstreet_player_create(allocation_owner, allocate_object,
        object_context, input, allocate, allocation_context, &connected, result);
}
