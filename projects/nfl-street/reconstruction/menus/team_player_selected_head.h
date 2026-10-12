#ifndef NFLSTREET_TEAM_PLAYER_SELECTED_HEAD_H
#define NFLSTREET_TEAM_PLAYER_SELECTED_HEAD_H

#include "../native_assets/archive.h"
#include "../native_assets/model.h"
#include "../native_assets/texture.h"

typedef struct {
 unsigned archive_index;
 char info_name[128];
 uint8_t *owned_model_bytes;
 size_t model_size;
 nflstreet_mesh mesh;
} nflstreet_team_player_head;

/* Load an explicit face-archive entry selected by the separately checked
   PLAY name lookup. Geometry uses the TMdl stored cluster pose, including its
   scale vectors. The caller retains the archive; this object owns decoded
   model bytes because material pointers refer into them. */
int nflstreet_team_player_head_open(const nflstreet_archive *,unsigned,
 nflstreet_team_player_head *);
void nflstreet_team_player_head_close(nflstreet_team_player_head *);

/* Flat inspection rendering only. No game's material, palette, actor
   attachment, animation, or lighting state is inferred. */
int nflstreet_team_player_head_render(const nflstreet_team_player_head *,
 unsigned width,unsigned height,float yaw,nflstreet_texture *);

#endif
