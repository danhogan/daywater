#ifndef NFLSTREET_PLAYER_MODEL_BINDING_CHECK_FIXTURE_H
#define NFLSTREET_PLAYER_MODEL_BINDING_CHECK_FIXTURE_H
/* Requires the baseline oracle helpers from actor_model_init_original_check.c. */
#include "player_model_binding.h"
#define BIND_HEAP 0x81500000u
#define BIND_STRIDE 0x10000u
#define BIND_BLOCKS 12
#define BIND_REQUESTS 4096
#define TABLE_TOKEN 0x8028dd10u
#define SCRATCH_TOKEN 0x8028dd18u
#define SHARED_TOKEN 0x803102b4u
typedef struct {nflstreet_player_model_request request;uint32_t output;uint8_t scratch[76];} binding_request;
typedef struct {uint32_t token,count,stride,released;uint8_t bytes[BIND_STRIDE];} binding_block;
typedef struct {
 uint8_t*ram;uint32_t resources[2];unsigned null_initial,mutation;
 binding_request requests[BIND_REQUESTS];unsigned count;
 binding_block blocks[BIND_BLOCKS];unsigned allocations;
} binding_fixture;
static int binding_resolve(void*opaque,uint32_t token,size_t min,nflstreet_player_model_span*out){
 binding_fixture*f=opaque;uint8_t*p=ptr(f->ram,token,min);if(!p)return 0;out->bytes=p;out->size=RAM_SIZE-(token-BASE);return 1;
}
static int binding_allocate(void*opaque,uint32_t count,uint32_t stride,uint32_t*token,nflstreet_player_model_span*out){
 binding_fixture*f=opaque;if(f->allocations>=BIND_BLOCKS||(uint64_t)count*stride>BIND_STRIDE)return 0;
 unsigned i=f->allocations++;binding_block*b=f->blocks+i;b->count=count;b->stride=stride;b->token=BIND_HEAP+i*BIND_STRIDE;
 memset(ptr(f->ram,b->token,BIND_STRIDE),0xa5,BIND_STRIDE);*token=b->token;out->bytes=ptr(f->ram,b->token,BIND_STRIDE);out->size=BIND_STRIDE;return 1;
}
static int binding_release(void*opaque,uint32_t token){binding_fixture*f=opaque;for(unsigned i=0;i<f->allocations;++i)if(f->blocks[i].token==token){f->blocks[i].released++;return 1;}return 0;}
static void bind_mutation(uint8_t*a,const nflstreet_player_model_request*q,unsigned mutation){
 if(!mutation)return;if(q->kind==NFLSTREET_MODEL_RENDER_BLOCK)put(a+q->target_offset+4,0x11110000u+q->target_offset);
 if(q->kind==NFLSTREET_MODEL_BIND_WORDS)put(a+q->target_offset,be(a+q->target_offset)^0x10203040u);
}
static int binding_service(void*opaque,const nflstreet_player_model_request*q,uint8_t*a,size_t size,uint32_t*out){
 binding_fixture*f=opaque;if(f->count>=BIND_REQUESTS||size!=NFLSTREET_ACTOR_MODEL_STORAGE)return 0;
 binding_request*r=f->requests+f->count++;r->request=*q;memcpy(r->scratch,ptr(f->ram,SCRATCH_TOKEN,76),76);*out=0;
 if(q->kind==NFLSTREET_MODEL_RESOURCE){if(q->arguments[2]>1)return 0;*out=q->arguments[0]==4&&f->null_initial?0:f->resources[q->arguments[2]];}
 if(q->kind==NFLSTREET_MODEL_NAMES_EQUAL){uint8_t*x=ptr(f->ram,q->arguments[0],1),*y=ptr(f->ram,q->arguments[1],1);if(!x||!y)return 0;
  *out=strcmp((const char*)x,(const char*)y)==0;}
 r->output=*out;bind_mutation(a,q,f->mutation);return 1;
}
static int binding_boundary(CPUState*c,binding_fixture*f){
 nflstreet_player_model_request q={0};uint8_t*a=ptr(f->ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE);
 switch(c->pc){
 case 0x800e0ac4u:q.kind=NFLSTREET_MODEL_PREPARE_KEY;q.arguments[0]=c->gpr[3];break;
 case 0x800e0734u:q.kind=NFLSTREET_MODEL_RESOURCE;memcpy(q.arguments,c->gpr+3,12);break;
 case 0x801eac04u:q.kind=NFLSTREET_MODEL_RENDER_BLOCK;q.target_offset=c->gpr[3]-ACTOR;q.arguments[0]=c->gpr[5];if(c->gpr[4]!=SCRATCH_TOKEN)return -1;break;
 case 0x800e1404u:q.kind=NFLSTREET_MODEL_PREPARE_STYLE;q.arguments[0]=c->gpr[3];break;
 case 0x8017cca0u:q.kind=NFLSTREET_MODEL_NAMES_EQUAL;memcpy(q.arguments,c->gpr+3,8);break;
 case 0x80164a88u:q.kind=NFLSTREET_MODEL_RESET;q.arguments[0]=c->gpr[3];break;
 case 0x800e144cu:q.kind=NFLSTREET_MODEL_COMMIT_STYLE;q.arguments[0]=c->gpr[3];break;
 case 0x800e0c88u:q.kind=NFLSTREET_MODEL_BIND_WORDS;q.arguments[0]=c->gpr[3];q.target_offset=c->gpr[4]-ACTOR;break;
 case 0x800e0dbcu:q.kind=NFLSTREET_MODEL_BIND_A;memcpy(q.arguments,c->gpr+3,8);break;
 case 0x800e0ea4u:q.kind=NFLSTREET_MODEL_BIND_B;memcpy(q.arguments,c->gpr+3,8);break;
 case 0x80164ab8u:q.kind=NFLSTREET_MODEL_FINISH;break;
 default:return 0;}
 uint32_t out=0;if(!binding_service(f,&q,a,NFLSTREET_ACTOR_MODEL_STORAGE,&out))return -1;
 c->gpr[3]=q.kind==NFLSTREET_MODEL_NAMES_EQUAL?(out?0:1):out;c->pc=c->lr;return 1;
}
#endif
