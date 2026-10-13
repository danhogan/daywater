#ifndef NFLSTREET_MATCH_SKELETONS_H
#define NFLSTREET_MATCH_SKELETONS_H
#include "match_visuals.h"
#include "skeleton_startup.h"
typedef struct {
 uint32_t namespace_base,namespace_size,skeleton_token,face_view_token,hair_view_token,temporary_token,temporary_capacity;
 unsigned face_target,hair_target; /* Explicit resource indices; no actor inference. */
} nflstreet_match_skeleton_input;
typedef struct {
 nflstreet_skeleton_startup players[NFLSTREET_MATCH_PLAYERS];
 uint32_t face_view_tokens[NFLSTREET_MATCH_PLAYERS],hair_view_tokens[NFLSTREET_MATCH_PLAYERS];
 unsigned count,temporary_allocations,temporary_releases;
} nflstreet_match_skeletons;
/* Host transaction around verified skeleton startup and face/hair binding.
   Clone mutable geometry privately, bind every player, then replace geometry
   in visuals only after total success. Surfaces and shared body data unchanged. Bind before publishing face/hair
   buffer pointers: successful commit invalidates the prior geometry buffers.
   Failure preserves both visuals and output. Empty output required. Keep this
   skeleton owner alive while bound visual views are used; close visuals first.
   Body skinning, actor allocation and draw state remain separate. */
int nflstreet_match_skeletons_bind(nflstreet_match_visuals *,const nflstreet_body_resource_pool *,
 const nflstreet_archive *,const nflstreet_skeleton_startup_catalog *,
 const nflstreet_match_skeleton_input[NFLSTREET_MATCH_PLAYERS],nflstreet_match_skeletons *);
void nflstreet_match_skeletons_close(nflstreet_match_skeletons *);
#endif
