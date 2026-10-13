#ifndef NFLSTREET_SLOT_MATERIALS_H
#define NFLSTREET_SLOT_MATERIALS_H
#include "slot_textures.h"
#define NFLSTREET_SLOT_MATERIAL_NAME_BYTES 16u
/* Private SMC1 catalog exported by export_slot_material_catalog.py: the 25
   material names of the original 80276C6C binding table (HAIR ... GLASSES),
   16 bytes each, NUL-terminated, then the 80164DD8 default settings list
   (8028DD90): a count and per entry the name and six words (clamp-wrap byte,
   min filter, mag filter, LOD bias float, max anisotropy, bias-clamp and
   edge-LOD bytes). Then the 8028DD10 player material list (count and per
   descriptor the material, shader and texture names, 16 bytes each) and the
   OnePass/TwoPass shader definitions (name, stage count, 20-byte stage
   records: name, type byte, flag byte, BE16 stage index). */
typedef struct {
 const uint8_t *names,*settings,*descriptors,*shaders[2];unsigned setting_count,descriptor_count,shader_stages[2];
} nflstreet_slot_material_catalog;
int nflstreet_slot_material_catalog_open(const uint8_t *,size_t,nflstreet_slot_material_catalog *);
const char *nflstreet_slot_material_name(const nflstreet_slot_material_catalog *,unsigned slot);
/* Original 8018B7B8 allocation: the service returns a token and the bytes at
   that token, with whatever contents the allocator leaves. The original reads
   or keeps some of those bytes, so native code writes into them the same way. */
typedef int (*nflstreet_material_allocate)(void *,uint32_t size,uint32_t *token,uint8_t **bytes);
typedef struct {uint32_t token,size;uint8_t *bytes;} nflstreet_material_block;
/* The 0x28-byte texture set built by original 801C8E80 for one relocated
   MMAPv2 destination, with its four arrays (absent when the count is 0):
   entries (0x10 per image), texture objects (0x2C per image: a GXTexObj from
   GXInitTexObj/CI, LOD 0..mips-1, filters 5/1, wrap REPEAT for power-of-two
   sizes else CLAMP, then mips/width/height/size), palette objects (0x28 per
   palette: a GXTlutObj, or a 16x16 GXTexObj for palette format 6, then the
   palette size) and the name table (8 per image). */
typedef struct {nflstreet_material_block object,entries,textures,palettes,names;} nflstreet_texture_set;
/* Original 801EBD38 for one destination (token 0 builds an empty set). Cache
   flushes go to flush in original order: each palette (entries*4 bytes),
   then each image (GXGetTexBufferSize). The destination is relocated first
   (a no-op when it already is). Only MMAPv2 is accepted. */
int nflstreet_texture_set_build(uint8_t *mmap,size_t size,uint32_t token,nflstreet_material_allocate,void *,
 nflstreet_texture_flush,void *,nflstreet_texture_set *);
/* One 32-byte entry of the original 80276C6C table for one player and slot:
   the material name (DOL, unchanged) and the words 801EBD38 writes (+14 =
   texture set entries, +18 = texture set, +1C = destination). */
typedef struct {const char *material;uint32_t entries,set,destination;nflstreet_texture_set texture_set;} nflstreet_slot_material;
/* Original 800E2BE8 for one player: 801EBD38 on each of the 25 owned slot
   destinations in slot order. The 8017B570 loader pool it also creates is not
   modeled. */
int nflstreet_slot_materials_bind(const nflstreet_slot_material_catalog *,nflstreet_slot_textures *,
 nflstreet_material_allocate,void *,nflstreet_texture_flush,void *,nflstreet_slot_material out[NFLSTREET_GEAR_TEXTURE_SLOTS]);
/* Original 80164DD8(table, 0) with the default list, as actor startup 801639C8
   runs it on the player's binding table: for each list entry, the first slot
   with that material name (801EBB44); a set clamp byte makes every texture
   object of its set CLAMP/CLAMP (801C9208), then 801C9348 reapplies GXTexObj
   LOD with the entry's filters, bias and flags (max LOD stays mips-1). A list
   name without a slot rejects (the original would dereference null). */
int nflstreet_slot_materials_configure(const nflstreet_slot_material_catalog *,nflstreet_slot_material m[NFLSTREET_GEAR_TEXTURE_SLOTS]);
/* One stage entry of an actor material instance (12 bytes in the original). */
typedef struct {uint32_t texture,palette,index;} nflstreet_material_stage;
#define NFLSTREET_ACTOR_MATERIALS_MAX 64u
/* Final state of one actor material instance after actor startup 801639C8:
   shader 0 = OnePass (r13-5638), 1 = TwoPass (r13-5634), its stage count and
   stages. */
typedef struct {const char *material,*texture;unsigned shader,stage_count;nflstreet_material_stage stages[2];} nflstreet_actor_material;
/* Original 801C7528 + 801EBBA8 + the eight 801EBC78 tattoo calls of 801639C8
   for the 8028DD10 list and one player's binding table. Each descriptor's
   texture name selects the first slot with that name (801EBB44); stage "base"
   gets that slot's first texture object and, when its first entry has
   palettes, the entry's palette object (801C9088/801C90AC); its index word is
   -1 (801CCB50 with no texture-index list, r13-63F8 null). The shader is then
   OnePass. Materials 10/11, 19/20, 14/15 and 23/24 take tattoo slots
   19/20/21/22: when that slot has a texture, the shader becomes TwoPass
   (stage array rebuilt, stage 0 kept, stage 1 zeroed) and stage "over" gets
   the tattoo texture and palette. A descriptor without a slot rejects (its
   startup-registry state is not modeled). */
int nflstreet_actor_materials_hook(const nflstreet_slot_material_catalog *,const nflstreet_slot_material binding[NFLSTREET_GEAR_TEXTURE_SLOTS],
 nflstreet_actor_material out[NFLSTREET_ACTOR_MATERIALS_MAX],unsigned *count);
#endif
