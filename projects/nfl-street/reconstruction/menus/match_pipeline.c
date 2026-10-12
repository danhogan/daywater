#include "match_pipeline.h"
#include <string.h>

typedef struct {
    const nflstreet_match_pipeline_services *services;
    nflstreet_match_session_owner *owner;
    nflstreet_match_pipeline_result result;
    unsigned invalid;
} pipeline;
static int32_t session_service(void *context,
    const nflstreet_match_session_request *request,
    nflstreet_match_session_response *response) {
    pipeline *p=context;
    if(request->kind!=NFLSTREET_SESSION_RESOLVE)
        return p->services->session(p->services->session_context,request,response);
    ++p->result.settings_called;
    if(!nflstreet_match_settings_resolve(request->argument,p->services->settings,
        p->services->settings_context,&p->result.settings)) {
        p->invalid=1;
        return 0;
    }
    memcpy(response->sides,p->result.settings.side_context,sizeof response->sides);
    return p->result.settings.status;
}
static int32_t start_service(void *context,
    const nflstreet_match_start_request *request,uint32_t *output) {
    pipeline *p=context;
    if(request->kind!=NFLSTREET_MATCH_SESSION)
        return p->services->start(p->services->start_context,request,output);
    ++p->result.session_called;
    if(!nflstreet_match_session_run(request->argument0,request->argument1,
        request->formations,p->owner,session_service,p,&p->result.session)) {
        p->invalid=1;
        return 0;
    }
    return p->result.session.status;
}
int nflstreet_match_pipeline_run(const uint32_t formations[2][14],
    nflstreet_match_session_owner *owner,const nflstreet_match_pipeline_services *services,
    nflstreet_match_pipeline_result *result) {
    if(!formations || !owner || !services || !services->start ||
        !services->settings || !services->session || !result) return 0;
    pipeline p={0};p.services=services;p.owner=owner;
    if(!nflstreet_match_start_run(formations,start_service,&p,&p.result.start) ||
        p.invalid) return 0;
    *result=p.result;
    return 1;
}
