#ifndef NFLSTREET_PLAYER_APPEARANCE_CHECK_FIXTURE_H
#define NFLSTREET_PLAYER_APPEARANCE_CHECK_FIXTURE_H
#define main bounded_model_checker_main
#include "actor_model_init_original_check.c"
#undef main
#include "player_appearance_init.h"
#define DESCRIPTOR 0x817d0100u
#define DESCRIPTOR_SIZE 0x45u
#define NODE_HEAP 0x815c0000u
#define CHANNEL_HEAP 0x815c1000u
#define MODEL_TOKEN 0x815d0000u
#define SIDE_TOKEN 0x815e0000u
#define BASE_TOKEN 0x815f0000u
#define NODE_LIST_TOKEN 0x815d0100u
#define REGISTRY_TOKEN 0x8030ffdcu
#define BINDINGS_TOKEN 0x8034fb88u
#define REGISTRY_SIZE 512u
typedef struct { nflstreet_player_appearance_request request;uint32_t response; } request_entry;
typedef struct {
    request_entry entries[128];unsigned count,mutation;
    uint8_t node_memory[512],channel_memory[512];unsigned node_allocations,channel_allocations;
} appearance_trace;
static void supplied_binding(uint8_t*a,const nflstreet_player_appearance_request*q,unsigned mutation){
    if(!mutation)return;if(q->kind==NFLSTREET_APPEARANCE_BIND_MODEL){for(unsigned i=0x420;i<0x4a8;++i)a[i]=(uint8_t)(i*7+0x21);}
    if(q->kind==NFLSTREET_APPEARANCE_COPY_NODES)put(a+q->target_offset+8,0xa55a1122u);
}
static int appearance_service(void*opaque,const nflstreet_player_appearance_request*q,uint8_t*a,size_t size,uint32_t*out){
    appearance_trace*t=opaque;if(t->count>=128||size!=NFLSTREET_ACTOR_MODEL_STORAGE)return 0;
    request_entry*r=t->entries+t->count++;r->request=*q;
    switch(q->kind){case NFLSTREET_APPEARANCE_BASE_RESOURCE:*out=BASE_TOKEN;break;
    case NFLSTREET_APPEARANCE_SIDE_RESOURCE:*out=SIDE_TOKEN+q->arguments[0]*0x1000u;break;
    case NFLSTREET_APPEARANCE_MODEL_RESOURCE:*out=MODEL_TOKEN;break;
    case NFLSTREET_APPEARANCE_FIND_SIDE_NODE:*out=0x11330000u;break;
    default:*out=0;break;}r->response=*out;supplied_binding(a,q,t->mutation);return 1;
}
static int appearance_allocate(void*opaque,nflstreet_player_appearance_allocation_kind kind,uint32_t count,uint32_t stride,nflstreet_player_appearance_memory*out){
    appearance_trace*t=opaque;if(stride!=4)return 0;
    if(kind==NFLSTREET_APPEARANCE_NODE_TABLE){if(count!=45||t->node_allocations++)return 0;out->token=NODE_HEAP;out->bytes=t->node_memory;}
    else {if(count!=64||t->channel_allocations++)return 0;out->token=CHANNEL_HEAP;out->bytes=t->channel_memory;}out->size=512;memset(out->bytes,0xa5,512);return 1;
}
static int appearance_boundary(CPUState*c,uint8_t*ram,appearance_trace*t){
    nflstreet_player_appearance_request q={0};uint32_t pc=c->pc;
    switch(pc){
    case 0x800e088cu:q.kind=NFLSTREET_APPEARANCE_BASE_RESOURCE;q.arguments[0]=c->gpr[3];break;
    case 0x800e2fc0u:q.kind=NFLSTREET_APPEARANCE_SIDE_RESOURCE;q.arguments[0]=c->gpr[3];break;
    case 0x800e0734u:q.kind=NFLSTREET_APPEARANCE_MODEL_RESOURCE;memcpy(q.arguments,c->gpr+3,3*4);break;
    case 0x801630b0u:q.kind=NFLSTREET_APPEARANCE_BIND_MODEL;if(c->gpr[3]!=ACTOR)return -1;q.arguments[0]=c->gpr[4];break;
    case 0x801c9028u:q.kind=NFLSTREET_APPEARANCE_TEXTURE_RESET;break;
    
    case 0x801c9050u:q.kind=NFLSTREET_APPEARANCE_TEXTURE_SELECT;if(c->gpr[3]!=0x80260b70u)return -1;q.arguments[0]=c->gpr[4];break;
    case 0x801c7528u:q.kind=NFLSTREET_APPEARANCE_COPY_NODES;q.target_offset=c->gpr[3]-ACTOR;q.arguments[0]=c->gpr[4];break;
    case 0x801ebba8u:q.kind=NFLSTREET_APPEARANCE_BIND_NODES;q.target_offset=c->gpr[3]-ACTOR;memcpy(q.arguments,c->gpr+4,2*4);break;
    case 0x801ebc78u:q.kind=NFLSTREET_APPEARANCE_BIND_NODE;q.target_offset=c->gpr[3]-ACTOR;memcpy(q.arguments,c->gpr+4,5*4);break;
    case 0x801ebb44u:q.kind=NFLSTREET_APPEARANCE_FIND_SIDE_NODE;memcpy(q.arguments,c->gpr+3,2*4);break;
    case 0x801c9208u:q.kind=NFLSTREET_APPEARANCE_RESET_SIDE_NODE;q.arguments[0]=c->gpr[3];break;
    case 0x801c91a4u:q.kind=NFLSTREET_APPEARANCE_APPLY_SIDE_NODE;q.arguments[0]=c->gpr[3];for(unsigned i=0;i<5;++i)q.arguments[1+i]=be(ptr(ram,c->gpr[4]+4*i,4));break;
    default:return 0;}
    uint32_t out=0;if(!appearance_service(t,&q,ptr(ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),NFLSTREET_ACTOR_MODEL_STORAGE,&out))return -1;
    c->gpr[3]=q.kind==NFLSTREET_APPEARANCE_FIND_SIDE_NODE?0x815d0200u:out;c->pc=c->lr;return 1;
}
#endif
