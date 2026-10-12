#include "match_start.h"
#include <string.h>

static int32_t call(nflstreet_match_start_service service, void *context,
    nflstreet_match_start_kind kind, uint32_t side, uint32_t index,
    uint32_t value, uint32_t side_context, uint32_t argument0, uint32_t argument1,
    const uint32_t formations[2][14], uint32_t *output) {
    nflstreet_match_start_request request = {kind,side,index,value,side_context,
        argument0,argument1,formations};
    return service(context,&request,output);
}
int nflstreet_match_start_run(const uint32_t formations[2][14],
    nflstreet_match_start_service service, void *context,
    nflstreet_match_start_result *result) {
    if (!formations || !service || !result) return 0;
    nflstreet_match_start_result owned = {0};
    memcpy(owned.formations,formations,sizeof owned.formations);
    uint32_t output = 0;
    (void)call(service,context,NFLSTREET_MATCH_REGISTER,0,0,0,0,0,0,NULL,&output);
    owned.status = call(service,context,NFLSTREET_MATCH_QUERY_MODE,0,0,0,0,0,0,NULL,&output);
    owned.mode = (uint8_t)output;
    if (owned.status) goto done;
    if (!owned.mode) {
        static const nflstreet_match_start_kind setup[] = {
            NFLSTREET_MATCH_SETUP_A,NFLSTREET_MATCH_SETUP_B,
            NFLSTREET_MATCH_SETUP_PAIR,NFLSTREET_MATCH_SETUP_FINAL};
        for (unsigned i=0;i<4;++i) {
            uint32_t first = i==0 || i==2 ? 0x07000000u : i==1 ? 0x04000000u : 0;
            uint32_t second = i==2 ? 0x03000000u : 0;
            owned.status = call(service,context,setup[i],0,0,0,0,first,second,NULL,&output);
            if (owned.status) goto done;
        }
        for (unsigned side=0;side<2;++side) {
            owned.status = call(service,context,NFLSTREET_MATCH_QUERY_SIDE,side,0,0,0,
                side,0,NULL,&owned.side_context[side]);
            if (owned.status) goto done;
        }
        for (unsigned side=0;side<2;++side) {
            owned.status = call(service,context,side ? NFLSTREET_MATCH_SET_SIDE_B :
                NFLSTREET_MATCH_SET_SIDE_A,side,0,owned.side_context[side],0,
                owned.side_context[side],0,NULL,&output);
            if (owned.status) goto done;
        }
        for (unsigned side=0;side<2;++side) {
            for (unsigned index=0;index<14;++index) {
                uint32_t *word=&owned.formations[side][index];
                (void)call(service,context,NFLSTREET_MATCH_QUERY_PLAYER,side,index,*word,
                    owned.side_context[side],0,0,NULL,word);
            }
        }
        owned.session_requested = 1;
        owned.status = call(service,context,NFLSTREET_MATCH_SESSION,0,0,owned.mode,0,
            owned.mode,0,(const uint32_t (*)[14])owned.formations,&output);
        if (owned.status) goto done;
    }
    owned.update_requested = 1;
    owned.status = call(service,context,NFLSTREET_MATCH_UPDATE,0,0,2,0,2,0,NULL,&output);
done:
    *result=owned;
    return 1;
}
