#ifndef NFLSTREET_VARIANT_SELECTION_H
#define NFLSTREET_VARIANT_SELECTION_H
#include "locomotion_selection.h"
/* Neutral source fields; no football meanings are assigned to opaque values. */
typedef struct {
 uint8_t kind_byte,side_byte,status_byte,variant_byte,callback_tag;
 uint32_t selector_word,global_selector;
 unsigned available,predicate,special_predicate,special_word,special_enabled;
} nflstreet_variant_facts;
enum {NFLSTREET_VARIANT_PREDICATE=1,NFLSTREET_VARIANT_SPECIAL_PREDICATE=2,
 NFLSTREET_VARIANT_SPECIAL_WORD=4,NFLSTREET_VARIANT_SPECIAL_ENABLED=8};
typedef struct {uint8_t key,flag;} nflstreet_variant_choice;
/* Original 80062C30 decision. Absent lazy service facts reject atomically.
   Supplied predicate facts are not a reconstruction of their producers. */
int nflstreet_variant_first(const nflstreet_variant_facts *,uint8_t initial_flag,
                            nflstreet_variant_choice *);
/* Join calculated first key to the original hierarchy; second key is still
   an upstream input. No runtime RAM access or original module dependency. */
int nflstreet_variant_resolve(const nflstreet_asset_store *,uint16_t,
 const nflstreet_variant_facts *,uint8_t second_key,uint8_t initial_flag,
 uint8_t data[64],nflstreet_variant_choice *);
typedef struct {
 uint32_t selector_word,request_state,service_argument,heading;
 float speed;
 unsigned target_available,target_resolved,target_separation;float target_distance;
} nflstreet_request_facts;
/* Original 62834 control logic; target resolution/geometry are explicit facts. */
int nflstreet_variant_request(const nflstreet_request_facts *,unsigned *);
enum {NFLSTREET_SECOND_REQUEST=1,NFLSTREET_SECOND_PHASE=2,NFLSTREET_SECOND_ADDITIONAL=4,
 NFLSTREET_SECOND_ENDING=8,NFLSTREET_SECOND_INDEX=16,NFLSTREET_SECOND_VECTOR=32,NFLSTREET_SECOND_FOCUS=64};
typedef struct {
 unsigned same_actor,first_flag,request_state,available;
 unsigned request_predicate,phase_predicate,additional_predicate,ending_predicate,special_index;
 uint8_t aux_byte,control_byte,side_flag,old_key;
 uint32_t heading,vector_heading;
 float speed,x,y,focus_x,focus_y;
 const uint8_t *special_table;size_t table_size;
} nflstreet_variant_second_facts;
/* Original 62E2C decision with explicit lazy services. No producer side effects. */
int nflstreet_variant_second(const nflstreet_variant_facts *,const nflstreet_variant_second_facts *,uint8_t *);
int nflstreet_variant_resolve_pair(const nflstreet_asset_store *,uint16_t,
 const nflstreet_variant_facts *,const nflstreet_variant_second_facts *,uint8_t,
 uint8_t data[64],nflstreet_variant_choice *,uint8_t *);
typedef struct {float comparison,speed;uint32_t heading,global_flags;
 unsigned same_actor,service_argument,global_flags_available;uint8_t callback_tag;} nflstreet_phase_facts;
/* Bounded ordinary-callback portion of62950. Rejects target-search branches. */
int nflstreet_variant_phase_simple(const nflstreet_phase_facts *,unsigned *);
/* Direct source accessors; no caller side effects are reconstructed. */
int nflstreet_variant_additional_simple(uint8_t,unsigned *);
int nflstreet_variant_index(uint8_t,unsigned,uint8_t,uint8_t *);
#endif
