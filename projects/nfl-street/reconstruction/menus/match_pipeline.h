#ifndef NFLSTREET_MATCH_PIPELINE_H
#define NFLSTREET_MATCH_PIPELINE_H
#include "match_start.h"
#include "match_session.h"
#include "match_settings.h"

typedef struct {
    nflstreet_match_start_service start;
    void *start_context;
    nflstreet_match_settings_query_fn settings;
    void *settings_context;
    nflstreet_match_session_service session;
    void *session_context;
} nflstreet_match_pipeline_services;
typedef struct {
    nflstreet_match_start_result start;
    nflstreet_match_settings_result settings;
    nflstreet_match_session_result session;
    unsigned settings_called, session_called;
} nflstreet_match_pipeline_result;
/* Connects original caller -> session -> owned normalized settings. External
   owners handle remaining queries, resource I/O, record insertion and stages.
   No successful defaults are supplied for missing services. Return1 means
   API execution, not a playable match; examine original status/continuation. */
int nflstreet_match_pipeline_run(const uint32_t formations[2][14],
    nflstreet_match_session_owner *,const nflstreet_match_pipeline_services *,
    nflstreet_match_pipeline_result *);
#endif
