#ifndef NFLSTREET_ACTOR_POSE_H
#define NFLSTREET_ACTOR_POSE_H
/* A visual actor's animation pose on saved RAM, as its update 80020FDC
   builds it before 800D51B4 (actor_skeleton.h): the pose list from
   80176650 is blended into the visual by 8015E9C8, its root rotation and
   extra bones are set from the channels by 8015EC30, and the torso and
   neck twist is added by 80053404. The pose (+5C) holds signed 16-bit
   x/y/z angles per compact SKL1 bone (compact order, +30 bones; the
   streamed clips write 26), the same buffer 801EAE1C and 80162760 read.
   Streamed banks (flag 0x200) reach 8015E9C8 already decoded by 8015E938
   (the ARAM read callback 801519A8), with no compression descriptor.
   All addresses go through the span (big-endian memory); constants and
   the mirror table are read from the original's data (80260638..,
   802606B0, 80261748, 80261180, 8026E338.., 8028CB30, 8028CBA8..).
   Each call returns 0 when an address falls outside the span; it is not
   transactional (writes made before that point stay). The native decoder
   also rejects field widths over 16 bits and five-field groups over 64
   bits, which the original's 64-bit reader (80151ABC) would mis-extract;
   the shipped descriptors have neither. */
#include "player_draw.h"
/* 8015E9C8 keeps a blend descriptor and a 384-byte decode buffer in its
   stack frame (r1+8 .. r1+1C0). Entries after the first are decoded there
   and only partly overwritten (the heading under flag 4, components past
   the frame's), so the stale bytes are an input: local holds that frame,
   addressed as NFLSTREET_POSE_LOCAL (the buffer pointer word at +30 is
   rewritten on every call). */
#define NFLSTREET_POSE_LOCAL 0xf0000000u
#define NFLSTREET_POSE_LOCAL_BYTES 0x1b8u
typedef struct {
 nflstreet_draw_span span;void *memory;
 uint8_t local[NFLSTREET_POSE_LOCAL_BYTES];
} nflstreet_actor_pose_context;
/* 8015E938(to, from, descriptor, mirror): copy a compressed frame's header
   and decode its components (8015D104, scattered when mirror) dequantized
   (801667CC) after it: the streamed frame record. */
int nflstreet_actor_pose_stream(nflstreet_actor_pose_context *,uint32_t to,uint32_t from,uint32_t descriptor,int mirror);
/* 8015E9C8(visual, list): list is the 80176650 layout (u16 count, then
   0x2C-byte entries from +4: +4 normalized weight, +8 flags, +24
   compression descriptor or 0, +28 frame). Writes the pose, the root
   offset +34, the ball vector +40, heading +4C, the ball angles +50, the
   ball bone +32, the entries' +C..+1C and the local frame. */
int nflstreet_actor_pose_blend(nflstreet_actor_pose_context *,uint32_t visual,uint32_t list);
/* 8015EC30(visual, channels, count, clips): the weighted root rotation of
   the active 0x7C-byte channels (+10..+18) into pose bone 0 and +4C, then
   zero bones 26.. (to 29, or only 26 with flag 0x80). */
int nflstreet_actor_pose_root(nflstreet_actor_pose_context *,uint32_t visual,uint32_t channels,unsigned count,uint32_t clips);
/* 80053404(gameplay): the twist angles +12C/+130 added to bones 9-12. */
int nflstreet_actor_pose_twist(nflstreet_actor_pose_context *,uint32_t gameplay);
#endif
