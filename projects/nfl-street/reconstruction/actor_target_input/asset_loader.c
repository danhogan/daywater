#include "asset_loader.h"
#include "command_timing.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static uint16_t u16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void p16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void p32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
const uint8_t *nflstreet_asset_read(const nflstreet_asset_store *s,uint32_t id,size_t n){
    if(!s||!s->arena||id<s->base||n>s->arena_size||(size_t)(id-s->base)>s->arena_size-n)return NULL;
    return s->arena+(id-s->base);
}
static int valid(const nflstreet_asset_store *s){return s&&s->registry&&s->registry_size%28==0&&s->registry_size/28<=4096;}
int nflstreet_asset_bank(const nflstreet_asset_store *s,uint32_t key,const uint8_t **out){
    if(!valid(s)||!out)return 0;
    const uint8_t *b=NULL;
    if(key<s->registry_size/28){const uint8_t *r=s->registry+key*28;
        if(u16(r+20)!=0x8000){b=nflstreet_asset_read(s,u32(r+12),32);if(!b)return 0;}}
    *out=b;return 1;
}
int nflstreet_asset_bank_find(const nflstreet_asset_store *s,uint16_t context,uint16_t tag,uint32_t *out){
    if(!valid(s)||!out)return 0;
    for(size_t i=0;i<s->registry_size/28;++i){const uint8_t *r=s->registry+i*28;
        if(r[18]!=1||u16(r+20)!=context)continue;
        const uint8_t *b=nflstreet_asset_read(s,u32(r+12),32);if(!b)return 0;
        if(u16(b+2)==tag){*out=(uint32_t)i;return 1;}}
    *out=0xffffff;return 1;
}
static int groups(const nflstreet_asset_store *s,const uint8_t *b,const uint8_t **out,size_t *size){
    const uint8_t *g=nflstreet_asset_read(s,u32(b+8),4);if(!g)return 0;
    unsigned n=u32(g);if(n>512)return 0;
    *size=4+(size_t)n*16;*out=nflstreet_asset_read(s,u32(b+8),*size);return *out!=NULL;
}
static int group_index(const uint8_t *g,size_t size,uint16_t key){
    for(size_t i=4;i+16<=size;i+=16)if(u16(g+i)==key)return (int)((i-4)/16);
    return -1;
}
int nflstreet_asset_metadata(const nflstreet_asset_store *s,uint16_t context,uint16_t key,uint32_t *out){
    uint32_t result=0;unsigned matches=0;
    if(!valid(s)||!out)return 0;
    for(size_t i=0;i<s->registry_size/28;++i){const uint8_t *r=s->registry+i*28;
        if(r[18]!=1||u16(r+20)!=context)continue;
        const uint8_t *b,*g;size_t size;if(!nflstreet_asset_bank(s,(uint32_t)i,&b)||!b||!groups(s,b,&g,&size))return 0;
        int j=group_index(g,size,key);if(j>=0&&matches<32){result=u32(g+12+(size_t)j*16);++matches;}}
    *out=result;return 1;
}
static int table(const nflstreet_asset_store *s,uint16_t context,uint16_t key,int transition,uint8_t out[520]){
    unsigned count=0;memset(out,0,520);p16(out+2,transition?8:0x17c);p16(out+4,key);
    if(!valid(s))return 0;
    unsigned matches=0;
    for(size_t i=0;i<s->registry_size/28;++i){const uint8_t *r=s->registry+i*28;
        if(r[18]!=1||u16(r+20)!=context)continue;
        const uint8_t *b,*g;size_t size;if(!nflstreet_asset_bank(s,(uint32_t)i,&b)||!b||!groups(s,b,&g,&size))return 0;
        int j=group_index(g,size,key);if(j<0||matches++>=32)continue;
        const uint8_t *row=g+4+(size_t)j*16;
        if(transition){uint32_t id=u32(row+12);const uint8_t *blob;
            if(!id)continue;
            if(!(blob=nflstreet_asset_read(s,id,4)))return 0;
            if(!u32(blob)||count>=8)continue;
            p16(out+8+count*8,(uint16_t)i);p16(out+10+count*8,u16(b+2));p32(out+12+count*8,id);++count;
        }else{uint32_t id=u32(row+4);const uint8_t *refs=nflstreet_asset_read(s,id,4);if(!refs)return 0;
            unsigned n=u16(refs);if(n>64||!(refs=nflstreet_asset_read(s,id,4+(size_t)n*8)))return 0;
            for(unsigned k=0;k<n;++k){if(count>=64)return 0;
                p16(out+4+count*8,(uint16_t)i);p16(out+6+count*8,u16(refs+4+k*8));p32(out+8+count*8,u32(refs+8+k*8));++count;}}
    }
    p16(out,(uint16_t)count);return 1;
}
int nflstreet_asset_group_rows(const nflstreet_asset_store *s,uint16_t context,
                              uint16_t group,uint8_t out[520]){
    uint8_t candidate[520];
    if(!out||!table(s,context,group,0,candidate))return 0;
    memcpy(out,candidate,sizeof candidate);return 1;
}
int nflstreet_asset_sources(const nflstreet_asset_store *s,uint16_t bank_key,uint16_t clip,uint16_t context,
                           uint16_t group,uint16_t old_group,nflstreet_loaded_animation *out){
    const uint8_t *b,*p;size_t n;uint32_t identity;
    if(!out||!valid(s)||!nflstreet_asset_bank(s,bank_key,&b)||!b)return 0;
    memset(out,0,sizeof *out);nflstreet_animation_sources *v=&out->source;v->store=s;v->bank_key=bank_key;v->bank=b;v->side_source=-1;
    if(!groups(s,b,&v->groups,&v->groups_size))return 0;
    p=nflstreet_asset_read(s,u32(b+12),4);if(!p||u32(p)>4096)return 0;
    n=4+(size_t)u32(p)*4;v->program_table=nflstreet_asset_read(s,u32(b+12),n);v->program_table_size=n;
    if(!v->program_table||clip>=u32(p)||!nflstreet_stream_program_lookup(v->program_table,n,clip,&identity))return 0;
    v->program_identity=identity;v->program=nflstreet_asset_read(s,identity,4096);v->program_size=4096;if(!v->program)return 0;
    if(!table(s,context,group,0,out->metadata_table)||!table(s,context,old_group,1,out->transition_table))return 0;
    v->metadata_assets=out->metadata;v->metadata_count=u16(out->metadata_table);
    for(size_t i=0;i<v->metadata_count;++i){uint16_t key=(uint16_t)(u16(out->metadata_table+6+i*8)&0x7fff);
        if(!nflstreet_asset_metadata(s,context,key,&identity)||!identity)return 0;
        p=nflstreet_asset_read(s,identity,8);if(!p||u16(p+4)>127)return 0;n=8+(size_t)u16(p+4)*20;
        out->metadata[i]=(nflstreet_animation_blob){identity,nflstreet_asset_read(s,identity,n),n};if(!out->metadata[i].data)return 0;}
    v->transition_table=out->transition_table;v->transition_count=u16(out->transition_table);v->transition_table_size=8+v->transition_count*8;
    v->transition_assets=out->transitions;v->transition_targets=out->targets;
    for(size_t i=0;i<v->transition_count;++i){identity=u32(out->transition_table+12+i*8);p=nflstreet_asset_read(s,identity,4);
        if(!p||u32(p)>256)return 0;
        n=4+(size_t)u32(p)*16;p=nflstreet_asset_read(s,identity,n);if(!p)return 0;
        out->transitions[i]=(nflstreet_animation_blob){identity,p,n};
        for(size_t j=0;j<u32(p);++j){identity=u32(p+16+j*16);if(!identity)continue;
            const uint8_t *t=nflstreet_asset_read(s,identity,16);if(!t||v->target_count==16384)return 0;
            out->targets[v->target_count++]=(nflstreet_animation_blob){identity,t,16};}}
    v->registry=out->registry;v->registry_count=s->registry_size/28;
    for(size_t i=0;i<v->registry_count;++i){const uint8_t *r=s->registry+i*28;out->registry[i].registry=r;out->registry[i].registry_size=28;
        if(r[18]!=1||u16(r+20)!=context)continue;
        if(!nflstreet_asset_bank(s,(uint32_t)i,&b)||!b||!groups(s,b,&out->registry[i].groups,&out->registry[i].groups_size))return 0;
        out->registry[i].bank=b;}
    return 1;
}
/* Original 0x801770FC: initialize two tags then count interval steps through
   the terminator. Control arguments are skipped, not executed in this scan. */
int nflstreet_animation_program_duration(const uint8_t *program,size_t size,float *out){
    if(!program||!out||size<6)return 0;
    uint8_t state[64]={0};uint32_t flags=0;uint16_t emitted[64];size_t count;unsigned done;
    if(!nflstreet_stream_setup(state,1,program,size,&flags,emitted,&count,&done))return 0;
    uint32_t bits=u32(state+12);float interval;memcpy(&interval,&bits,4);
    bits=u32(state+8);float duration;memcpy(&duration,&bits,4);duration+=interval+interval;
    size_t cursor=u16(state+24);
    for(unsigned steps=0;steps<65536;++steps){if(4+cursor*2+2>size)return 0;
        uint16_t w=u16(program+4+cursor++*2);
        if(!(w&0x8000)){duration+=interval;continue;}
        if(w==0x8000){duration-=interval;*out=duration;return isfinite(duration);}
        if(w==0x8001){*out=duration;return isfinite(duration);}
        if((w&0xe000)==0xa000&&(w&0x1000))++cursor;
    }
    return 0;
}
static uint8_t *at(uint8_t *p,size_t n,uint32_t offset,size_t length){return length<=n&&offset<=n-length?p+offset:NULL;}
static int pointer(uint8_t *p,size_t n,uint8_t *field,uint32_t base,int nullable){
    uint32_t offset=u32(field);if(nullable&&!offset)return 1;
    if(!at(p,n,offset,1)||offset>UINT32_MAX-base)return 0;
    p32(field,base+offset);return 1;
}
int nflstreet_asset_relocate(uint8_t *bank,size_t size,uint32_t base){
    if(!bank||size<32||size>0x1000000||size>UINT32_MAX-base)return 0;
    uint16_t flags=u16(bank+6);if(flags&0x1000)return 1;
    if(flags&4)return 0;
    uint8_t *p=malloc(size);if(!p)return 0;memcpy(p,bank,size);int ok=0;
    uint32_t go=u32(p+8),po=u32(p+12),to=u32(p+16);
    if(!pointer(p,size,p+8,base,0)||!pointer(p,size,p+12,base,0)||!pointer(p,size,p+16,base,0))goto end;
    if(flags&8){if(!pointer(p,size,p+24,base,0)||!pointer(p,size,p+28,base,0))goto end;}
    uint8_t *table=at(p,size,to,(size_t)u16(p+4)*4);if(!table)goto end;
    /* Frame offsets can refer to payload beyond the resident header block.
       Original adds the bank base without dereferencing these identities. */
    for(unsigned i=0;i<u16(p+4);++i)p32(table+i*4,base+u32(table+i*4));
    uint8_t *g=at(p,size,go,4);if(!g||u32(g)>512)goto end;
    unsigned rows=u32(g);g=at(p,size,go,4+(size_t)rows*16);if(!g)goto end;
    for(unsigned i=0;i<rows;++i){uint8_t *row=g+4+i*16;uint32_t refs=u32(row+4),trans=u32(row+12);
        if(!pointer(p,size,row+4,base,0))goto end;
        uint8_t *ref=at(p,size,refs,4);if(!ref)goto end;
        if(!(flags&8)){uint16_t a=u16(ref),b=u16(ref+2);p16(ref,b);p16(ref+2,a);}
        unsigned count=u16(ref);if(count>512||!(ref=at(p,size,refs,4+(size_t)count*8)))goto end;
        for(unsigned j=0;j<count;++j)if(!pointer(p,size,ref+8+j*8,base,1))goto end;
        if(!pointer(p,size,row+8,base,1)||!pointer(p,size,row+12,base,0))goto end;
        uint8_t *t=at(p,size,trans,4);if(!t||u32(t)>256)goto end;count=u32(t);
        if(!(t=at(p,size,trans,4+(size_t)count*16)))goto end;
        for(unsigned j=0;j<count;++j)if(!pointer(p,size,t+16+j*16,base,1))goto end;
    }
    uint8_t *programs=at(p,size,po,4);if(!programs||u32(programs)>4096)goto end;
    unsigned count=u32(programs);programs=at(p,size,po,4+(size_t)count*4);if(!programs)goto end;
    for(unsigned j=0;j<count;++j)if(!pointer(p,size,programs+4+j*4,base,0))goto end;
    p16(p+6,(uint16_t)(flags|0x1000));memcpy(bank,p,size);ok=1;
end:free(p);return ok;
}
