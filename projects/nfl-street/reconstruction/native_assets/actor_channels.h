#ifndef NFLSTREET_ACTOR_CHANNELS_H
#define NFLSTREET_ACTOR_CHANNELS_H
/* The gameplay actors' animation update on saved RAM: 8003A24C (called once
   per game update from 8002E24C with the elapsed updates as f1), without
   its per-actor sub-animations 80039990 (flag 0x10 of the visual +14) and
   80039A18 (the two attachment blocks), which are not reconstructed here.
   For every gameplay actor (r13-0x7C30 table, stride 0x14D4; manager +9BC,
   channel set +4D8, channels +4E4 = ten 0x7C-byte records), in this order:

   1. 80178290 manager tick for all actors: the clip-set records' delay
      (+C) and weight blend (+50, the curve table r13-0x6714: 80178E78 none,
      80178E7C linear, 80178F04 delay, 80178F28 rise/hold/fall), retiring
      (80177A64: group callback mode 3, 801740BC stops its channels) or
      finishing groups (callback modes 2 and 1, 8017428C), then 80177950
      compaction.
   2. 80173D60 channel step for all actors: the channel delay (+48), its
      weight blend (+30), and the clip program (80174374 -> 80177374 /
      80176E28 / 80176DD0): the frame pair (+5C/+5E), time (+54),
      remaining (+58) and fraction (+60); program events (0xC000..) go to
      the event callback r13-0x6734 through 80173A24.
   3. Per actor: the visual's +1C/+20 from +304/+308, the pose list
      80176650 (at 8032BF28; frames through 801749E8, the stream cache
      80179B60 for streamed banks), the blend 800D4CBC/8015E9C8, the channel
      roots and heading travel 80176904 (with 8017650C: the shortest turn
      8018879C, or the quaternion slerp 801A3794/801A383C/801A3580 past a
      quarter turn), the root travel 80176B3C, then 800D4CE4/8015EC30.

   The channel roots (+10..+18) are produced by 80176904 from the pose
   list's entry roots (written by 8015E9C8): a channel with two entries
   gets the interpolation by its fraction (+60) between them, a single
   entry its roots as they are; flag 2 entries then move the heading
   (+14, zeroed) into the travel accumulator (+1C/+20, the actor's +224).

   Providers (not reconstructed, called through the hooks below):
   - The program event callback (8001532C in the saved matches): the hook
     receives (actor, command, key) as 80173A24 passes them and returns the
     callback's r3; it may read the command's operand words through
     nflstreet_actor_channels_operand (801777A0) as the original does.
     NULL: no event effects and r3 = 1 (accepted, no operands consumed).
   - The group callbacks of 801778C0 (table r13-0x6718): the hook receives
     the function word and the original arguments (resources 8032C34C as
     80175AF8 filled them, key, channel set, channels, actor, mode). NULL:
     nothing.
   - The stream service (anim_stream.h): 8017BB78's ARAM reads come from the
     bank files. With direct set, streamed frames are instead decoded from
     the files into host memory (0xA0000000.., through this context's span)
     and the stream cache, the bank frame tables, the read buffers 8030B8E0
     and r13-0x5968 are left untouched (the list entries' +28 then point to
     host memory). Without it, the cache is updated as the original does.

   8015E9C8's stale stack frame is the per-actor nflstreet_actor_pose_context
   local (see actor_pose.h). All addresses go through the span; constants
   are read from the original's data. Each call returns 0 when an address
   falls outside the span or a dead/unsupported case is reached (a curve
   function outside the four, a bank with flag 4 on the streamed path,
   rem_pio2f's large-argument path); writes made before stay. */
#include "actor_pose.h"
#include "anim_stream.h"
#define NFLSTREET_CHANNEL_ACTORS 16u
#define NFLSTREET_CHANNEL_DIRECT 0xa0000000u
#define NFLSTREET_CHANNEL_DIRECT_SLOT 0x200u
typedef struct nflstreet_actor_channels nflstreet_actor_channels;
typedef uint32_t (*nflstreet_channel_event)(void *user,nflstreet_actor_channels *,uint32_t actor,uint32_t command,uint32_t key);
typedef void (*nflstreet_channel_group)(void *user,nflstreet_actor_channels *,uint32_t function,uint32_t resources,uint32_t key,uint32_t set,uint32_t channels,uint32_t actor,uint32_t mode);
struct nflstreet_actor_channels {
 nflstreet_draw_span span;void *memory;           /* the game's RAM */
 const nflstreet_anim_aram *aram;                 /* required for streamed banks */
 int direct;                                      /* serve streamed frames without the cache */
 nflstreet_channel_event event;nflstreet_channel_group group;void *user;
 nflstreet_actor_pose_context pose[NFLSTREET_CHANNEL_ACTORS]; /* per actor index; span/memory set by the calls */
 uint8_t host[0x18u*NFLSTREET_CHANNEL_DIRECT_SLOT+0x100u];      /* direct frames, one per list entry, then the read window */
 int ok;                                          /* internal: cleared when an access fails */
};
/* 8003A24C (without 80039990/80039A18) for all gameplay actors; dt is the
   elapsed updates (1.0 per 60 Hz update in the shipped loop). */
int nflstreet_actor_channels_update(nflstreet_actor_channels *,float dt);
/* Its pieces for one actor (gameplay actor address; index selects pose[]). */
int nflstreet_actor_channels_manager(nflstreet_actor_channels *,float dt,uint32_t actor);  /* 80178290 */
int nflstreet_actor_channels_step(nflstreet_actor_channels *,float dt,uint32_t actor);     /* 80173D60 */
int nflstreet_actor_channels_pose(nflstreet_actor_channels *,unsigned index,uint32_t actor); /* phase 3 */
/* 801777A0 for an event hook: the next operand of the stream at r13-0x4F58. */
int nflstreet_actor_channels_operand(nflstreet_actor_channels *,uint32_t *value);
#endif
