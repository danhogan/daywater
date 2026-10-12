#include "field_bootstrap.h"
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u * 1024u * 1024u)
#define RAM_BASE 0x80000000u
#define ORDER_ADDRESS 0x803a2c30u
#define KEY0_DESCRIPTION 0x80256e40u
static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
int nflstreet_field_bootstrap_from_dol(const uint8_t *dol, size_t size,
    const nflstreet_field_catalog_entry catalog[8], nflstreet_field_bootstrap *out) {
    if (!dol || !catalog || !out || size<0xe4 || size>16u*1024u*1024u) return 0;
    uint8_t *ram=calloc(1,RAM_SIZE);
    if (!ram) return 0;
    unsigned loaded=0, order_loaded=0, description_loaded=0;
    for (unsigned i=0;i<18;++i) {
        uint32_t offset=be32(dol+i*4), address=be32(dol+0x48+i*4), length=be32(dol+0x90+i*4);
        if (!length) continue;
        if (offset>size || length>size-offset || address<RAM_BASE ||
            (uint64_t)address-RAM_BASE+length>RAM_SIZE) { free(ram); return 0; }
        memcpy(ram+address-RAM_BASE,dol+offset,length); ++loaded;
        order_loaded |= address<=ORDER_ADDRESS && (uint64_t)ORDER_ADDRESS+8<=(uint64_t)address+length;
        description_loaded |= address<=KEY0_DESCRIPTION && (uint64_t)KEY0_DESCRIPTION+102<=(uint64_t)address+length;
    }
    if (!loaded || !order_loaded || !description_loaded) { free(ram); return 0; }
    const uint8_t *description=ram+KEY0_DESCRIPTION-RAM_BASE;
    const uint8_t *end=memchr(description,0,102);
    if (!end) { free(ram); return 0; }
    nflstreet_field_bootstrap next={0};
    unsigned seen=0;
    for (unsigned slot=0;slot<8;++slot) {
        unsigned key=ram[ORDER_ADDRESS-RAM_BASE+slot];
        if (key>=8 || (seen&(1u<<key)) || catalog[key].DIVE!=key ||
            !catalog[key].record[0xca]) { free(ram); return 0; }
        seen |= 1u<<key; next.keys[slot]=(uint8_t)key;
        uint8_t *record=next.records+slot*NFLSTREET_FIELD_CACHE_RECORD_SIZE;
        memcpy(record,catalog[key].record,NFLSTREET_FIELD_CACHE_RECORD_SIZE);
        if (!key) {
            memset(record+99,0,102);
            memcpy(record+99,description,(size_t)(end-description)+1);
        }
    }
    free(ram); memcpy(out,&next,sizeof next); return 1;
}
