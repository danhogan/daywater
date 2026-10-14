#ifndef NFLSTREET_SCENE_FRAME_H
#define NFLSTREET_SCENE_FRAME_H
/* The original in-match frame order over the native draws: the field
   camera's view (8017D16C, then 801DFAB0's copy into stack slot 3), the sky,
   the field passes with the ground marks and ball shadows at their original
   points, actor shadows, field lines, player-select rings, the main field,
   the ball, the 14 players, then the HUD camera, name tags and in-match
   screens. Ten further models and the crowd are not drawn. */
#include "scenery_draw.h"
#include "hud_draw.h"
#include "marks_draw.h"
#include "ball_draw.h"
#include "uis_draw.h"
#include "player_draw.h"
typedef struct {
 nflstreet_player_draw_context draw; /* span, memory and emit set by the caller */
 nflstreet_scenery_context venue;
 nflstreet_hud_context hud;
 nflstreet_marks_context marks;
 nflstreet_ball_context shadows;
 nflstreet_ball_model_context ball;
 nflstreet_uis_context screens;
 nflstreet_pass_hooks hooks;
 uint32_t field_camera,hud_camera,main_field,field_passes;
 uint32_t lines[4],rings[3],name_tags,name_tag_count;
 uint32_t actors,actor_count;  /* value of r13-7BBC, and the count */
 uint32_t screen_camera_word;  /* r13-6CD8 */
 uint32_t view_slot;           /* 80331F80: stack slot 3 */
} nflstreet_scene;
/* Links the contexts to each other and to scene->draw; call once the
   struct is at its final address. */
void nflstreet_scene_link(nflstreet_scene *);
int nflstreet_scene_venue(nflstreet_scene *,nflstreet_render_state *);   /* camera to ball */
int nflstreet_scene_players(nflstreet_scene *,nflstreet_render_state *); /* the 14 players */
int nflstreet_scene_hud(nflstreet_scene *,nflstreet_render_state *);     /* HUD camera, tags, screens */
#endif
