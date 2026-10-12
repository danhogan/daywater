#include "team_player_selected_pair.h"
#include "../native_assets/mesh_render.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint16_t be16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}
static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}
static int span(size_t size, size_t off, size_t n) {
    return off <= size && n <= size - off;
}

typedef struct {
    size_t offset, size;
} section;

static int find_section(const uint8_t *b, size_t size, const char tag[4],
                        section *out) {
    if (!b || size < 16 || memcmp(b, "TMdl", 4)) return 0;
    unsigned count = be16(b + 8);
    size_t directory = be16(b + 10);
    if (!span(size, directory, (size_t)count * 16)) return 0;
    section found = {0};
    unsigned matches = 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *row = b + directory + (size_t)i * 16;
        if (memcmp(row, tag, 4)) continue;
        found.offset = be32(row + 4);
        found.size = be32(row + 8);
        if (!span(size, found.offset, found.size)) return 0;
        ++matches;
    }
    if (matches != 1) return 0;
    *out = found;
    return 1;
}

static int stored_skeleton(const uint8_t *b, size_t size, section *skel,
                           section *clst, unsigned *bones, size_t *translations,
                           size_t *rotations, size_t *scales, size_t *rows,
                           unsigned *row_count) {
    if (!find_section(b, size, "Skel", skel) || skel->size < 24 ||
        !find_section(b, size, "Clst", clst) || clst->size < 8)
        return 0;
    const uint8_t *s = b + skel->offset, *c = b + clst->offset;
    *bones = be32(s);
    *translations = be32(s + 8);
    *rotations = be32(s + 12);
    *scales = be32(s + 16);
    *rows = be32(c);
    *row_count = be32(c + 4);
    return *bones && *bones <= 256 && *row_count && *row_count <= 256 &&
           span(size, *translations, (size_t)*bones * 16) &&
           span(size, *rotations, (size_t)*bones * 6) &&
           span(size, *scales, (size_t)*bones * 12) &&
           span(size, *rows, (size_t)*row_count * 4);
}

int nflstreet_team_player_selected_pair_has_shared_pose(
    const nflstreet_team_player_selected_pair *pair) {
    if (!pair || !pair->body_binding.bytes || !pair->head.owned_model_bytes)
        return 0;
    const uint8_t *body = pair->body_binding.bytes;
    const uint8_t *head = pair->head.owned_model_bytes;
    section bsk, bcl, hsk, hcl;
    unsigned bn, hn, brc, hrc;
    size_t bt, bro, bs, bcr, ht, hro, hs, hcr;
    if (!stored_skeleton(body, pair->body_binding.size, &bsk, &bcl, &bn,
                         &bt, &bro, &bs, &bcr, &brc) ||
        !stored_skeleton(head, pair->head.model_size, &hsk, &hcl, &hn,
                         &ht, &hro, &hs, &hcr, &hrc) || bn != hn)
        return 0;
    /* Root transform must match for direct root-local composition. */
    if (memcmp(body + bt, head + ht, 16) ||
        memcmp(body + bro, head + hro, 6) ||
        memcmp(body + bs, head + hs, 12))
        return 0;
    const uint8_t *br = body + bcr, *hr = head + hcr;
    for (unsigned i = 0; i < hrc; ++i) {
        unsigned bone = hr[i * 4], parent = hr[i * 4 + 1];
        if (bone >= bn) return 0;
        unsigned head_parent_bone = parent == 255 ? 255 :
            (parent < i ? hr[parent * 4] : 256);
        if (head_parent_bone > 255) return 0;
        int parent_match = 0;
        for (unsigned j = 0; j < brc; ++j) {
            unsigned body_parent = br[j * 4 + 1];
            unsigned body_parent_bone = body_parent == 255 ? 255 :
                (body_parent < j ? br[body_parent * 4] : 256);
            if (br[j * 4] == bone && body_parent_bone == head_parent_bone) {
                parent_match = 1;
                break;
            }
        }
        if (!parent_match ||
            memcmp(body + bt + (size_t)bone * 16,
                   head + ht + (size_t)bone * 16, 16) ||
            memcmp(body + bro + (size_t)bone * 6,
                   head + hro + (size_t)bone * 6, 6) ||
            memcmp(body + bs + (size_t)bone * 12,
                   head + hs + (size_t)bone * 12, 12))
            return 0;
    }
    return 1;
}

int nflstreet_team_player_selected_pair_open(
    const uint8_t *plamodel_bytes, size_t plamodel_size, uint32_t tbyp,
    const nflstreet_archive *face_archive, unsigned head_index,
    nflstreet_team_player_selected_pair *out) {
    if (!plamodel_bytes || !face_archive || !out) return 0;

    nflstreet_team_player_selected_pair next = {0};
    next.tbyp = tbyp;
    next.body_variant = 0; /* Explicit host inspection choice, not menu state. */
    if (!nflstreet_team_body_binding_open(plamodel_bytes, plamodel_size,
                                          tbyp, 0, &next.body_binding))
        return 0;
    if (!nflstreet_mesh_open_bind_pose(next.body_binding.bytes,
                                       next.body_binding.size,
                                       &next.body_mesh)) {
        nflstreet_team_body_binding_close(&next.body_binding);
        return 0;
    }
    if (!nflstreet_team_player_head_open(face_archive, head_index,
                                         &next.head)) {
        nflstreet_mesh_close(&next.body_mesh);
        nflstreet_team_body_binding_close(&next.body_binding);
        return 0;
    }
    *out = next;
    return 1;
}

void nflstreet_team_player_selected_pair_close(
    nflstreet_team_player_selected_pair *pair) {
    if (!pair) return;
    nflstreet_team_player_head_close(&pair->head);
    nflstreet_mesh_close(&pair->body_mesh);
    nflstreet_team_body_binding_close(&pair->body_binding);
    memset(pair, 0, sizeof *pair);
}

int nflstreet_team_player_selected_pair_render(
    const nflstreet_team_player_selected_pair *pair, unsigned panel_width,
    unsigned panel_height, float yaw, nflstreet_texture *out) {
    if (!pair || !out || !panel_width || panel_width > 2048 ||
        !panel_height || panel_height > 2048 || panel_width > 4096 / 2)
        return 0;

    nflstreet_texture body = {0}, head = {0};
    if (!nflstreet_mesh_render(&pair->body_mesh, panel_width, panel_height,
                               yaw, &body) ||
        !nflstreet_team_player_head_render(&pair->head, panel_width,
                                           panel_height, yaw, &head)) {
        nflstreet_texture_close(&body);
        nflstreet_texture_close(&head);
        return 0;
    }
    size_t pixels = (size_t)panel_width * panel_height;
    nflstreet_texture result = {panel_width * 2, panel_height, NULL};
    result.bgra = malloc(pixels * 2 * sizeof *result.bgra);
    if (!result.bgra) {
        nflstreet_texture_close(&body);
        nflstreet_texture_close(&head);
        return 0;
    }
    for (unsigned y = 0; y < panel_height; ++y) {
        size_t row = (size_t)y * panel_width;
        memcpy(result.bgra + row * 2, body.bgra + row,
               panel_width * sizeof *result.bgra);
        memcpy(result.bgra + row * 2 + panel_width, head.bgra + row,
               panel_width * sizeof *result.bgra);
    }
    nflstreet_texture_close(&body);
    nflstreet_texture_close(&head);
    *out = result;
    return 1;
}

int nflstreet_team_player_selected_pair_render_candidate(
    const nflstreet_team_player_selected_pair *pair, unsigned width,
    unsigned height, float yaw, nflstreet_texture *out) {
    if (!pair || !out || width == 0 || height == 0 || width > 4096 ||
        height > 4096 || !isfinite(yaw) ||
        !nflstreet_team_player_selected_pair_has_shared_pose(pair))
        return 0;
    const nflstreet_mesh *body = &pair->body_mesh, *head = &pair->head.mesh;
    if (!body->triangles || !body->count || !head->triangles || !head->count ||
        body->count > SIZE_MAX - head->count)
        return 0;
    unsigned body_materials = body->material_count;
    unsigned head_materials = head->material_count;
    unsigned body_base = 0, head_base = body_materials;
    unsigned material_count = body_materials + head_materials;
    int body_dummy = body_materials == 0 && head_materials != 0;
    int head_dummy = head_materials == 0 && body_materials != 0;
    if (body_dummy) { head_base = 1; ++material_count; }
    if (head_dummy) ++material_count;
    if (material_count > 8192 || body->count + head->count > 2000000) return 0;

    nflstreet_mesh combined = {0};
    combined.count = body->count + head->count;
    combined.draws = body->draws + head->draws;
    combined.material_count = material_count;
    combined.inspection_opaque = body->inspection_opaque && head->inspection_opaque;
    combined.triangles = malloc(combined.count * sizeof *combined.triangles);
    if (!combined.triangles) return 0;
    if (material_count) {
        combined.materials = calloc(material_count, sizeof *combined.materials);
        if (!combined.materials) { nflstreet_mesh_close(&combined); return 0; }
        if (body_materials)
            memcpy(combined.materials + body_base, body->materials,
                   (size_t)body_materials * sizeof *combined.materials);
        if (head_materials)
            memcpy(combined.materials + head_base, head->materials,
                   (size_t)head_materials * sizeof *combined.materials);
    }
    memcpy(combined.triangles, body->triangles,
           body->count * sizeof *combined.triangles);
    memcpy(combined.triangles + body->count, head->triangles,
           head->count * sizeof *combined.triangles);
    for (size_t i = 0; i < body->count; ++i) {
        nflstreet_mesh_triangle *t = combined.triangles + i;
        if (body_materials) t->material = (uint16_t)(t->material + body_base);
        else if (body_dummy) t->material = 0;
    }
    for (size_t i = body->count; i < combined.count; ++i) {
        nflstreet_mesh_triangle *t = combined.triangles + i;
        if (head_materials) t->material = (uint16_t)(t->material + head_base);
        else if (head_dummy) t->material = (uint16_t)(material_count - 1);
    }
    int first = 1;
    for (size_t i = 0; i < combined.count; ++i) {
        for (unsigned v = 0; v < 3; ++v) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                float value = combined.triangles[i].vertex[v].position[axis];
                if (!isfinite(value)) { nflstreet_mesh_close(&combined); return 0; }
                if (first) {
                    combined.minimum[axis] = combined.maximum[axis] = value;
                } else {
                    if (value < combined.minimum[axis]) combined.minimum[axis] = value;
                    if (value > combined.maximum[axis]) combined.maximum[axis] = value;
                }
            }
            first = 0;
        }
    }
    memcpy(combined.name, "stored-pose-candidate", sizeof "stored-pose-candidate");
    int ok = nflstreet_mesh_render_view_y_up(&combined, width, height, yaw,
                                            0.1f, 1.1f, out);
    nflstreet_mesh_close(&combined);
    return ok;
}

int nflstreet_team_player_selected_pair_render_combined(
    const nflstreet_team_player_selected_pair *pair, unsigned width,
    unsigned height, float yaw, nflstreet_texture *out) {
    return nflstreet_team_player_selected_pair_render_candidate(
        pair, width, height, yaw, out);
}
