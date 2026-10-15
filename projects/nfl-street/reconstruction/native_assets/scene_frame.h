#ifndef NFLSTREET_SCENE_FRAME_H
#define NFLSTREET_SCENE_FRAME_H
/* The original in-match frame order over the native draws: the field
   camera's view (8017D16C, then 801DFAB0's copy into stack slot 3), the sky,
   the field passes with the ground marks and ball shadows at their original
   points, actor shadows, field lines, player-select rings, the main field,
   the field camera's light update (801ED3A0, optional), the crowd and
   street props (800D2F20 entries, optional), the ball, the 14 players (each
   optionally rebuilding its skinning palettes first, 80162760 as called
   from 80164000, after projecting its name tag when view is set), then the
   HUD camera, name tags and in-match screens.
   With effects set, also the actor fans before the ball and the actor
   strips and particles after entry 14 (800D1118), following the players.
   The sky node includes clouds (invisible in the saved frames). */
#include "scenery_draw.h"
#include "hud_draw.h"
#include "marks_draw.h"
#include "ball_draw.h"
#include "uis_draw.h"
#include "player_draw.h"
#include "actor_palettes.h"
#include "light_rig.h"
#include "crowd_draw.h"
#include "name_tag_project.h"
#include "camera_projection.h"
#include "effects_draw.h"
#include "actor_scale.h"
#include "field_overlay.h"
typedef struct {
 nflstreet_player_draw_context draw; /* span, memory and emit set by the caller */
 nflstreet_scenery_context venue;
 nflstreet_cloud_context clouds;
 nflstreet_field_overlay_context field_overlay;
 int field_overlay_enabled; /* caller supplies EFB depth service; original byte r13-71C0 still gates the entry */
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
 /* When set, each drawn player's palettes are rebuilt from its pose and
    the current camera before it draws (the frame's palette pool starts
    empty); when unset the stored palettes are drawn. */
 nflstreet_actor_palette_context *palettes;
 /* When set, the light sets are rebuilt from the camera each frame (the
    field camera node's phase-1 callback); when unset the stored ones load. */
 nflstreet_light_rig_context *lights;
 /* The crowd and street props: crowd_count 800D2F20 entries 0x1DC apart
    (needs palettes: they share the frame's palette pool and locked cache). */
 nflstreet_crowd_context crowd;
 uint32_t crowd_first,crowd_count;
 /* When set, the camera's derived slots (3 to 6) are rebuilt from the view
    (801DFAB0's tail) instead of copying slot 3 alone, and each player's
    name tag is projected through slot 4 right before it draws (800E20E4),
    with the camera node word (r13-4EEC) naming the field camera meanwhile. */
 const nflstreet_view_context *view;
 uint32_t camera_node_word;
 /* When effects is set: the actor fans (801646CC) before the ball, and
    after the players the actor strips (801649C4) and particles (80158798). */
 int effects;
 nflstreet_fans_context fans;
 nflstreet_strips_context strips;
 nflstreet_particles_context particles;
 uint32_t r13; /* small-data base: the scaled-actor byte r13-7238 (800DFBA8/800DFB34) */
} nflstreet_scene;
/* Links the contexts to each other and to scene->draw; call once the
   struct is at its final address. */
void nflstreet_scene_link(nflstreet_scene *);
/* Frame start: the frame's palette pool starts empty (801CA494 from
   801C769C). Call before the venue (or the players, if the venue is not
   drawn) when palettes are rebuilt. */
int nflstreet_scene_start(nflstreet_scene *,nflstreet_render_state *);
int nflstreet_scene_venue(nflstreet_scene *,nflstreet_render_state *);   /* camera to ball */
int nflstreet_scene_players(nflstreet_scene *,nflstreet_render_state *); /* the 14 players */
int nflstreet_scene_hud(nflstreet_scene *,nflstreet_render_state *);     /* HUD camera, tags, screens */
#endif
