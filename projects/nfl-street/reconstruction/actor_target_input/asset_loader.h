#ifndef NFLSTREET_ASSET_LOADER_H
#define NFLSTREET_ASSET_LOADER_H
#include "animation_callbacks.h"
typedef struct nflstreet_asset_store {
    uint32_t base;
    const uint8_t *arena, *registry;
    size_t arena_size, registry_size;
} nflstreet_asset_store;
typedef struct {
    nflstreet_animation_sources source;
    nflstreet_animation_blob metadata[64], transitions[64], targets[16384];
    nflstreet_animation_registry_entry registry[4096];
    uint8_t transition_table[520], metadata_table[520];
} nflstreet_loaded_animation;
/* Bounds-checked views over privately loaded data; source identities are
   opaque addresses in the source image, never host pointers. */
const uint8_t *nflstreet_asset_read(const nflstreet_asset_store *,uint32_t,size_t);
int nflstreet_asset_bank(const nflstreet_asset_store *,uint32_t,const uint8_t **);
int nflstreet_asset_bank_find(const nflstreet_asset_store *,uint16_t,uint16_t,uint32_t *);
/* Original group-to-descriptor table, bounded to 64 rows; atomic. */
int nflstreet_asset_group_rows(const nflstreet_asset_store *,uint16_t context,
                              uint16_t group,uint8_t out[520]);
int nflstreet_asset_metadata(const nflstreet_asset_store *,uint16_t,uint16_t,uint32_t *);
int nflstreet_asset_sources(const nflstreet_asset_store *,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,
                           nflstreet_loaded_animation *);
/* Offset-form animation bank -> loaded-form bank. Atomic. Does not install
   banks in the registry or invoke the original optional relocation hook.
   Cross-reference banks (flag 4) are rejected until their loader is verified. */
int nflstreet_asset_relocate(uint8_t *,size_t,uint32_t);
int nflstreet_animation_program_duration(const uint8_t *,size_t,float *);
#endif
