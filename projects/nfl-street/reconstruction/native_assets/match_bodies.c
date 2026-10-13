#include "match_bodies.h"
#include <stdlib.h>
#include <string.h>

typedef struct arena_block {
    struct arena_block *next;
    uint8_t *bytes;
    uint32_t token;
} arena_block;

typedef struct {
    arena_block *blocks;
    uint32_t base, capacity;
    uint64_t used;
    unsigned allocations, releases;
    int failed;
} body_arena;

static int overlaps(uint32_t a, size_t as, uint32_t b, size_t bs)
{
    return (uint64_t)a < (uint64_t)b + bs &&
           (uint64_t)b < (uint64_t)a + as;
}

static int contained(uint32_t base, uint32_t capacity, uint32_t token, size_t size)
{
    uint64_t end = (uint64_t)base + capacity;
    return size && token >= base && (uint64_t)token + size <= end;
}

static int arena_allocate(void *opaque, uint32_t count, uint32_t width,
    nflstreet_model_postattach_segment *out)
{
    body_arena *arena = (body_arena *)opaque;
    if (!arena || !out || arena->failed || (count && width > SIZE_MAX / count)) {
        if (arena) arena->failed = 1;
        return 0;
    }
    uint64_t bytes = (uint64_t)count * width;
    uint64_t absolute = (uint64_t)arena->base + arena->used;
    uint64_t aligned = (absolute + 31u) & ~UINT64_C(31);
    uint64_t offset = aligned - arena->base;
    uint64_t rounded = ((bytes ? bytes : 1u) + 31u) & ~UINT64_C(31);
    if (aligned > UINT32_MAX || bytes > UINT32_MAX || offset > arena->capacity ||
        rounded > arena->capacity - offset || aligned + bytes > UINT32_MAX) {
        arena->failed = 1;
        return 0;
    }
    arena_block *block = malloc(sizeof *block);
    if (!block) {
        arena->failed = 1;
        return 0;
    }
    block->bytes = malloc(bytes ? (size_t)bytes : 1u);
    if (!block->bytes) {
        free(block);
        arena->failed = 1;
        return 0;
    }
    block->token = (uint32_t)aligned;
    block->next = arena->blocks;
    arena->blocks = block;
    arena->used = offset + rounded;
    ++arena->allocations;
    *out = (nflstreet_model_postattach_segment){block->token, block->bytes, (size_t)bytes};
    return 1;
}

static void arena_release(void *opaque, uint32_t token)
{
    body_arena *arena = (body_arena *)opaque;
    if (!arena) return;
    arena_block **link = &arena->blocks;
    while (*link) {
        arena_block *block = *link;
        if (block->token == token) {
            *link = block->next;
            free(block->bytes);
            free(block);
            ++arena->releases;
            return;
        }
        link = &block->next;
    }
}

static void arena_clear(body_arena *arena)
{
    while (arena->blocks) arena_release(arena, arena->blocks->token);
}

static int namespace_valid(const nflstreet_match_body_input *input)
{
    return input->namespace_base && input->namespace_size &&
        (uint64_t)input->namespace_base + input->namespace_size <= UINT32_MAX &&
        contained(input->namespace_base, input->namespace_size, input->model_token, 1) &&
        contained(input->namespace_base, input->namespace_size, input->view_token, 52) &&
        contained(input->namespace_base, input->namespace_size,
            input->temporary_token, input->temporary_capacity) &&
        input->target_index < 6 &&
        !overlaps(input->view_token, 52, input->temporary_token, input->temporary_capacity);
}

static int namespace_hits_visual(const nflstreet_match_body_input *input,
    const nflstreet_match_visuals *visuals)
{
    for (unsigned p = 0; p < NFLSTREET_MATCH_PLAYERS; ++p) {
        const nflstreet_selected_player_visual *visual = visuals->players + p;
        const nflstreet_selected_player_resources *base = &visual->geometry.base;
        if (!base->body || !base->body->bytes || !base->body->size ||
            overlaps(input->namespace_base, input->namespace_size,
                base->body_token, base->body->size)) return 1;
        for (unsigned i = 0; i < 2; ++i)
            if (!base->face.bytes[i] || !base->face.sizes[i] ||
                overlaps(input->namespace_base, input->namespace_size,
                    base->face.tokens[i], base->face.sizes[i])) return 1;
        for (unsigned i = 0; i < 3; ++i)
            if (visual->geometry.hair.bytes[i] &&
                overlaps(input->namespace_base, input->namespace_size,
                    visual->geometry.hair.tokens[i], visual->geometry.hair.sizes[i])) return 1;
        if (visual->surface.textures.block_size &&
            overlaps(input->namespace_base, input->namespace_size,
                visual->surface.textures.token, visual->surface.textures.block_size)) return 1;
        for (unsigned i = 0; i < visual->surface.allocation_count; ++i) {
            const nflstreet_material_block *block = visual->surface.allocations + i;
            size_t span = ((size_t)block->size + 31u) & ~(size_t)31u;
            if (span && overlaps(input->namespace_base, input->namespace_size,
                    block->token, span)) return 1;
        }
    }
    return 0;
}

void nflstreet_match_bodies_close(nflstreet_match_bodies *bodies)
{
    if (!bodies) return;
    for (unsigned p = 0; p < NFLSTREET_MATCH_PLAYERS; ++p)
        nflstreet_owned_body_close(bodies->players + p);
    memset(bodies, 0, sizeof *bodies);
}

int nflstreet_match_bodies_bind(nflstreet_match_visuals *visuals,
    const nflstreet_match_skeletons *skeletons, const uint8_t *raw_plamodel,
    size_t raw_size, const nflstreet_match_body_input *inputs,
    nflstreet_match_bodies *out)
{
    if (!visuals || visuals->count != NFLSTREET_MATCH_PLAYERS || !skeletons ||
        skeletons->count != NFLSTREET_MATCH_PLAYERS || !raw_plamodel || !inputs ||
        !out || out->count || out->temporary_allocations || out->temporary_releases) return 0;
    for (unsigned p = 0; p < NFLSTREET_MATCH_PLAYERS; ++p) {
        const nflstreet_match_body_input *input = inputs + p;
        if (out->players[p].file.bytes || out->players[p].model_token ||
            out->players[p].view_token || !namespace_valid(input) ||
            namespace_hits_visual(input, visuals)) return 0;
        for (unsigned q = 0; q < p; ++q)
            if (overlaps(input->namespace_base, input->namespace_size,
                    inputs[q].namespace_base, inputs[q].namespace_size)) return 0;
        for (unsigned q = 0; q < NFLSTREET_MATCH_PLAYERS; ++q) {
            const nflstreet_skeleton_startup *startup = skeletons->players + q;
            for (unsigned k = 0; k < 6; ++k) {
                const nflstreet_skeleton_resource *resource = startup->resources + k;
                if (!resource->bytes || !resource->size || !resource->token ||
                    overlaps(input->namespace_base, input->namespace_size,
                        resource->token, resource->size)) return 0;
            }
            if (!skeletons->face_view_tokens[q] || !skeletons->hair_view_tokens[q] ||
                overlaps(input->namespace_base, input->namespace_size,
                    skeletons->face_view_tokens[q], 128) ||
                overlaps(input->namespace_base, input->namespace_size,
                    skeletons->hair_view_tokens[q], 192)) return 0;
        }
    }

    nflstreet_match_bodies staged = {0};
    for (unsigned p = 0; p < NFLSTREET_MATCH_PLAYERS; ++p) {
        const nflstreet_match_body_input *input = inputs + p;
        const nflstreet_selected_player_visual *visual = visuals->players + p;
        const nflstreet_skeleton_resource *target =
            skeletons->players[p].resources + input->target_index;
        body_arena arena = {0};
        arena.base = input->temporary_token;
        arena.capacity = input->temporary_capacity;
        if (!nflstreet_owned_body_open(raw_plamodel, raw_size,
                visual->geometry.base.manifest.tbyp,
                visual->geometry.base.body_variant, input->model_token,
                input->view_token, target, arena_allocate, arena_release, &arena,
                staged.players + p)) {
            arena_clear(&arena);
            staged.temporary_allocations += arena.allocations;
            staged.temporary_releases += arena.releases;
            goto fail;
        }
        if (!contained(input->namespace_base, input->namespace_size,
                staged.players[p].model_token, staged.players[p].file.size) ||
            overlaps(staged.players[p].model_token, staged.players[p].file.size,
                input->temporary_token, input->temporary_capacity) ||
            arena.failed || arena.blocks || arena.allocations != arena.releases) {
            arena_clear(&arena);
            staged.temporary_allocations += arena.allocations;
            staged.temporary_releases += arena.releases;
            goto fail;
        }
        arena_clear(&arena);
        staged.temporary_allocations += arena.allocations;
        staged.temporary_releases += arena.releases;
        if (arena.blocks || arena.allocations != arena.releases) goto fail;
        ++staged.count;
    }

    *out = staged;
    for (unsigned p = 0; p < NFLSTREET_MATCH_PLAYERS; ++p) {
        nflstreet_selected_player_resources *base = &visuals->players[p].geometry.base;
        base->body = &out->players[p].file;
        base->body_view = out->players[p].view;
        base->body_token = out->players[p].model_token;
    }
    return 1;

fail:
    nflstreet_match_bodies_close(&staged);
    return 0;
}
