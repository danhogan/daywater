#include "field_catalog.h"
#include "team_catalog.h"

#include <stdint.h>
#include <string.h>

#define RVNE_TAG 0x52564e45u
#define DIVE_TAG 0x44495645u
#define LTVE_TAG 0x4c545645u
#define ROVE_TAG 0x524f5645u
#define SDVE_TAG 0x53445645u
#define TOVE_TAG 0x544f5645u
#define MNVE_TAG 0x4d4e5645u
#define OLVE_TAG 0x4f4c5645u
#define EDVE_TAG 0x45445645u

static int field_index(const nflstreet_team_table *table, uint32_t tag, unsigned *out) {
    unsigned matches = 0, found = 0;
    for (unsigned i = 0; i < table->field_count; ++i) {
        nflstreet_team_field f;
        if (!nflstreet_team_table_field(table, i, &f)) return 0;
        if (f.tag == tag) { found = i; ++matches; }
    }
    if (matches != 1) return 0;
    *out = found;
    return 1;
}

static int required_numeric(const nflstreet_team_table *table, unsigned row,
                            unsigned ix, uint32_t *out) {
    nflstreet_team_field f;
    return nflstreet_team_table_field(table, ix, &f) && f.type == 3 &&
           nflstreet_team_table_number(table, row, ix, out);
}

static int required_text(const nflstreet_team_table *table, unsigned row,
                         unsigned ix, char *out, size_t cap) {
    nflstreet_team_field f;
    return nflstreet_team_table_field(table, ix, &f) && f.type == 0 &&
           nflstreet_team_table_text(table, row, ix, out, cap);
}

int nflstreet_field_catalog_from_template(const uint8_t *bytes, size_t size,
                                          const char text42[8][33],
                                          const uint8_t value_c9[8],
                                          nflstreet_field_catalog_entry output[8]) {
    if (!bytes || !text42 || !value_c9 || !output) return 0;
    for (unsigned i = 0; i < 8; ++i)
        if (!memchr(text42[i], 0, 33)) return 0;

    nflstreet_team_table table;
    if (!nflstreet_team_table_open_tag(bytes, size, RVNE_TAG, &table) || table.used != 8)
        return 0;
    unsigned dive_ix, ltve_ix, rove_ix, sdve_ix, tove_ix;
    unsigned mnve_ix,olve_ix,edve_ix;
    if (!field_index(&table,DIVE_TAG,&dive_ix) || !field_index(&table,LTVE_TAG,&ltve_ix) ||
        !field_index(&table,ROVE_TAG,&rove_ix) || !field_index(&table,SDVE_TAG,&sdve_ix) ||
        !field_index(&table,TOVE_TAG,&tove_ix) || !field_index(&table,MNVE_TAG,&mnve_ix) ||
        !field_index(&table,OLVE_TAG,&olve_ix) || !field_index(&table,EDVE_TAG,&edve_ix))
        return 0;

    nflstreet_field_catalog_entry next[8];
    memset(next,0,sizeof next);
    unsigned seen=0;
    for (unsigned row=0;row<table.used;++row) {
        uint32_t key, ltve, rove, sdve, tove;
        if (!required_numeric(&table,row,dive_ix,&key) || key>=8 || (seen&(1u<<key)) ||
            !required_numeric(&table,row,ltve_ix,&ltve) ||
            !required_numeric(&table,row,rove_ix,&rove) ||
            !required_numeric(&table,row,sdve_ix,&sdve) ||
            !required_numeric(&table,row,tove_ix,&tove)) return 0;
        nflstreet_field_catalog_entry *e=&next[key];
        e->DIVE=key;e->LTVE=ltve;e->ROVE=rove;e->SDVE=sdve;e->TOVE=tove;
        if (!required_text(&table,row,mnve_ix,(char *)e->record,33) ||
            !required_text(&table,row,olve_ix,(char *)e->record+0x21,33) ||
            !required_text(&table,row,edve_ix,(char *)e->record+99,102)) return 0;
        memcpy(e->record+0x42,text42[key],33);
        e->record[0xc9]=value_c9[key];
        e->record[0xca]=1;
        seen|=1u<<key;
    }
    if (seen!=0xffu) return 0;
    memcpy(output,next,sizeof next);
    return 1;
}
