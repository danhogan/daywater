#include "startup.h"
#include <stdlib.h>
#include <string.h>
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void p32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(24-i*8));}
static int candidate(const nflstreet_archive_entry *e){
    /* Observed direct context-one header, not a scan for matching RAM bytes. */
    return !e->codec && e->size>=32 && e->bytes[0]==0 && e->bytes[1]==1 &&
        e->bytes[6]==0 && e->bytes[7]==11 && u32(e->bytes+8)==32;
}
int nflstreet_native_banks_open(const nflstreet_archive *a,nflstreet_native_banks *out){
    if(!a||!out)return 0;
    nflstreet_native_banks v={0};size_t total=0;
    for(unsigned i=0;i<a->count;++i){nflstreet_archive_entry e;if(!nflstreet_archive_get(a,i,&e))return 0;
        if(!candidate(&e))continue;
        if(v.count==256 || e.size>0x1000000 || total>0x10000000-e.size-32)return 0;
        v.entry_index[v.count]=i;v.bank_size[v.count++]=e.size;total+=(e.size+31)&~(size_t)31;
    }
    if(!v.count)return 0;
    v.arena=calloc(1,total);v.registry=calloc(v.count,28);
    if(!v.arena||!v.registry){nflstreet_native_banks_close(&v);return 0;}
    /* Opaque identities in this owned arena. They are not original RAM claims. */
    const uint32_t base=0x10000000;size_t at=0;
    for(unsigned i=0;i<v.count;++i){nflstreet_archive_entry e;
        if(!nflstreet_archive_get(a,v.entry_index[i],&e)){nflstreet_native_banks_close(&v);return 0;}
        memcpy(v.arena+at,e.bytes,e.size);
        if(!nflstreet_asset_relocate(v.arena+at,e.size,base+(uint32_t)at)){
            nflstreet_native_banks_close(&v);return 0;
        }
        uint8_t *r=v.registry+i*28;p32(r+12,base+(uint32_t)at);r[18]=1;r[21]=1;
        at+=(e.size+31)&~(size_t)31;
    }
    v.store=(nflstreet_asset_store){base,v.arena,v.registry,total,v.count*28};*out=v;return 1;
}
void nflstreet_native_banks_close(nflstreet_native_banks *v){
    if(v){free(v->arena);free(v->registry);memset(v,0,sizeof *v);}
}
