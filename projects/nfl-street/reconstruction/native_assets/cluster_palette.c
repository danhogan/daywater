#include "cluster_palette.h"
#include <math.h>
#include <string.h>

static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}
static unsigned u16(const uint8_t *p) { return (unsigned)p[0] << 8 | p[1]; }
static float word_float(uint32_t w) { float f; memcpy(&f, &w, 4); return f; }
static int span(size_t total, size_t off, size_t n) {
    return off <= total && n <= total - off;
}
static int16_t signed_half(unsigned v) {
    uint16_t bits = (uint16_t)v; int16_t value;
    memcpy(&value, &bits, sizeof value); return value;
}
static float sine_poly(int16_t angle, const float p[6]) {
    float x = angle * (1.0f / 32768.0f), x2 = x * x;
    float x3 = x * x2, x4 = x2 * x2, x5 = x * x4;
    float x7 = x3 * x4, x9 = x5 * x4, x11 = x7 * x4;
    float a = p[0] * x, b = p[1] * x3;
    a = fmaf(p[2], x5, a); b = fmaf(p[3], x7, b);
    a = fmaf(p[4], x9, a); b = fmaf(p[5], x11, b);
    return a + b;
}
static int32_t weighted_delta(int16_t pose, int16_t base, unsigned weight) {
    /* The game multiplies the signed difference by an unsigned weight in a
       wrapping 32-bit register, then performs an arithmetic shift by 12. */
    int32_t difference = (int32_t)pose - (int32_t)base;
    uint32_t product_bits = (uint32_t)difference * (uint32_t)weight;
    int32_t product; memcpy(&product, &product_bits, sizeof product);
    if (product >= 0) return product / 4096;
    return -(int32_t)(((uint32_t)(-(int64_t)product) + 4095u) / 4096u);
}
static void concat(const float *a, const float *b, float *out) {
    for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c) {
        float v = a[r * 4] * b[c];
        v = fmaf(a[r * 4 + 1], b[4 + c], v);
        v = fmaf(a[r * 4 + 2], b[8 + c], v);
        if (c >= 2) v = fmaf(c == 3 ? 1.0f : 0.0f, a[r * 4 + 3], v);
        out[r * 4 + c] = v;
    }
}

int nflstreet_cluster_palette(const nflstreet_model_view_source *source,
    const uint8_t view[NFLSTREET_MODEL_VIEW_BYTES], const int16_t *pose,
    size_t pose_count, const float root[12], const float polynomial[6],
    float out[256][12], unsigned *count) {
    if (!source || !source->bytes || source->size < 16 || !view || !root ||
        !polynomial || !out || !count || source->bytes[15] != 1) return 0;
    nflstreet_model_view_chunk skel_chunk, clst_chunk;
    int skel_found = nflstreet_model_view_find(source, 0x536b656cu, &skel_chunk);
    int clst_found = nflstreet_model_view_find(source, 0x436c7374u, &clst_chunk);
    if (skel_found != 1 || clst_found != 1 ||
        skel_chunk.encoded_word != u32(view + 0x10) ||
        clst_chunk.encoded_word != u32(view + 0x0c)) return 0;

    size_t skel_off = (size_t)(skel_chunk.bytes - source->bytes);
    size_t clst_off = (size_t)(clst_chunk.bytes - source->bytes);
    if (!span(source->size, skel_off, 16) || !span(source->size, clst_off, 8)) return 0;
    const uint8_t *skel = source->bytes + skel_off;
    const uint8_t *clst = source->bytes + clst_off;
    unsigned bones = u32(skel), clusters = u32(clst + 4);
    uint32_t translations_token = u32(skel + 8);
    uint32_t rotations_token = u32(skel + 12);
    uint32_t rows_token = u32(clst);
    if (translations_token < source->token || rotations_token < source->token ||
        rows_token < source->token) return 0;
    size_t translations = (size_t)(translations_token - source->token);
    size_t rotations = (size_t)(rotations_token - source->token);
    size_t rows = (size_t)(rows_token - source->token);
    if (!bones || bones > 256 || !clusters || clusters > 256 ||
        !span(source->size, translations, (size_t)bones * 16) ||
        !span(source->size, rotations, (size_t)bones * 6) ||
        !span(source->size, rows, (size_t)clusters * 4) ||
        (pose && pose_count < (size_t)bones * 3)) return 0;
    if (!nflstreet_cluster_palette_rows(source->bytes + translations,
            source->bytes + rotations, source->bytes + rows, bones, clusters,
            pose, root, polynomial, out)) return 0;
    *count = clusters;
    return 1;
}

int nflstreet_cluster_palette_rows(const uint8_t *translations,
    const uint8_t *rotations, const uint8_t *rows, unsigned bones,
    unsigned clusters, const int16_t *pose, const float root[12],
    const float polynomial[6], float out[][12]) {
    if (!translations || !rotations || !rows || !root || !polynomial || !out ||
        !bones || bones > 256 || !clusters || clusters > 256) return 0;
    for (unsigned i = 0; i < 12; ++i) if (!isfinite(root[i])) return 0;
    for (unsigned i = 0; i < 6; ++i) if (!isfinite(polynomial[i])) return 0;

    float result[256][12];
    memcpy(result[0], root, sizeof result[0]);
    for (unsigned i = 1; i < clusters; ++i) {
        const uint8_t *row = rows + (size_t)i * 4;
        unsigned bone = row[0], parent = row[1], weight = u16(row + 2);
        if (bone >= bones || parent >= i) return 0;
        const uint8_t *rot = rotations + (size_t)bone * 6;
        float s[3], c[3];
        for (unsigned axis = 0; axis < 3; ++axis) {
            int16_t angle = signed_half(u16(rot + axis * 2));
            if (pose) {
                int16_t base = angle;
                uint16_t low = (uint16_t)((uint32_t)(int32_t)base +
                    (uint32_t)weighted_delta(pose[bone * 3 + axis], base, weight));
                angle = signed_half(low);
            }
            s[axis] = sine_poly(angle, polynomial);
            angle = signed_half((uint16_t)((uint16_t)angle + 16384u));
            c[axis] = sine_poly(angle, polynomial);
        }
        float sx=s[0], sy=s[1], sz=s[2], cx=c[0], cy=c[1], cz=c[2];
        float a=sx*sy, d=cx*sy;
        float local[12] = {
            cz*cy, fmaf(-sz,cx,cz*a), fmaf(sz,sx,cz*d), 0.0f,
            sz*cy, fmaf(cz,cx,sz*a), fmaf(-cz,sx,sz*d), 0.0f,
            -sy, cy*sx, cy*cx, 0.0f
        };
        const uint8_t *translation = translations + (size_t)bone * 16;
        for (unsigned axis=0; axis<3; ++axis) {
            local[axis*4+3] = word_float(u32(translation + axis*4));
            if (!isfinite(local[axis*4+3])) return 0;
        }
        concat(result[parent], local, result[i]);
        for (unsigned j=0; j<12; ++j) if (!isfinite(result[i][j])) return 0;
    }
    memcpy(out, result, (size_t)clusters * sizeof result[0]);
    return 1;
}
