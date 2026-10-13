#ifndef NFLSTREET_MATCH_MODEL_INSTANCES_H
#define NFLSTREET_MATCH_MODEL_INSTANCES_H
#include "match_attachment_palettes.h"
#include "model_instance.h"
#define NFLSTREET_MATCH_MODEL_PARTS 6u
#define NFLSTREET_MATCH_INSTANCE_PART_BYTES (240u+52u+12u+256u*48u)
typedef struct {uint32_t base,materials;} nflstreet_match_instance_input;
typedef struct {uint8_t bytes[NFLSTREET_MATCH_INSTANCE_PART_BYTES];uint32_t token;unsigned count;} nflstreet_owned_model_instance;
typedef struct {nflstreet_owned_model_instance parts[14][6];} nflstreet_match_model_instances;
/* Body, face0/1, hair0/1/2; copy current views and serialize supplied matrices
   into owned BE palette bytes. Material tables are explicit borrowed tokens.
   Fresh zeroed instances; groups remain pending source selections. Commit all
   parts atomically. This is resource/draw-object assembly, not full actors. */
int nflstreet_match_model_instances_build(const nflstreet_match_visuals *,
 const nflstreet_match_body_palettes *,const nflstreet_match_attachment_palettes *,
 const nflstreet_match_instance_input[14],nflstreet_match_model_instances *);
uint8_t *nflstreet_match_model_instances_span(nflstreet_match_model_instances *,uint32_t,uint32_t);
#endif
