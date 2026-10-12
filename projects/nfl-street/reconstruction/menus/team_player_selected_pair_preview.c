/* Private side-by-side inspection of the selected PLAY head lookup and the
   separately verified TBYP body binding. The two panels are not assembled. */
#include "team_player_selected_pair.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static int read_file(const char *path, uint8_t **out, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long n = ftell(f);
    if (n < 16 || n > 128 * 1024 * 1024 || fseek(f, 0, SEEK_SET)) {
        fclose(f); return 0;
    }
    uint8_t *p = malloc((size_t)n);
    if (!p || fread(p, 1, (size_t)n, f) != (size_t)n) {
        free(p); fclose(f); return 0;
    }
    fclose(f); *out = p; *size = (size_t)n; return 1;
}

static int number(const char *s, unsigned long max, unsigned long *out) {
    errno = 0; char *end = NULL;
    unsigned long n = strtoul(s, &end, 10);
    if (errno || !s[0] || !end || *end || n > max) return 0;
    *out = n; return 1;
}

int main(int argc, char **argv) {
    if (argc != 6 && argc != 7) {
        fprintf(stderr, "usage: selected-pair-preview <plamodel.dat> <tbyp> <plaface.dat> <verified-head-entry-index> <private-output.bmp> [private-combined-candidate.bmp]\n");
        return 2;
    }
    unsigned long tbyp, head_index;
    if (!number(argv[2], 7, &tbyp) || !number(argv[4], 65535, &head_index))
        return 2;

    uint8_t *body_bytes = NULL, *face_bytes = NULL;
    size_t body_size = 0, face_size = 0;
    nflstreet_archive face_archive;
    nflstreet_team_player_selected_pair pair = {0};
    nflstreet_texture image = {0}, combined = {0};
    int ok = 0;
    if (!read_file(argv[1], &body_bytes, &body_size) ||
        !read_file(argv[3], &face_bytes, &face_size) ||
        !nflstreet_archive_open(face_bytes, face_size, &face_archive) ||
        !nflstreet_team_player_selected_pair_open(
            body_bytes, body_size, (uint32_t)tbyp, &face_archive,
            (unsigned)head_index, &pair) ||
        !nflstreet_team_player_selected_pair_render(
            &pair, 768, 640, 0.6f, &image) ||
        !nflstreet_texture_bmp(&image, argv[5])) {
        fprintf(stderr, "Selected body/head geometry inspection unsupported for these inputs.\n");
        goto done;
    }
    if (argc == 7 &&
        (!nflstreet_team_player_selected_pair_render_combined(
             &pair, 1024, 1024, 0.6f, &combined) ||
         !nflstreet_texture_bmp(&combined, argv[6]))) {
        fprintf(stderr, "Shared stored-pose composition candidate unsupported for these model records.\n");
        goto done;
    }
    printf("Selected PLAY resource comparison: head archive entry %lu Info=%s; TBYP=%lu -> body key=%u variant=0 archive entry=%u Info=%s; head triangles=%zu body triangles=%zu; shared head-used skeleton=%s. Separate stored-pose panels; optional combined image is a candidate using common model coordinates with no extra transform.\n",
           head_index, pair.head.info_name, tbyp, pair.body_binding.pool_key,
           pair.body_binding.archive_index, pair.body_binding.info_name,
           pair.head.mesh.count, pair.body_mesh.count,
           nflstreet_team_player_selected_pair_has_shared_pose(&pair) ? "yes" : "no");
    ok = 1;
done:
    nflstreet_texture_close(&image);
    nflstreet_texture_close(&combined);
    nflstreet_team_player_selected_pair_close(&pair);
    free(face_bytes); free(body_bytes);
    return ok ? 0 : 1;
}
