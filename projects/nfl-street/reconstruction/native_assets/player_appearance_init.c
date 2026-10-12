#include "player_appearance_init.h"
#include <string.h>
static uint32_t read(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void word(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void bits(uint8_t*p,uint32_t v){word(p,read(p)|v);}
static void vector(uint8_t*p){word(p,0);word(p+4,0xe0000000u);word(p+8,0);}
static void mapping(uint8_t*p,unsigned slot,uint32_t v){if(p[8+slot]==v)return;p[8+slot]=(uint8_t)v;bits(p+((slot>>3)&0x1ffffffcu),1u<<(slot&3));}
static void node(uint8_t*a,unsigned off,unsigned voff,uint32_t token){
    word(a+off,1);word(a+off+12,0);word(a+off+16,0);word(a+off+0xe0,0);word(a+off+0xe4,0);word(a+off+0xe8,0);word(a+off+0xec,0);
    for(unsigned i=0;i<64;++i){a[off+0x14+8+i]=0xff;bits(a+off+0x14+((i>>3)&0x1ffffffcu),1u<<(i&3));}
    bits(a+off+4,0x411);bits(a+off+8,0x411);word(a+off+0xe0,token+voff);word(a+off+0xe8,token+0xfec);
}
static int service(const nflstreet_player_appearance_context*c,uint8_t*a,size_t size,
    nflstreet_player_appearance_service_kind kind,uint32_t off,uint32_t x,uint32_t y,uint32_t z,uint32_t w,uint32_t v,uint32_t*out){
    nflstreet_player_appearance_request q={kind,off,{x,y,z,w,v}};return c->service(c->service_context,&q,a,size,out);
}
static int allocation(const nflstreet_player_appearance_context*c,nflstreet_player_appearance_allocation_kind kind,unsigned count,
    nflstreet_player_appearance_memory*m){memset(m,0,sizeof *m);return c->allocate(c->allocation_context,kind,count,4,m)&&m->token&&m->bytes&&m->size>=count*4u;}
int nflstreet_player_appearance_initialize(uint8_t*a,size_t size,const uint8_t*d,size_t dsize,const nflstreet_player_appearance_context*c){
    if(!a||size<0x1a90||!d||dsize<0x45||!c||!c->actor_token||c->actor_token>UINT32_MAX-0x1a90u||!c->catalog||!c->registry||!c->service||!c->allocate)return 0;
    const nflstreet_player_appearance_catalog*cat=c->catalog;nflstreet_player_appearance_registry*r=c->registry;uint32_t key=read(d+0x38),style=d[0x42];
    if(!cat->side_texture_tokens||d[0x43]>=cat->side_count||!cat->finish_records||!cat->indexed_words||key>=cat->indexed_word_count||!cat->mapping_words||style>=cat->mapping_count||
        !r->actor_entries||((uint64_t)r->actor_count+1)*0x34>r->actor_entries_size||!r->binding_entries||((uint64_t)r->binding_count+1)*4>r->binding_entries_size)return 0;
    word(a+0x3fc,0);word(a+0x3f8,0);for(unsigned i=0;i<7;++i)word(a+0x400+4*i,read(d+0x20+4*i));
    memcpy(a+0x1360,d+0x40,5);word(a+0x1358,cat->indexed_words[key]);uint32_t out=0,side_resource=0,model=0;
#define CALL(K,O,X,Y,Z,W,V) do {if(!service(c,a,size,K,O,X,Y,Z,W,V,&out))return 0;}while(0)
    CALL(NFLSTREET_APPEARANCE_BASE_RESOURCE,0,0,0,0,0,0);word(a+0x41c,out);
    CALL(NFLSTREET_APPEARANCE_SIDE_RESOURCE,0,d[0x43],0,0,0,0);side_resource=out;word(a+0xffc,out);
    CALL(NFLSTREET_APPEARANCE_MODEL_RESOURCE,0,0,key,0,0,0);model=out;
    CALL(NFLSTREET_APPEARANCE_BIND_MODEL,0,read(d+4),0,0,0,0);
    word(a+0x4a8,r->actor_entries_token+r->actor_count*0x34);word(r->actor_entries+r->actor_count*0x34,c->actor_token);++r->actor_count;
    const unsigned vectors[]={0x1008,0x1014,0x1020,0x102c,0x1038};for(unsigned i=0;i<5;++i)vector(a+vectors[i]);
    CALL(NFLSTREET_APPEARANCE_TEXTURE_RESET,0,0,0,0,0,0);CALL(NFLSTREET_APPEARANCE_SIDE_RESOURCE,0,d[0x43],0,0,0,0);
    CALL(NFLSTREET_APPEARANCE_TEXTURE_SELECT,0,cat->side_texture_tokens[d[0x43]],0,0,0,0);
    nflstreet_player_appearance_memory memory;if(!allocation(c,NFLSTREET_APPEARANCE_NODE_TABLE,45,&memory))return 0;
    word(a+0xfec,45);word(a+0xff0,memory.token);memset(memory.bytes,0,45*4);
    CALL(NFLSTREET_APPEARANCE_COPY_NODES,0xfec,c->model_node_list_token,0,0,0,0);
    CALL(NFLSTREET_APPEARANCE_SIDE_RESOURCE,0,d[0x43],0,0,0,0);CALL(NFLSTREET_APPEARANCE_BIND_NODES,0xfec,model,out,0,0,0);
    const unsigned ids[]={10,11,19,20,14,15,23,24};const unsigned offsets[]={0x260,0x260,0x280,0x280,0x2a0,0x2a0,0x2c0,0x2c0};
    for(unsigned i=0;i<8;++i){uint32_t first;CALL(NFLSTREET_APPEARANCE_SIDE_RESOURCE,0,d[0x43],0,0,0,0);first=out;
        CALL(NFLSTREET_APPEARANCE_SIDE_RESOURCE,0,d[0x43],0,0,0,0);CALL(NFLSTREET_APPEARANCE_BIND_NODE,0xfec,ids[i],1,first+offsets[i],out+offsets[i],0);}
    CALL(NFLSTREET_APPEARANCE_SIDE_RESOURCE,0,d[0x43],0,0,0,0);side_resource=out;
    for(size_t i=0;i<cat->finish_count;++i){const uint32_t*record=cat->finish_records[i];uint32_t texture;
        CALL(NFLSTREET_APPEARANCE_FIND_SIDE_NODE,0,side_resource,record[0],0,0,0);texture=out;
        if(record[1]>>24)CALL(NFLSTREET_APPEARANCE_RESET_SIDE_NODE,0,texture,0,0,0,0);
        nflstreet_player_appearance_request q={NFLSTREET_APPEARANCE_APPLY_SIDE_NODE,0,{texture,record[2],record[3],record[4],record[5],record[6]}};
        if(!c->service(c->service_context,&q,a,size,&out))return 0;
    }
    CALL(NFLSTREET_APPEARANCE_TEXTURE_RESET,0,0,0,0,0,0);
    node(a,0x4ac,0x1008,c->actor_token);node(a,0x59c,0x1014,c->actor_token);
    if(!allocation(c,NFLSTREET_APPEARANCE_CHANNEL_TABLE,64,&memory))return 0;word(a+0x350,64);word(a+0x354,memory.token);
    for(unsigned i=0;i<64;++i)word(memory.bytes+4*i,cat->channel_default_word);
    bits(a+0x59c+4,1);bits(a+0x59c+8,1);word(a+0x59c+0xe4,c->actor_token+0x350);word(a+0x358,12);a[0x35e]=0;a[0x35f]=0x25;
    const unsigned nodes[]={0x68c,0x77c,0x86c,0x95c,0xa4c,0xefc};const unsigned vptrs[]={0x1020,0x102c,0x102c,0x1038,0x1008,0x1008};
    for(unsigned i=0;i<6;++i)node(a,nodes[i],vptrs[i],c->actor_token);
    for(unsigned i=0;i<2;++i){vector(a+0x1044+i*12);node(a,0xb3c+i*0xf0,0x1044+i*12,c->actor_token);}
    for(unsigned i=0;i<2;++i){vector(a+0x105c+i*12);node(a,0xd1c+i*0xf0,0x105c+i*12,c->actor_token);}
    word(a+0x108c,read(a+0x41c));word(a+0x1094,c->actor_token+0x3ac);word(a+0x1090,cat->matrix_binding_token);word(a+0x1098,0);
    word(r->binding_entries+4*r->binding_count,c->actor_token+0x1074);++r->binding_count;
    const unsigned mapped[]={5,7,8,9,10,11,14,15,16,17,18};for(unsigned i=0;i<11;++i)mapping(a+0x4c0,mapped[i],cat->mapping_words[style][mapped[i]]);
    mapping(a+0x5b0,1,0);mapping(a+0x6a0,0,cat->mapping_words[style][0]);mapping(a+0x790,23,0);mapping(a+0x880,21,0);mapping(a+0x970,24,0);mapping(a+0xd30,5,0);mapping(a+0xe20,5,0);
#undef CALL
    return 1;
}
