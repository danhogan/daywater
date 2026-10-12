#include "body_resource_pool.h"
#include <string.h>

void nflstreet_body_resource_pool_close(nflstreet_body_resource_pool *pool)
{
    if (!pool) return;
    for (unsigned i = 0; i < 9; ++i)
        nflstreet_team_body_binding_close(pool->assets + i);
    memset(pool, 0, sizeof *pool);
}

int nflstreet_body_resource_pool_open(const uint8_t *bytes, size_t size,
    uint32_t identity_base, nflstreet_body_resource_pool *out)
{
    if (!bytes || !out || !identity_base) return 0;
    nflstreet_body_resource_pool pool = {0};
    uint64_t cursor = identity_base;
    for (unsigned i = 0; i < 9; ++i) {
        nflstreet_team_body_binding *asset = pool.assets + i;
        if (!nflstreet_team_body_binding_open(bytes, size, i / 3, i % 3, asset))
            goto fail;
        if (cursor > UINT32_MAX || asset->size > UINT32_MAX - cursor) goto fail;
        pool.tokens[i] = (uint32_t)cursor;
        nflstreet_model_view_source source = {asset->bytes, asset->size, pool.tokens[i]};
        if (!nflstreet_model_view_attach(pool.views[i], sizeof pool.views[i], &source))
            goto fail;
        cursor += ((uint64_t)asset->size + 31u) & ~(uint64_t)31u;
    }
    *out = pool;
    return 1;
fail:
    nflstreet_body_resource_pool_close(&pool);
    return 0;
}

int nflstreet_body_resource_pool_get(const nflstreet_body_resource_pool *pool,
    uint32_t tbyp, uint32_t variant, const nflstreet_team_body_binding **asset,
    const uint8_t **view)
{
    uint32_t key = tbyp == 7 ? 2 : tbyp;
    if (!pool || !asset || !view || key > 2 || variant > 2) return 0;
    unsigned i = key * 3 + variant;
    if (!pool->assets[i].bytes || !pool->tokens[i]) return 0;
    *asset = pool->assets + i;
    *view = pool->views[i];
    return 1;
}

int nflstreet_body_resource_pool_process(nflstreet_body_resource_pool *pool)
{
    if(!pool)return 0;
    size_t count=0;
    const nflstreet_model_chunk_registration *registry=nflstreet_model_default_registry(&count);
    for(unsigned i=0;i<9;++i){
        nflstreet_team_body_binding *asset=pool->assets+i;
        if(!asset->bytes||!pool->tokens[i]||
           !nflstreet_model_relocate(asset->bytes,asset->size,pool->tokens[i],registry,count))return 0;
        nflstreet_model_view_source source={asset->bytes,asset->size,pool->tokens[i]};
        if(!nflstreet_model_view_attach(pool->views[i],sizeof pool->views[i],&source))return 0;
    }
    return 1;
}
