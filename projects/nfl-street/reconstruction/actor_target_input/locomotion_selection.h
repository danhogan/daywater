#ifndef NFLSTREET_LOCOMOTION_SELECTION_H
#define NFLSTREET_LOCOMOTION_SELECTION_H
#include "asset_loader.h"
typedef struct {int32_t row;uint16_t score,bank_key,clip_key;uint32_t metadata_identity;} nflstreet_locomotion_choice;
/* Original 0x800211D8 weighted exact-byte selection. Source rows and
   metadata are inputs; selected index/bank/clip are computed, never supplied.
   This does not reconstruct movement-to-group or variant-key generation. */
int nflstreet_locomotion_select(const nflstreet_asset_store *,const uint8_t *,
                               size_t,const uint8_t keys[4],nflstreet_locomotion_choice *);
/* Original normal group 4A two-level descriptor resolution. Variant keys
   are upstream inputs; game-service generation of those bytes is not covered.
   Updates the original 64-byte manager payload transactionally. */
int nflstreet_locomotion_resolve(const nflstreet_asset_store *,uint16_t context,
                                const uint8_t keys[2],uint8_t manager_data[64]);
/* Original 0x80064638 index and its adjacent clip IDs. Does not assign a
   gameplay name to the speed units or select between primary/other curves. */
int nflstreet_locomotion_pair(const uint8_t *curve,size_t,double signed_speed,
                             uint16_t *index,uint32_t clips[2]);
/* Original 0x800648EC paired weights/rates. Channel records retain original
   124-byte layout. Requires resolved stream tag counts (not 0xffff); upstream
   stream setup and channel allocation remain caller responsibilities. */
int nflstreet_locomotion_blend(uint8_t *curve,size_t,uint16_t index,double speed,
                              const uint8_t descriptor[8],uint8_t *channels,
                              size_t channel_size,float *rate);
/* Bounded original callback countdown plus 0x80064B04 retiring weights.
   Descriptor uses original 64-byte payload, including duration +3A / left +3B. */
int nflstreet_locomotion_fade(uint8_t data[64],uint8_t *channels,size_t);
/* Original 0x80176650 per-frame expansion and cumulative normalization for
   supplied channel weights (group weight already applied), in slot order. */
int nflstreet_locomotion_pose_weights(const float *,const float *,unsigned,
                                     uint8_t slots[32],uint8_t next[32],float weights[32],unsigned *);
/* Original 0x80062624 decision. Inputs retain neutral names until their
   gameplay meanings are established. Asset identities preserve aliases. */
typedef struct { float speed, comparison, bound_a, bound_b; uint8_t status; } nflstreet_curve_inputs;
typedef struct { uint32_t identity; uint8_t bank; float minimum,maximum; } nflstreet_curve_source;
typedef struct { uint32_t identity; uint8_t bank,duration,changed; } nflstreet_curve_decision;
int nflstreet_locomotion_curve_select(const nflstreet_curve_inputs *,
 const nflstreet_curve_source curves[3],const nflstreet_curve_decision *,nflstreet_curve_decision *);
/* Original 24-bit shortest separation and distinct pair/blend sign tests.
   Heading interpretation is deliberately left to the owning actor adapter. */
typedef struct {uint32_t separation;double pair_speed,blend_speed;} nflstreet_directional_speed;
int nflstreet_locomotion_direction(float,uint32_t,uint32_t,nflstreet_directional_speed *);
#endif
