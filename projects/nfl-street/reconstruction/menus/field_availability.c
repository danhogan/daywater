#include "field_availability.h"
#include "team_catalog.h"

#include <string.h>

#define IVID_TAG 0x49564944u
#define OCID_TAG 0x4f434944u
#define DIGD_TAG 0x44494744u
#define IFID_TAG 0x49464944u

static int unique_field(const nflstreet_team_table *t, uint32_t tag, unsigned *ix) {
    unsigned n=0, found=0;
    for (unsigned i=0; i<t->field_count; ++i) {
        nflstreet_team_field f;
        if (!nflstreet_team_table_field(t,i,&f)) return 0;
        if (f.tag==tag) { found=i; ++n; }
    }
    if (n!=1) return 0;
    *ix=found;
    return 1;
}

static int number(const nflstreet_team_table *t, unsigned row, unsigned ix, uint32_t *v) {
    nflstreet_team_field f;
    return nflstreet_team_table_field(t,ix,&f) && f.type==3 &&
           nflstreet_team_table_number(t,row,ix,v);
}

int nflstreet_field_availability_from_ivid(const uint8_t *bytes, size_t size,
                                           int8_t raw_field_id, uint8_t *c9,
                                           uint8_t ifid_by_ocid[2]) {
    if (!bytes || !c9 || !ifid_by_ocid) return 0;
    nflstreet_team_table t;
    if (!nflstreet_team_table_open_tag(bytes,size,IVID_TAG,&t)) return 0;
    unsigned ocid_ix,digd_ix,ifid_ix;
    if (!unique_field(&t,OCID_TAG,&ocid_ix) || !unique_field(&t,DIGD_TAG,&digd_ix) ||
        !unique_field(&t,IFID_TAG,&ifid_ix)) return 0;
    uint32_t digd_by_ocid[2]={0,0}, ifid_by_digd[256]={0};
    unsigned seen_ocid=0, seen_digd[256]={0};
    for (unsigned row=0; row<t.used; ++row) {
        uint32_t ocid,digd,ifid;
        if (!number(&t,row,ocid_ix,&ocid) || !number(&t,row,digd_ix,&digd) ||
            !number(&t,row,ifid_ix,&ifid) || digd>255u || ifid>127u) return 0;
        if (ocid<2) {
            if (seen_ocid&(1u<<ocid)) return 0;
            digd_by_ocid[ocid]=digd;
            seen_ocid|=1u<<ocid;
        }
        if (seen_digd[digd]) return 0;
        seen_digd[digd]=1;
        ifid_by_digd[digd]=ifid;
    }
    if (seen_ocid!=3u || !seen_digd[digd_by_ocid[0]] || !seen_digd[digd_by_ocid[1]]) return 0;
    uint32_t result[2]={ifid_by_digd[digd_by_ocid[0]],ifid_by_digd[digd_by_ocid[1]]};
    uint8_t predicate=(uint8_t)((int32_t)raw_field_id==(int32_t)result[0] ||
                                (int32_t)raw_field_id==(int32_t)result[1]);
    *c9=(uint8_t)!predicate;
    ifid_by_ocid[0]=(uint8_t)result[0];
    ifid_by_ocid[1]=(uint8_t)result[1];
    return 1;
}
