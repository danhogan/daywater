#ifndef NFLSTREET_MATCH_SESSION_H
#define NFLSTREET_MATCH_SESSION_H
#include <stdint.h>
/* Native boundaries of original801E577C, ordinary mode0 only. Other modes
   consume uninitialized stack contexts in the source and are not reconstructed. */
typedef enum {
    NFLSTREET_SESSION_RESOLVE,
    NFLSTREET_SESSION_CHECK_CALLBACK,
    NFLSTREET_SESSION_DELETE_A,
    NFLSTREET_SESSION_DELETE_B,
    NFLSTREET_SESSION_PREPARE_A,
    NFLSTREET_SESSION_PREPARE_B,
    NFLSTREET_SESSION_ASSIGN_ROSTERS,
    NFLSTREET_SESSION_BEGIN_CALLBACK,
    NFLSTREET_SESSION_STAGE_A,
    NFLSTREET_SESSION_STAGE_B,
    NFLSTREET_SESSION_STAGE_C,
    NFLSTREET_SESSION_FINALIZE
} nflstreet_match_session_kind;
typedef struct {
    uint8_t initialized;
    unsigned check_callback_present, begin_callback_present;
} nflstreet_match_session_owner;
typedef struct {
    nflstreet_match_session_kind kind;
    uint32_t sides[2], mode, argument, table_tag;
    const uint32_t (*formations)[14];
} nflstreet_match_session_request;
typedef struct {
    uint32_t sides[2];
    uint8_t continuation;
} nflstreet_match_session_response;
typedef int32_t (*nflstreet_match_session_service)(void *,
    const nflstreet_match_session_request *, nflstreet_match_session_response *);
typedef struct {
    uint32_t sides[2];
    uint8_t continuation;
    int32_t status;
    unsigned rosters_assigned, stages_completed;
} nflstreet_match_session_result;
/* Owner flag changes follow the original even on errors after table clearing.
   Continuation1 means the supplied downstream services accepted the sequence,
   not that actors or a playable match exist. Return0 rejects invalid API/mode. */
int nflstreet_match_session_run(uint32_t mode,uint32_t argument,
    const uint32_t formations[2][14],nflstreet_match_session_owner *,
    nflstreet_match_session_service,void *,nflstreet_match_session_result *);
#endif
