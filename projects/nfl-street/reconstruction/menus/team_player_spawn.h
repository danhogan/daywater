#ifndef NFLSTREET_TEAM_PLAYER_SPAWN_H
#define NFLSTREET_TEAM_PLAYER_SPAWN_H
#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_TEAM_PLAYER_MODEL_NAME_BYTES 27u

/* Bounded input/output description for the original 80128F90 request path.
   The numeric members retain their original PLAY field tags; the model does
   not infer body-part semantics from those tag names. */
typedef struct {
 uint32_t team_id, player_id;
 uint32_t digt, iksp, cfrp, tbyp, tghp, ytrp;
 char model_name[NFLSTREET_TEAM_PLAYER_MODEL_NAME_BYTES];
 uint8_t side_label;
 uint32_t actor_index, callback_argument;
 uint32_t factory_enabled;
} nflstreet_team_player_spawn_request;

/* Resolve exactly one PLAY row by (DIGT,DIGP), preserve the values read by
   80128F90's getters, and form its factory-call arguments. The original
   callback receives one indexed byte from the original DOL table at 80282CFC, supplied by the owner.
   The factory is an explicit supplied gate; no renderer/model I/O is run. */
int nflstreet_team_player_spawn_from_db(const uint8_t *,size_t,
 uint32_t team_id,uint32_t player_id,uint32_t actor_index,
 uint32_t callback_argument,uint32_t factory_enabled,uint8_t side_label,
 nflstreet_team_player_spawn_request *);

#endif
