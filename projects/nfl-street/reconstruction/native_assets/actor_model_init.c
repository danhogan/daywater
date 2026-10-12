#include "actor_model_init.h"
#include <string.h>
static uint16_t read16(const uint8_t*p){return(uint16_t)((uint16_t)p[0]<<8|p[1]);}
static uint32_t read32(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void write16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void write32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static int valid(const nflstreet_actor_model_resource*r,unsigned minimum){
    if(!r->token)return !r->bytes&&!r->size;
    if(!r->bytes||r->size<8)return 0;unsigned n=read16(r->bytes+6);
    return n<=64&&r->size>=0x410u+6u*(n>minimum?n:minimum);
}
static int allocate(nflstreet_actor_model_allocator fn,void*ctx,
    nflstreet_actor_model_allocation_kind kind,unsigned count,nflstreet_actor_model_memory*out){
    nflstreet_actor_model_allocation q={kind,kind==NFLSTREET_ACTOR_TRANSFORMS?count*64u:384u,count};
    memset(out,0,sizeof *out);return fn(ctx,&q,out)&&out->token&&out->bytes&&out->size>=q.bytes;
}
static void copy_channel(const nflstreet_actor_model_resource*r,nflstreet_actor_model_memory*m){
    unsigned n=read16(r->bytes+6);
    for(unsigned i=0;i<n*3;++i)write16(m->bytes+i*2,(uint16_t)(read16(r->bytes+0x410+i*2)<<4));
}
static int channel(uint8_t*a,unsigned offset,const nflstreet_actor_model_resource*r,unsigned extra,
    nflstreet_actor_model_allocator fn,void*ctx,nflstreet_actor_model_memory*m){
    unsigned n=read16(r->bytes+6);write16(a+offset,0x100);write16(a+offset+2,0x202);write16(a+offset+4,(uint16_t)(n+extra));
    if(!allocate(fn,ctx,NFLSTREET_ACTOR_CHANNEL,n,m))return 0;
    write32(a+offset+0x30,m->token);copy_channel(r,m);return 1;
}
static uint32_t shifted_position(uint16_t word){return(uint32_t)((int32_t)(int16_t)word*256);}
int nflstreet_actor_model_initialize(uint8_t*a,size_t size,const nflstreet_actor_model_descriptor*d,
    nflstreet_actor_model_allocator fn,void*ctx){
    if(!a||size<NFLSTREET_ACTOR_MODEL_STORAGE||!d||!fn||!valid(&d->skeleton,0)||!valid(&d->composite,19))return 0;
    for(unsigned i=0;i<2;++i)if(!valid(d->primary+i,0)||!valid(d->secondary+i,2))return 0;
    write32(a+0x14,read32(a+0x14)|1u);write32(a+0x18,d->initial_word_bits);write32(a+0x1c,d->initial_word_bits);write32(a+0x20,0);
    nflstreet_actor_model_memory memory,transforms;
    if(d->skeleton.token){unsigned n=read16(d->skeleton.bytes+6);write32(a+0x64,d->skeleton.token);
        if(!allocate(fn,ctx,NFLSTREET_ACTOR_TRANSFORMS,n,&transforms))return 0;write32(a+0x74,transforms.token);
        if(!channel(a,0x2c,&d->skeleton,1,fn,ctx,&memory))return 0;write32(a+0x60,transforms.token);}
    for(unsigned i=0;i<2;++i){unsigned off=0x78+i*0x48;const nflstreet_actor_model_resource*r=d->primary+i;
        memset(a+off,0,0x48);if(r->token){a[off]=1;a[off+1]=a[0];a[off+6]=1;write32(a+off+0x44,r->token);
            if(!channel(a,off+0xc,r,0,fn,ctx,&memory))return 0;write32(a+off+0x40,0);}}
    for(unsigned i=0;i<2;++i){unsigned off=0x11c+i*0x7c;const nflstreet_actor_model_resource*r=d->secondary+i;
        if(!r->token)continue;memset(a+off,0,0x7c);write32(a+off+0x78,r->token);a[off+1]=i?0xe:0x14;
        if(!channel(a,off+0x40,r,0,fn,ctx,&memory))return 0;write32(a+off+0x74,0);a[off]=1;
        for(unsigned axis=0;axis<3;++axis){uint16_t v=read16(r->bytes+0x416+2*axis);write32(a+off+0x28+4*axis,shifted_position(v));write16(memory.bytes+6+2*axis,v);}
        memset(a+off+0x34,0,12);}
    write32(a+0x70,d->composite.token);
    if(d->composite.token){const nflstreet_actor_model_resource*r=&d->composite;unsigned off=0x214;
        memset(a+off,0,0x13c);write32(a+off+0x138,r->token);a[off+1]=0xd;
        if(!channel(a,off+0x100,r,0,fn,ctx,&memory))return 0;write32(a+off+0x134,0);a[off]=1;
        for(unsigned i=0;i<9;++i)for(unsigned axis=0;axis<3;++axis){uint16_t v=read16(r->bytes+0x41c+i*12+axis*2);write32(a+off+0x28+i*12+axis*4,shifted_position(v));}
        memset(a+off+0x94,0,9*12);memcpy(memory.bytes,r->bytes+0x410,19*6);}
    write32(a+0x3ec,0);return 1;
}
