#include "quick_match_selection.h"
#include <string.h>
int nflstreet_quick_match_selection_build(const uint32_t team_ids[2],
    const nflstreet_team_activation *teams, const nflstreet_field_bootstrap *fields,
    unsigned slot, nflstreet_quick_match_selection *out) {
    if(!team_ids||!teams||!fields||!out||slot>=8||fields->keys[slot]>=8||
       !fields->records[slot*NFLSTREET_FIELD_CACHE_RECORD_SIZE+0xca]||
       fields->records[slot*NFLSTREET_FIELD_CACHE_RECORD_SIZE+0xc9])return 0;
    nflstreet_quick_match_selection next={0};
    for(unsigned side=0;side<2;++side){
        const nflstreet_team_activation_side *a=&teams->sides[side];
        if(team_ids[side]==UINT32_MAX||a->selected_count!=7||a->formation_count!=7)return 0;
        for(unsigned player=0;player<7;++player){
            uint32_t id=a->selected_ids[player];if(id==0x7fff||id==UINT32_MAX)return 0;
            for(unsigned prior=0;prior<player;++prior)if(a->selected_ids[prior]==id)return 0;
        }
        for(unsigned half=0;half<2;++half){
            unsigned mask=0;
            for(unsigned position=0;position<7;++position){
                uint32_t id=a->active_formation[half*7+position];unsigned player=0;
                while(player<7&&a->selected_ids[player]!=id)++player;
                if(player==7||(mask&(1u<<player)))return 0;
                mask|=1u<<player;
            }
        }
        next.team_ids[side]=team_ids[side];
        memcpy(next.selected_players[side],a->selected_ids,sizeof next.selected_players[side]);
        memcpy(next.formation_players[side],a->active_formation,sizeof next.formation_players[side]);
    }
    next.field_slot=slot;next.field_key=fields->keys[slot];
    memcpy(out,&next,sizeof next);return 1;
}
