#ifndef NFLSTREET_LIVE_POSE_H
#define NFLSTREET_LIVE_POSE_H
/* The controlled player's pose for the live app, per 60 Hz update, in the
   original +5C layout on saved RAM. Verified pieces (actor_pose.c against
   the original): 8015E938 streamed-frame decode, 8015E9C8 pose blend,
   8015EC30 root and extra bones, 80053404 twist. Host choices (not
   reconstructed, named here and in live_pose.md):
   - Clip selection and channel timing come from live_locomotion.c (source
     curves, paired streams, NSC1 set catalog) in place of the gameplay
     actor's own channels (+320) and animation services.
   - The pose list is built from those four channels with the verified
     80176650 weight expansion (nflstreet_locomotion_pose_weights), entry
     flags 0 (no mirror, no root-motion flag 4, ball mode from +20) and no
     group (clip-set) weighting.
   - Channel root rotations (+10..+18, whose producer is not identified) are
     the 24-bit shortest-turn interpolation (8018879C form) by the channel's
     fraction between its two frames' list-entry roots (+C, +10, +14 after
     8015E9C8), with heading 0 under flag 4. This matches the saved bank 7
     channels of position-neutral-15 exactly; it is an inference.
   The bank is animdata/00111.bin (bank 7, resident at bank table entry 2 in
   the saved matches); frames are decoded from it directly instead of
   through the ARAM stream cache. Bank and catalog memory must outlive the
   state. */
#include "actor_pose.h"
#include "live_locomotion.h"
#define NFLSTREET_LIVE_POSE_ENTRIES 8u
#define NFLSTREET_LIVE_POSE_HOST_BYTES 0x1000u
typedef struct {
 nflstreet_live_locomotion locomotion;
 const uint8_t *bank,*catalog;size_t bank_size,catalog_size;
 nflstreet_actor_pose_context pose;          /* 8015E9C8's frame persists across updates */
 nflstreet_draw_span ram_span;void *ram;     /* set on each step */
 uint8_t host[NFLSTREET_LIVE_POSE_HOST_BYTES]; /* descriptor, list, records, channels, clip set */
 unsigned updates,entries;
} nflstreet_live_pose;
/* Bank 7 (animdata/00111.bin) and an NSC1 set catalog (e.g. sets-14.bin). */
int nflstreet_live_pose_open(nflstreet_live_pose *,const uint8_t *bank,size_t,const uint8_t *catalog,size_t);
void nflstreet_live_pose_reset(nflstreet_live_pose *);
/* One update of the gameplay actor's visual (its +04) from its movement
   blocks as last written (+1A8 state, +200 input): heading state[6]
   (+1C0), state[8], speed state[7]. set selects the catalog's profile;
   updates is the elapsed time in 60 Hz updates (1 per update). Writes the
   visual's +18/+1C/+20 from +300/+304/+308 (as 80020FDC), then the pose
   and root fields. The skeleton (+60) is left to nflstreet_actor_skeleton_frame. */
int nflstreet_live_pose_step(nflstreet_live_pose *,nflstreet_draw_span,void *memory,uint32_t gameplay,unsigned set,double updates);
#endif
