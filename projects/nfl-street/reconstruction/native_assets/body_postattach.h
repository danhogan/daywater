#ifndef NFLSTREET_BODY_POSTATTACH_H
#define NFLSTREET_BODY_POSTATTACH_H
#include "../menus/team_player_body_binding.h"
#include "model_postattach.h"
#include "skeleton_resource.h"

typedef struct {
    nflstreet_team_body_binding file;
    uint8_t view[52];
    uint32_t model_token, view_token;
} nflstreet_owned_body;

/* Own one explicitly selected decoded team body, relocated and attached at
   caller identities, then bind its private file/view to a fresh skeleton.
   Token ranges for model, view, target, and allocator storage must be disjoint.
   Failure leaves out unchanged and releases the private decoded body. */
int nflstreet_owned_body_open(const uint8_t *raw_plamodel, size_t size,
    uint32_t tbyp, uint32_t variant, uint32_t model_token, uint32_t view_token,
    const nflstreet_skeleton_resource *target,
    nflstreet_model_postattach_allocate allocate,
    nflstreet_model_postattach_release release, void *opaque,
    nflstreet_owned_body *out);
void nflstreet_owned_body_close(nflstreet_owned_body *body);
#endif
