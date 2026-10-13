#include "body_postattach.h"
#include "model_relocate.h"
#include "model_view.h"
#include <string.h>

typedef struct {
    uint32_t token;
    size_t size;
    int active;
} tracked_range;

typedef struct {
    nflstreet_model_postattach_allocate allocate;
    nflstreet_model_postattach_release release;
    void *opaque;
    uint32_t model_token, view_token, target_token;
    size_t model_size, target_size;
    tracked_range allocations[3];
    size_t count;
    int failed;
} allocation_context;

static int range_end(uint32_t token, size_t size, uint64_t *end)
{
    if (!token || !size || size > UINT32_MAX ||
        (uint64_t)token + (uint64_t)size > UINT32_MAX) return 0;
    *end = (uint64_t)token + (uint64_t)size;
    return 1;
}

static int overlaps(uint32_t a, size_t as, uint32_t b, size_t bs)
{
    uint64_t ae = (uint64_t)a + as, be = (uint64_t)b + bs;
    return (uint64_t)a < be && (uint64_t)b < ae;
}

static int tracked_allocate(void *opaque, uint32_t count, uint32_t width,
    nflstreet_model_postattach_segment *out)
{
    allocation_context *ctx = (allocation_context *)opaque;
    if (!ctx || !out || ctx->failed || !ctx->allocate || ctx->count >= 3 ||
        (count && width > SIZE_MAX / count)) {
        if (ctx) ctx->failed = 1;
        return 0;
    }
    nflstreet_model_postattach_segment segment = {0};
    int ok = ctx->allocate(ctx->opaque, count, width, &segment);
    size_t required = (size_t)count * width;
    if (!ok || !segment.bytes || !segment.token || segment.size > UINT32_MAX ||
        (uint64_t)segment.token + (uint64_t)segment.size > UINT32_MAX ||
        segment.size < required ||
        overlaps(segment.token, segment.size, ctx->model_token, ctx->model_size) ||
        overlaps(segment.token, segment.size, ctx->view_token, 52) ||
        overlaps(segment.token, segment.size, ctx->target_token, ctx->target_size)) {
        ctx->failed = 1;
        if (ok && segment.token && ctx->release)
            ctx->release(ctx->opaque, segment.token);
        return 0;
    }
    for (size_t i = 0; i < ctx->count; ++i) {
        tracked_range *prior = ctx->allocations + i;
        if (prior->active && overlaps(segment.token, segment.size,
                prior->token, prior->size)) {
            ctx->failed = 1;
            if (ctx->release) ctx->release(ctx->opaque, segment.token);
            return 0;
        }
    }
    ctx->allocations[ctx->count++] = (tracked_range){segment.token, segment.size, 1};
    *out = segment;
    return 1;
}

static void tracked_release(void *opaque, uint32_t token)
{
    allocation_context *ctx = (allocation_context *)opaque;
    if (!ctx) return;
    if (ctx->release) ctx->release(ctx->opaque, token);
    for (size_t i = 0; i < ctx->count; ++i)
        if (ctx->allocations[i].active && ctx->allocations[i].token == token)
            ctx->allocations[i].active = 0;
}

void nflstreet_owned_body_close(nflstreet_owned_body *body)
{
    if (!body) return;
    nflstreet_team_body_binding_close(&body->file);
    memset(body, 0, sizeof *body);
}

int nflstreet_owned_body_open(const uint8_t *raw_plamodel, size_t size,
    uint32_t tbyp, uint32_t variant, uint32_t model_token, uint32_t view_token,
    const nflstreet_skeleton_resource *target,
    nflstreet_model_postattach_allocate allocate,
    nflstreet_model_postattach_release release, void *opaque,
    nflstreet_owned_body *out)
{
    if (!raw_plamodel || !target || !target->bytes || !out || !allocate ||
        !release || out->file.bytes) return 0;

    nflstreet_owned_body staged = {0};
    if (!nflstreet_team_body_binding_open(raw_plamodel, size, tbyp, variant,
            &staged.file)) return 0;
    uint64_t model_end, view_end, target_end;
    if (!range_end(model_token, staged.file.size, &model_end) ||
        !range_end(view_token, sizeof staged.view, &view_end) ||
        !range_end(target->token, target->size, &target_end) ||
        overlaps(model_token, staged.file.size, view_token, sizeof staged.view) ||
        overlaps(model_token, staged.file.size, target->token, target->size) ||
        overlaps(view_token, sizeof staged.view, target->token, target->size))
        goto fail;
    (void)model_end; (void)view_end; (void)target_end;

    size_t registration_count = 0;
    const nflstreet_model_chunk_registration *registry =
        nflstreet_model_default_registry(&registration_count);
    if (!nflstreet_model_relocate(staged.file.bytes, staged.file.size,
            model_token, registry, registration_count)) goto fail;
    nflstreet_model_view_source source = {
        staged.file.bytes, staged.file.size, model_token
    };
    if (!nflstreet_model_view_attach(staged.view, sizeof staged.view, &source))
        goto fail;

    staged.model_token = model_token;
    staged.view_token = view_token;
    nflstreet_model_postattach_segment segments[3] = {
        {model_token, staged.file.bytes, staged.file.size},
        {view_token, staged.view, sizeof staged.view},
        {target->token, target->bytes, target->size}
    };
    allocation_context ctx = {0};
    ctx.allocate = allocate;
    ctx.release = release;
    ctx.opaque = opaque;
    ctx.model_token = model_token;
    ctx.model_size = staged.file.size;
    ctx.view_token = view_token;
    ctx.target_token = target->token;
    ctx.target_size = target->size;
    if (!nflstreet_model_postattach_bind(view_token, target->token, segments, 3,
            tracked_allocate, tracked_release, &ctx) || ctx.failed) goto fail;
    *out = staged;
    return 1;

fail:
    nflstreet_owned_body_close(&staged);
    return 0;
}
