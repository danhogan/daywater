#ifndef NFLSTREET_TEAM_PLAYER_SELECTED_PAIR_H
#define NFLSTREET_TEAM_PLAYER_SELECTED_PAIR_H

#include "team_player_body_binding.h"
#include "team_player_selected_head.h"

typedef struct {
    uint32_t tbyp;
    uint32_t body_variant; /* This host inspection always selects variant 0. */
    nflstreet_team_body_binding body_binding;
    nflstreet_mesh body_mesh;
    nflstreet_team_player_head head;
} nflstreet_team_player_selected_pair;

/* Load two separately resolved resources. `head_index` must be supplied by
   the verified selected-PLAY head lookup; body binding follows TBYP and the
   verified PLYRBODY key/variant table. This does not attach either mesh. */
int nflstreet_team_player_selected_pair_open(
    const uint8_t *plamodel_bytes, size_t plamodel_size, uint32_t tbyp,
    const nflstreet_archive *face_archive, unsigned head_index,
    nflstreet_team_player_selected_pair *out);
void nflstreet_team_player_selected_pair_close(
    nflstreet_team_player_selected_pair *pair);

/* Side-by-side geometry inspection. Each panel is independently fitted to
   its own bounds; this intentionally avoids implying a shared pose space. */
int nflstreet_team_player_selected_pair_render(
    const nflstreet_team_player_selected_pair *pair, unsigned panel_width,
    unsigned panel_height, float yaw, nflstreet_texture *out);

/* Candidate common stored-pose composition. This is available only when the
   two TMdls expose a compatible shared root and matching face-used bones.
   Passing this check supports a host geometry hypothesis, not original actor
   attachment or original matrix semantics. */
int nflstreet_team_player_selected_pair_has_shared_pose(
    const nflstreet_team_player_selected_pair *pair);
int nflstreet_team_player_selected_pair_render_candidate(
    const nflstreet_team_player_selected_pair *pair, unsigned width,
    unsigned height, float yaw, nflstreet_texture *out);
/* Host-facing name for the same no-extra-transform candidate renderer. */
int nflstreet_team_player_selected_pair_render_combined(
    const nflstreet_team_player_selected_pair *pair, unsigned width,
    unsigned height, float yaw, nflstreet_texture *out);

#endif
