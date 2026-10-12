#include "match_player_query.h"
#include "team_catalog.h"

static int field(const nflstreet_team_table *table,uint32_t tag,unsigned *index) {
    unsigned count=0;
    for (unsigned i=0;i<table->field_count;++i) {
        nflstreet_team_field f;
        if (!nflstreet_team_table_field(table,i,&f)) return 0;
        if (f.tag!=tag) continue;
        if (++count!=1 || f.type!=3 || !f.width || f.width>32) return 0;
        *index=i;
    }
    return count==1;
}
int nflstreet_match_player_query(const uint8_t *bytes,size_t size,
    uint32_t side_context,uint32_t input_word,uint32_t *player_id) {
    if (!bytes || !player_id) return 0;
    nflstreet_team_table table;
    unsigned team=0,target=0,id=0;
    if (!nflstreet_team_table_open_tag(bytes,size,0x59414c50u,&table) ||
        !field(&table,0x44494754u,&team) ||
        !field(&table,0x44494f50u,&target) ||
        !field(&table,0x44494750u,&id)) return 0;
    unsigned matches=0;
    uint32_t found=0;
    for (unsigned row=0;row<table.used;++row) {
        uint32_t context=0,word=0;
        if (!nflstreet_team_table_number(&table,row,team,&context) ||
            !nflstreet_team_table_number(&table,row,target,&word)) return 0;
        if (context!=side_context || word!=input_word) continue;
        if (++matches!=1 || !nflstreet_team_table_number(&table,row,id,&found)) return 0;
    }
    if (matches!=1) return 0;
    *player_id=found;
    return 1;
}
