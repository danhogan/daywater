#ifndef NFLSTREET_PLAYER_SURFACE_H
#define NFLSTREET_PLAYER_SURFACE_H
#include "slot_materials.h"
#include "palette_recolor.h"
/* Host ownership around the reconstructed stages, not a recovered game struct.
   Tokens identify three disjoint caller-reserved namespaces; material storage
   is allocated natively with 32-byte token strides and the supplied fill. */
typedef struct {
 const nflstreet_gear_texture_catalog *gear;
 const nflstreet_slot_material_catalog *materials;
 const nflstreet_archive *textures;
 const nflstreet_dynclut_table *dynclut;
 const nflstreet_color_table *colors;
 uint32_t destination_token,scratch_token,material_token,material_capacity;
 uint8_t allocator_fill;
 nflstreet_texture_flush flush;void *flush_opaque;
} nflstreet_player_surface_context;
#define NFLSTREET_SURFACE_BLOCKS (NFLSTREET_GEAR_TEXTURE_SLOTS*5u)
typedef struct {
 nflstreet_slot_textures textures;
 nflstreet_slot_material bindings[NFLSTREET_GEAR_TEXTURE_SLOTS];
 nflstreet_actor_material instances[NFLSTREET_ACTOR_MATERIALS_MAX];unsigned instance_count;
 nflstreet_material_block allocations[NFLSTREET_SURFACE_BLOCKS];unsigned allocation_count;
 uint32_t material_used;
} nflstreet_player_surface;
/* Apply a completed gear plan (including its slot0 fallback), a separate
   original face request for slot1 and a completed player's +4C..+63 color
   state, then bind/configure/hook materials. Catalog/name pointers are borrowed
   and must outlive the owner. Output must be empty; failure preserves it and
   frees every new host allocation. Flush callback effects cannot be rolled back.
   Caller must reserve token spans against other geometry/actor resources. */
int nflstreet_player_surface_open(const nflstreet_player_surface_context *,const nflstreet_gear_texture_plan *,
 int32_t face_index,uint32_t face_palette,const uint8_t state[NFLSTREET_COLOR_STATE_BYTES],nflstreet_player_surface *);
/* Fresh selected-player entry point: completed 0x68-byte player record from
   player_record_build and normalized roster name from the selected geometry
   manifest. Builds gear decisions and performs the original named face lookup;
   does not read captured RAM. The +65 plan result stays caller-owned. */
int nflstreet_player_surface_from_player(const nflstreet_player_surface_context *,const nflstreet_gear_records *,
 const nflstreet_archive *dynamic_palettes,const uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],
 const char *normalized_name,nflstreet_player_surface *);
void nflstreet_player_surface_close(nflstreet_player_surface *);
#endif
