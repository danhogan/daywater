#include "match_session.h"

static int32_t call(nflstreet_match_session_service service,void *context,
    nflstreet_match_session_kind kind,uint32_t argument,const uint32_t sides[2],
    const uint32_t formations[2][14],nflstreet_match_session_response *response) {
    nflstreet_match_session_request request={kind,{sides[0],sides[1]},0,argument,
        kind==NFLSTREET_SESSION_ASSIGN_ROSTERS ? 0x41474344u : 0,formations};
    return service(context,&request,response);
}
int nflstreet_match_session_run(uint32_t mode,uint32_t argument,
    const uint32_t formations[2][14],nflstreet_match_session_owner *owner,
    nflstreet_match_session_service service,void *context,
    nflstreet_match_session_result *result) {
    if(mode || !formations || !owner || !service || !result) return 0;
    nflstreet_match_session_result next={{0,0},1,0,0,0};
    nflstreet_match_session_response response={{0,0},1};
    next.status=call(service,context,NFLSTREET_SESSION_RESOLVE,argument,next.sides,
        formations,&response);
    next.sides[0]=response.sides[0];next.sides[1]=response.sides[1];
    if(next.status) goto error;
    if(owner->initialized==1 && owner->check_callback_present) {
        response.continuation=next.continuation;
        next.status=call(service,context,NFLSTREET_SESSION_CHECK_CALLBACK,argument,
            next.sides,formations,&response);
        next.continuation=response.continuation;
        if(next.status) goto error;
    }
    if(next.continuation==1) {
        static const nflstreet_match_session_kind prepare[]={
            NFLSTREET_SESSION_DELETE_A,NFLSTREET_SESSION_DELETE_B,
            NFLSTREET_SESSION_PREPARE_A,NFLSTREET_SESSION_PREPARE_B};
        for(unsigned i=0;i<4;++i) {
            next.status=call(service,context,prepare[i],argument,next.sides,
                formations,&response);
            if(next.status) break;
        }
        /* Original801E5868 executes this even after a preparation error. */
        (void)call(service,context,NFLSTREET_SESSION_ASSIGN_ROSTERS,argument,
            next.sides,formations,&response);
        next.rosters_assigned=1;
        owner->initialized=1;
        if(next.status) goto error;
    }
    if(next.continuation==1) {
        if(owner->begin_callback_present) {
            next.status=call(service,context,NFLSTREET_SESSION_BEGIN_CALLBACK,argument,
                next.sides,formations,&response);
            if(next.status) goto error;
        }
        static const nflstreet_match_session_kind stages[]={
            NFLSTREET_SESSION_STAGE_A,NFLSTREET_SESSION_STAGE_B,
            NFLSTREET_SESSION_STAGE_C,NFLSTREET_SESSION_FINALIZE};
        for(unsigned i=0;i<4;++i) {
            next.status=call(service,context,stages[i],argument,next.sides,
                formations,&response);
            if(next.status) goto error;
            ++next.stages_completed;
        }
    }
    *result=next;
    return 1;
error:
    next.continuation=0;
    *result=next;
    return 1;
}
