#ifndef NFLSTREET_ANIMATION_CALLBACKS_H
#define NFLSTREET_ANIMATION_CALLBACKS_H
#include "animation_channels.h"
typedef struct {
    uint32_t identity;
    const uint8_t *data;
    size_t size;
} nflstreet_animation_blob;
typedef struct {
    const uint8_t *registry;
    size_t registry_size;
    const uint8_t *bank;
    const uint8_t *groups;
    size_t groups_size;
} nflstreet_animation_registry_entry;
struct nflstreet_asset_store;
typedef struct {
    const struct nflstreet_asset_store *store;
    uint16_t bank_key;
    const uint8_t *bank, *groups, *program;
    size_t groups_size, program_size;
    const uint8_t *program_table;
    size_t program_table_size;
    uint32_t program_identity;
    const nflstreet_animation_blob *metadata_assets;
    size_t metadata_count;
    const uint8_t *profile, *speed_table;
    size_t speed_table_size;
    int side_source; /* -1 = absent; otherwise raw source byte at +0x39. */
    const uint8_t *transition_table;
    size_t transition_table_size;
    const nflstreet_animation_blob *transition_assets, *transition_targets;
    size_t transition_count, target_count;
    const nflstreet_animation_registry_entry *registry;
    size_t registry_count;
} nflstreet_animation_sources;
/* Scope: group 30, descriptor 0x1A/0x1B, unmirrored metadata, source profile
   speed table, transition modes 0/1/2, source-backed mode 3, or default. Source lookup providers
   are explicit; no selected row, channel index or transition outcome input. */
int nflstreet_animation_callback(uint8_t actor[0x14d4],const uint8_t partner[0x14d4],
                                uint8_t descriptor,const nflstreet_animation_sources *sources,
                                uint8_t globals[8]);
int nflstreet_animation_transition(uint8_t actor[0x14d4],uint16_t new_group,uint16_t old_group,
                                  const nflstreet_animation_sources *sources);
int nflstreet_animation_group_update(uint8_t actor[0x14d4],const uint8_t partner[0x14d4],
                                    uint8_t descriptor,const nflstreet_animation_sources *sources,
                                    uint8_t globals[8]);
int nflstreet_animation_transition_select(const nflstreet_animation_blob *assets,size_t count,
                                         uint16_t source_clip,uint16_t source_tag,
                                         uint16_t target_group,uint16_t target_clip,uint16_t target_bank,
                                         uint32_t *identity);
/* Paired contact application, including reciprocal links, both animation
   group updates, and owned event completion. Requests come from the native
   contact selector. No reference output blocks are consumed. */
int nflstreet_contact_animation_complete(uint8_t actor[0x14d4],uint8_t incoming[0x14d4],
                                        uint8_t manager[256],const uint8_t requests[2][16],
                                        const uint8_t descriptors[2],
                                        const nflstreet_animation_sources sources[2],uint8_t globals[8]);
#endif
