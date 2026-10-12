#ifndef NFLSTREET_LIVE_LOCOMOTION_H
#define NFLSTREET_LIVE_LOCOMOTION_H
#include <stdint.h>
#include <stddef.h>
#include "../actor_target_input/locomotion_selection.h"
/* Bounded native connection of source curves, paired streams and poses.
   NLC1 retains legacy fixed-primary inspection; NLC2 carries original opaque
   identities and permits the checked actor-input curve decision. Four slots,
   NSC1 catalogs permit checked inspection set changes. Complete gameplay
   eligibility remains open. Bank and initial profile memory must remain alive
   until the controller is no longer used, including reset. */
typedef struct {
 const uint8_t *bank;size_t bank_size;
 const uint8_t *initial_profile;size_t initial_profile_size;uint8_t set_keys[2];unsigned set_changes,pending_set;
 uint8_t seed_curve[1600],curve[1600],channels[496],descriptor[64];size_t curve_size;
 const uint8_t *program[4];size_t program_size[4];
 uint8_t source_curves[3][1600];size_t source_sizes[3];nflstreet_curve_source source[3];
 unsigned current_curve,curve_changes,source_decisions;
 unsigned direction_updates,negative_pair_updates,negative_blend_updates;
 uint32_t clips[2],flags;uint16_t pair;unsigned initialized,changes,updates;
 float rate;unsigned transition_duration,fade_updates,completed_fades;
} nflstreet_live_locomotion;
int nflstreet_live_locomotion_open(nflstreet_live_locomotion *,const uint8_t *,size_t,const uint8_t *,size_t);
void nflstreet_live_locomotion_reset(nflstreet_live_locomotion *);
int nflstreet_live_locomotion_step(nflstreet_live_locomotion *,double,double,int16_t *,size_t,unsigned *);
int nflstreet_live_locomotion_step_actor(nflstreet_live_locomotion *,const nflstreet_curve_inputs *,double,int16_t *,size_t,unsigned *);
int nflstreet_live_locomotion_step_direction(nflstreet_live_locomotion *,const nflstreet_curve_inputs *,uint32_t,uint32_t,double,int16_t *,size_t,unsigned *);
int nflstreet_live_locomotion_catalog_get(const uint8_t *,size_t,unsigned,const uint8_t **,size_t *);
int nflstreet_live_locomotion_step_set(nflstreet_live_locomotion *,const uint8_t *,size_t,const nflstreet_curve_inputs *,uint32_t,uint32_t,double,int16_t *,size_t,unsigned *);
#endif
