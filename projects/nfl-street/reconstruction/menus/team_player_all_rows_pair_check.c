/* Private all-PLAY resource/geometry coverage using the checked native HSH1
   resolver and TBYP body binding. No proprietary rows or payloads are saved
   to the repository. */
#include "team_player_resolve.h"
#include "team_player_body_binding.h"
#include "team_player_selected_pair.h"
#include "team_catalog.h"
#include "../native_assets/model.h"
#include "../native_assets/texture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define make_dir(p) _mkdir(p)
#else
#include <sys/stat.h>
#define make_dir(p) mkdir((p), 0777)
#endif

#define PLAY_TAG 0x59414c50u
#define DIGT_TAG 0x44494754u
#define DIGP_TAG 0x44494750u

static int read_file(const char *path, uint8_t **out, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long n = ftell(f);
    if (n <= 0 || n > 128 * 1024 * 1024 || fseek(f, 0, SEEK_SET)) {
        fclose(f); return 0;
    }
    uint8_t *p = malloc((size_t)n);
    if (!p || fread(p, 1, (size_t)n, f) != (size_t)n) {
        free(p); fclose(f); return 0;
    }
    fclose(f); *out = p; *size = (size_t)n; return 1;
}

static int field_index(const nflstreet_team_table *t, uint32_t tag,
                       unsigned *index) {
    unsigned found = 0;
    for (unsigned i = 0; i < t->field_count; ++i) {
        nflstreet_team_field f;
        if (!nflstreet_team_table_field(t, i, &f)) return 0;
        if (f.tag == tag) { *index = i; ++found; }
    }
    return found == 1;
}

static void row_log(FILE *f, const nflstreet_team_player_manifest *m,
                    unsigned body_key, const char *body_name,
                    const char *head_name, size_t body_tris,
                    size_t head_tris, int pose, const char *status) {
    fprintf(f, "%u\t%u\t%u\t%u\t%u\t%d\t%s\t%s\t%zu\t%zu\t%d\t%s\n",
            m->team_id, m->player_id, m->tbyp, body_key,
            m->body_key_supported, m->head_runtime_index,
            body_name ? body_name : "", head_name ? head_name : "",
            body_tris, head_tris, pose, status);
}

int main(int argc, char **argv) {
    if (argc != 7) {
        fprintf(stderr, "usage: all-rows-pair-check <db-teams-dir> <plamodel.dat> <plaface.dat> <plarosterhair.dat> <private-report.tsv> <private-render-prefix>\n");
        return 2;
    }
    uint8_t *plamodel = NULL, *face = NULL, *hair = NULL;
    size_t plamodel_n = 0, face_n = 0, hair_n = 0;
    nflstreet_team_player_resolver resolver;
    if (!read_file(argv[2], &plamodel, &plamodel_n) ||
        !read_file(argv[3], &face, &face_n) ||
        !read_file(argv[4], &hair, &hair_n) ||
        !nflstreet_team_player_resolver_open(face, face_n, hair, hair_n,
                                             &resolver)) {
        fprintf(stderr, "archive/resolver setup failed\n");
        free(plamodel); free(face); free(hair); return 3;
    }
    FILE *report = fopen(argv[5], "wb");
    if (!report) { free(plamodel); free(face); free(hair); return 4; }
    fprintf(report, "team_id\tdigp\ttbyp\tbody_key\tbody_key_supported\thead_index\tbody_info\thead_info\tbody_triangles\thead_triangles\tshared_pose\tstatus\n");

    unsigned rows = 0, body_ok = 0, pair_ok = 0, pose_ok = 0;
    unsigned unresolved_head = 0, unsupported_body = 0, body_geometry_fail = 0;
    unsigned head_geometry_fail = 0, pose_mismatch = 0, manifest_fail = 0;
    unsigned body_keys[3] = {0, 0, 0};
    unsigned representative[3] = {0, 0, 0};
    for (unsigned dbid = 1; dbid <= 32; ++dbid) {
        char path[4096];
        int n = snprintf(path, sizeof path, "%s\\%05u.bin", argv[1], dbid);
        uint8_t *db = NULL; size_t db_n = 0;
        if (n < 0 || (size_t)n >= sizeof path || !read_file(path, &db, &db_n)) {
            fprintf(stderr, "team DB read failed: db=%u\n", dbid);
            fclose(report); free(plamodel); free(face); free(hair); return 5;
        }
        nflstreet_team_table table;
        if (!nflstreet_team_table_open_tag(db, db_n, PLAY_TAG, &table)) {
            fprintf(stderr, "PLAY table parse failed: db=%u\n", dbid);
            free(db); fclose(report); free(plamodel); free(face); free(hair); return 6;
        }
        unsigned team_ix, player_ix;
        if (!field_index(&table, DIGT_TAG, &team_ix) ||
            !field_index(&table, DIGP_TAG, &player_ix)) {
            fprintf(stderr, "PLAY identity fields missing: db=%u\n", dbid);
            free(db); fclose(report); free(plamodel); free(face); free(hair); return 7;
        }
        for (unsigned row = 0; row < table.used; ++row) {
            uint32_t team = 0, player = 0;
            if (!nflstreet_team_table_number(&table, row, team_ix, &team) ||
                !nflstreet_team_table_number(&table, row, player_ix, &player)) {
                free(db); fclose(report); free(plamodel); free(face); free(hair); return 8;
            }
            /* The regular-team DB blocks contain rows assigned to multiple
               DIGT values. Catalog all nonzero-team PLAY rows, matching the
               402-row source enumerator; do not assume DIGT equals file ID. */
            if (team == 0) continue;
            ++rows;
            nflstreet_team_player_manifest m = {0};
            if (!nflstreet_team_player_resolve(&resolver, db, db_n, team,
                                               player, &m) || m.play_row != row) {
                ++manifest_fail;
                nflstreet_team_player_manifest failed = {.team_id=team,.player_id=player,.play_row=row};
                row_log(report, &failed, 0, NULL, NULL, 0, 0, 0, "resolver_failure");
                continue;
            }
            if (!m.body_key_supported || m.body_key > 2) {
                ++unsupported_body;
                row_log(report, &m, m.body_key, NULL, NULL, 0, 0, 0, "unsupported_tbyp");
                continue;
            }
            ++body_keys[m.body_key];
            nflstreet_team_body_binding binding = {0};
            nflstreet_mesh body_mesh = {0};
            if (!nflstreet_team_body_binding_open(plamodel, plamodel_n, m.tbyp,
                                                  0, &binding) ||
                binding.pool_key != m.body_key ||
                !nflstreet_mesh_open_bind_pose(binding.bytes, binding.size,
                                               &body_mesh)) {
                ++body_geometry_fail;
                row_log(report, &m, m.body_key,
                        binding.info_name[0] ? binding.info_name : NULL,
                        NULL, 0, 0, 0, "body_geometry_unsupported");
                nflstreet_mesh_close(&body_mesh);
                nflstreet_team_body_binding_close(&binding);
                continue;
            }
            ++body_ok;
            if (!m.custom_name_branch || m.head_runtime_index < 0) {
                ++unresolved_head;
                row_log(report, &m, m.body_key, binding.info_name, NULL,
                        body_mesh.count, 0, 0, "head_branch_unresolved");
                nflstreet_mesh_close(&body_mesh);
                nflstreet_team_body_binding_close(&binding);
                continue;
            }

            nflstreet_team_player_selected_pair pair = {0};
            if (!nflstreet_team_player_selected_pair_open(
                    plamodel, plamodel_n, m.tbyp, &resolver.face,
                    (unsigned)m.head_runtime_index, &pair) ||
                pair.body_binding.archive_index != binding.archive_index ||
                strcmp(pair.body_binding.info_name, binding.info_name)) {
                ++head_geometry_fail;
                row_log(report, &m, m.body_key, binding.info_name, NULL,
                        body_mesh.count, 0, 0, "head_geometry_unsupported");
                nflstreet_team_player_selected_pair_close(&pair);
                nflstreet_mesh_close(&body_mesh);
                nflstreet_team_body_binding_close(&binding);
                continue;
            }
            ++pair_ok;
            int pose = nflstreet_team_player_selected_pair_has_shared_pose(&pair);
            if (pose) ++pose_ok; else ++pose_mismatch;
            const char *status = pose ? "candidate_pair_ready" : "pose_skeleton_unaligned";
            if (!representative[m.body_key]) {
                nflstreet_texture side = {0}, combined = {0};
                char image_path[4096];
                int wrote = 1;
                n = snprintf(image_path, sizeof image_path, "%s-body-key%u-side-by-side.bmp", argv[6], m.body_key);
                if (n < 0 || (size_t)n >= sizeof image_path ||
                    !nflstreet_team_player_selected_pair_render(&pair, 768, 640, 0.6f, &side) ||
                    !nflstreet_texture_bmp(&side, image_path)) wrote = 0;
                nflstreet_texture_close(&side);
                if (wrote && pose) {
                    n = snprintf(image_path, sizeof image_path, "%s-body-key%u-combined-candidate.bmp", argv[6], m.body_key);
                    if (n < 0 || (size_t)n >= sizeof image_path ||
                        !nflstreet_team_player_selected_pair_render_combined(&pair, 1024, 1024, 0.6f, &combined) ||
                        !nflstreet_texture_bmp(&combined, image_path)) wrote = 0;
                    nflstreet_texture_close(&combined);
                }
                if (!wrote) status = pose ? "candidate_pair_render_failed" : "pose_skeleton_unaligned";
                else representative[m.body_key] = 1;
            }
            row_log(report, &m, m.body_key, pair.body_binding.info_name,
                    pair.head.info_name, pair.body_mesh.count,
                    pair.head.mesh.count, pose, status);
            nflstreet_team_player_selected_pair_close(&pair);
            nflstreet_mesh_close(&body_mesh);
            nflstreet_team_body_binding_close(&binding);
        }
        free(db);
    }
    fclose(report);
    printf("All regular-team PLAY pair checks: rows=%u body_ok=%u pair_ok=%u shared_pose=%u unresolved_head=%u unsupported_tbyp=%u body_geometry_fail=%u head_geometry_fail=%u pose_mismatch=%u resolver_fail=%u body_keys=%u/%u/%u representatives=%u/%u/%u\n",
           rows, body_ok, pair_ok, pose_ok, unresolved_head, unsupported_body,
           body_geometry_fail, head_geometry_fail, pose_mismatch, manifest_fail,
           body_keys[0], body_keys[1], body_keys[2], representative[0],
           representative[1], representative[2]);
    free(plamodel); free(face); free(hair);
    return rows == 402 ? 0 : 9;
}
