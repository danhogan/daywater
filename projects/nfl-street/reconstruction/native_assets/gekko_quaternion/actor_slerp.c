#include "actor_slerp.h"
#include "quaternion_kernels.h"
#include "../cluster_palette.h"

#include <string.h>

#define LOCKED 0xe0000000u
typedef nflstreet_actor_palette_context context;

static uint8_t *memory(context *c, uint32_t address, uint32_t bytes)
{
    if (address >= LOCKED)
        return (uint64_t)address - LOCKED + bytes <= sizeof c->locked ?
            c->locked + (address - LOCKED) : NULL;
    return address ? c->x->span(c->x->memory, address, bytes) : NULL;
}

static int read_word(context *c, uint32_t address, uint32_t *value)
{
    const uint8_t *p = memory(c, address, 4);
    if (!p) return 0;
    *value = nflstreet_rd32(p);
    return 1;
}

static float read_float(const uint8_t *p)
{
    return nflstreet_as_float(nflstreet_rd32(p));
}

static int16_t read_angle(const uint8_t *p)
{
    const uint16_t bits = (uint16_t)((unsigned)p[0] << 8 | p[1]);
    int16_t result;
    memcpy(&result, &bits, sizeof result);
    return result;
}

static void store_matrix(uint8_t *out, const float matrix[12])
{
    for (unsigned i = 0; i < 12; ++i)
        nflstreet_wr32(out + 4 * i, nflstreet_float_bits(matrix[i]));
}

/* 801F0DD4 transfers the final partial cache line as a whole 32-byte block. */
static int copy_palette(context *c, uint32_t out, uint32_t scratch, unsigned count)
{
    if (!scratch) return 1; /* 801F0BD0's flush has no RAM effect. */
    const uint32_t bytes = (count * 48u + 31u) & ~31u;
    uint8_t *to = memory(c, out, bytes);
    const uint8_t *from = memory(c, scratch, bytes);
    if (!to || !from) return 0;
    memmove(to, from, bytes);
    return 1;
}

int nflstreet_801ead98_slerp_body(context *c, nflstreet_draw *draw,
    uint32_t model, uint32_t descriptor, uint32_t pose, uint32_t enables)
{
    uint32_t skeleton, clusters, scratch, count, bones, translations, rotations;
    uint32_t rows_address, top, index, pool;
    if (!c || !c->x || !c->x->span || !draw) return 0;
    if (!model) return 1;
    if (!read_word(c, model + 12, &clusters) ||
        !read_word(c, model + 16, &skeleton)) return 0;
    if (!clusters || !skeleton) return 1;
    if (!read_word(c, descriptor + 4, &scratch)) return 0;
    if (scratch) {
        uint8_t *on = memory(c, c->cache_on, 1);
        if (!on) return 0;
        if (!*on) *on = 1; /* 801EDE38 */
    }
    if (!read_word(c, clusters + 4, &count) ||
        !read_word(c, skeleton, &bones) ||
        !read_word(c, skeleton + 8, &translations) ||
        !read_word(c, skeleton + 12, &rotations) ||
        !read_word(c, clusters, &rows_address) ||
        !nflstreet_matrix_top(draw, &c->stacks, &top) ||
        !read_word(c, c->pool_index, &index) ||
        !read_word(c, c->pool + 4 * index, &pool)) return 0;
    if (!bones || bones > 256 || !count || count > 256) return 0;
    uint8_t *desc = memory(c, descriptor, 12);
    uint8_t *used = memory(c, c->pool_used, 4);
    if (!desc || !used) return 0;
    const uint32_t out = pool + nflstreet_rd32(used) * 48u;
    const uint32_t allocated = (count + 1u) & ~1u;
    nflstreet_wr32(used, nflstreet_rd32(used) + allocated);
    nflstreet_wr32(desc, out);
    nflstreet_wr32(desc + 8, allocated); /* 801CA4B0 */

    const uint32_t base = scratch ? scratch : out;
    uint8_t *destination = memory(c, base, count * 48u + 16u);
    const uint8_t *root_words = memory(c, top, 64);
    const uint8_t *tr = memory(c, translations, bones * 16u);
    const uint8_t *rest = memory(c, rotations, bones * 6u);
    const uint8_t *rows = memory(c, rows_address, count * 4u);
    const uint8_t *compact = pose ? memory(c, pose, bones * 6u) : NULL;
    const uint8_t *poly = memory(c, c->stacks.sine, 32);
    const uint8_t *enabled = pose ? memory(c, enables, bones) : NULL;
    if (!destination || !root_words || !tr || !rest || !rows || !poly ||
        (pose && (!compact || !enabled))) return 0;
    float root[12], coefficients[8], matrices[256][12];
    int16_t current_angles[256 * 3];
    for (unsigned i = 0; i < 12; ++i) root[i] = read_float(root_words + 4 * i);
    for (unsigned i = 0; i < 8; ++i) coefficients[i] = read_float(poly + 4 * i);
    if (compact) for (unsigned i = 0; i < bones * 3u; ++i)
        current_angles[i] = read_angle(compact + 2 * i);
    /* 801CA900 calls the complete 801CA4E8 first, including its first DMA. */
    if (!nflstreet_cluster_palette_rows(tr, rest, rows, bones, count,
            compact ? current_angles : NULL, root, coefficients, matrices)) return 0;
    memmove(destination, root_words, 64);
    for (unsigned i = 1; i < count; ++i)
        store_matrix(destination + 48u * i, matrices[i]);
    if (!copy_palette(c, out, scratch, count)) return 0;

    for (unsigned i = 1; i < count; ++i) {
        const uint8_t *row = rows + 4 * i;
        const unsigned bone = row[0], parent = row[1];
        if (bone >= bones || parent >= i) return 0;
        if (!compact || !enabled[bone]) continue;
        float a[4], b[4], blended[4], translation[3], local[12], result[12];
        int32_t angles[3];
        /* Register arguments r4/r5/r6 are compact Z/Y/X, each shifted by
           eight. Multiplication avoids C's undefined negative left shift. */
        nflstreet_801a36ec_quaternion(current_angles[3 * bone + 2] * 256,
            current_angles[3 * bone + 1] * 256,
            current_angles[3 * bone] * 256, coefficients, a);
        nflstreet_801a36ec_quaternion(read_angle(rest + 6 * bone + 4) * 256,
            read_angle(rest + 6 * bone + 2) * 256,
            read_angle(rest + 6 * bone) * 256, coefficients, b);
        const unsigned weight = (unsigned)row[2] << 8 | row[3];
        const float t = (float)weight * 0x1p-12f;
        nflstreet_801a383c_slerp(t, a, b, blended);
        nflstreet_801a340c_euler(blended, angles);
        for (unsigned axis = 0; axis < 3; ++axis) {
            angles[axis] >>= 8; /* 801CAA74/7C/80: srawi */
            translation[axis] = read_float(tr + 16 * bone + 4 * axis);
        }
        nflstreet_801ca900_local_matrix(angles, coefficients, translation, local);
        float parent_matrix[12];
        for (unsigned n = 0; n < 12; ++n)
            parent_matrix[n] = read_float(destination + 48u * parent + 4 * n);
        nflstreet_801ffdf0_concat(parent_matrix, local, result);
        store_matrix(destination + 48u * i, result);
    }
    return copy_palette(c, out, scratch, count);
}
