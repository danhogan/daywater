#include "pair_requests.h"
#include <string.h>
static uint16_t word(const uint8_t *p) { return (uint16_t)((unsigned)p[0]<<8 | p[1]); }
int nflstreet_pair_requests(const uint8_t *asset,size_t asset_size,
                            const uint8_t params[60],uint16_t choice,
                            uint8_t current[16],uint8_t incoming[16])
{
    uint8_t c[16]={0},n[16]={0}; const uint8_t *rows[2]={0,0};
    unsigned count,i;
    if (!asset || !params || !current || !incoming || asset_size<8) return 0;
    count=word(asset+4);
    if (!count || count>256 || 8+(size_t)count*20>asset_size) return 0;
    for(i=0;i<count;++i) {
        const uint8_t *entry=asset+8+i*20;
        unsigned type=entry[2]!=0;
        if (!rows[type]) rows[type]=entry;
    }
    if (!rows[0] || !rows[1]) return 0;
    memcpy(c,params+0x24,8); memcpy(n,c,8);
    memcpy(c+8,params+0x30,2); memcpy(n+8,c+8,2);
    memcpy(c+10,rows[1],2); memcpy(n+10,rows[0],2);
    c[12]=n[12]=(uint8_t)(choice>>8); c[13]=n[13]=(uint8_t)choice;
    memcpy(current,c,16); memcpy(incoming,n,16); return 1;
}
