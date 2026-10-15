#ifndef NFLSTREET_ANIM_STREAM_H
#define NFLSTREET_ANIM_STREAM_H
/* The streamed animation banks (bank header flag 0x200) served from the
   extracted bank files (the animdata bank files) instead of ARAM.

   In the original, 80176650 asks 801749E8 for a frame. For a streamed bank
   the frame table word is either an ARAM address or 0xF0000000 | slot of
   the stream cache (80B40BE0 in the saved matches); 80179B60 then returns
   the cached slot (80179830, moved to the LRU tail by 8017972C) or evicts
   the LRU head (801798CC/80179900: the evicted frame's table word gets its
   ARAM address back, the new one gets the slot) and reads it through the
   read callback r13-0x5790 = 801519A8: 8017BB78 copies 0xC0 bytes of ARAM
   at the 32-byte aligned address into one of two buffers at 8030B8E0
   (alternating on r13-0x5968), and 8015E938 decodes the frame from the
   buffer into the slot (8017BC7C copies it for an uncompressed bank).

   The native service maps each streamed bank of the bank table (r13-0x4F6C,
   0x1C bytes per entry, header at +C) to its file and derives the bank's
   ARAM base from the saved frame table words (ARAM address = base + the
   file's frame offset), or from the cache slots' saved ARAM words for
   frames that are cached (frame words naming a slot that now holds another
   frame, as some saved matches have, are not evidence). ARAM bytes outside every mapped file read as 0
   (host choice: their real content is unknown; no decoded frame reaches
   them in the shipped banks). It serves 8017BB78 (nflstreet_anim_aram_read)
   for the RAM-identical cache path in actor_channels.c, and whole frames by
   bank index and frame number (nflstreet_anim_aram_frame) for the direct
   path that leaves the cache untouched. */
#include <stddef.h>
#include <stdint.h>
#include "player_draw.h"
#define NFLSTREET_ANIM_BANKS 32u
typedef struct {
 unsigned index;            /* bank table entry */
 uint32_t base;             /* ARAM address of file byte 0 */
 uint32_t first;            /* the lowest frame offset: bytes before it are not in ARAM */
 const uint8_t *data;size_t size;
} nflstreet_anim_aram_bank;
typedef struct {nflstreet_anim_aram_bank bank[NFLSTREET_ANIM_BANKS];unsigned count;} nflstreet_anim_aram;
/* Matches every flag 0x200 bank table entry of the saved RAM to one of the
   files (same first six bytes: type, id and frame count) and derives a
   single consistent ARAM base. Entries no file matches (free table slots
   whose header word points elsewhere) stay unmapped. Returns the number of
   streamed banks mapped, or -1 when a matching file's words disagree. */
int nflstreet_anim_aram_map(nflstreet_anim_aram *,nflstreet_draw_span,void *memory,const uint8_t *const files[],const size_t sizes[],unsigned count);
/* 8017BB78 served: n bytes of ARAM from address (unmapped bytes 0). */
void nflstreet_anim_aram_read(const nflstreet_anim_aram *,uint32_t address,uint8_t *to,uint32_t n);
/* A streamed frame by bank table entry and frame number: its file bytes
   (NULL when the bank or frame is not mapped). */
const uint8_t *nflstreet_anim_aram_frame(const nflstreet_anim_aram *,unsigned index,unsigned frame,size_t *available);
#endif
