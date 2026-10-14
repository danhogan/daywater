#ifndef NFLSTREET_NAME_TAG_PROJECT_H
#define NFLSTREET_NAME_TAG_PROJECT_H
/* A name tag's screen position, on saved RAM: the original 800DB0E4, which
   the per-player step 800E20E4 (called by the draw traversal's player
   callback 8012888C right before the player draws, 80164000) runs for a
   player whose tag (visual actor +3FC) has its shown byte (+1C) set.
   The visual actor's position (+04) goes through slot 4 (the camera's
   clip matrix, camera_slots.h); points outside the clip volume hide the tag
   (+1C = 0), others get the screen position (+04, +08: slot 1's 320 and 224
   scales; +08 lifted by 25 above the point), shown (+1C = 1), and the
   projected depth times r13-7244 (+20). The HUD draw (nflstreet_name_tag_draw,
   800DADC0) then prints the tag at that position. */
#include "camera_slots.h"
/* 800DB0E4(tag, position): position is the address of three floats. The
   camera node being drawn (word r13-4EEC, whose +D0 is the aspect) must be
   set, as it is while the original traverses the camera; it is zero between
   frames and in the saved RAM. */
int nflstreet_name_tag_project(const nflstreet_view_context *,uint32_t tag,uint32_t position);
/* 800E20E4 without the player draw: project the tag of the visual actor if
   it has one and it is shown. */
int nflstreet_actor_name_tag(const nflstreet_view_context *,uint32_t actor);
#endif
