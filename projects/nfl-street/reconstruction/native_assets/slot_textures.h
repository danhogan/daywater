#ifndef NFLSTREET_SLOT_TEXTURES_H
#define NFLSTREET_SLOT_TEXTURES_H
#include "gear_texture.h"
#include "texture_apply.h"
/* One player's 25 owned slot destinations. Original 800E2958 lays them out in
   one block: slot s starts after the previous slots' capacities, where
   capacity = template decoded size rounded up to the next multiple of 32
   (an aligned size still gains 32). Bytes past each template size are padding. */
typedef struct {
 uint32_t token,template_index,template_size,capacity;size_t offset;
 int32_t applied_index;uint32_t applied_palette;
} nflstreet_slot_texture;
typedef struct {
 uint8_t *block;size_t block_size;uint32_t token,scratch_capacity;
 nflstreet_slot_texture slots[NFLSTREET_GEAR_TEXTURE_SLOTS];
} nflstreet_slot_textures;
/* 800E2958 sizing/layout plus 800E2AF0: decode each slot template into its
   destination and relocate it there (801C5610). scratch_capacity is the largest
   template size, the original 800E2CAC scratch allocation. */
int nflstreet_slot_textures_open(const nflstreet_gear_texture_catalog *,const nflstreet_archive *textures,
 uint32_t token,nflstreet_slot_textures *);
void nflstreet_slot_textures_close(nflstreet_slot_textures *);
static inline uint8_t *nflstreet_slot_texture_bytes(const nflstreet_slot_textures *t,unsigned slot){
 return t->block+t->slots[slot].offset;
}
/* 800D9C0C boundary: palette composition of destination palette record0 with
   the player's 24-byte color state. Reported, not executed. */
typedef int (*nflstreet_slot_palette)(void *,unsigned slot,uint32_t palette);
/* Fresh 800E2CAC/800E27F8 pass for one player after a boot-time 800E2958:
   every slot is pending, so a slot whose request is -1 reloads its template
   with palette -1. Each request is decoded (at most scratch_capacity bytes,
   the original scratch size), relocated at scratch_token and copied with
   80164CE4 rules; flush receives the original cache-flush order per slot. */
int nflstreet_slot_textures_apply(nflstreet_slot_textures *,const nflstreet_archive *textures,
 const int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS],const uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS],
 uint32_t scratch_token,nflstreet_texture_flush,nflstreet_slot_palette,void *);
#endif
