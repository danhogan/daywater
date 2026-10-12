#include "player_constructor.h"
static uint32_t read(const uint8_t*p) {
    return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
}
static void write(uint8_t*p,uint32_t v) {
    p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);
    p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;
}
int nflstreet_player_construct(uint8_t *actor,size_t size,
    const nflstreet_player_constructor_input *input,
    nflstreet_actor_model_allocator allocate,void *allocate_context,
    nflstreet_player_appearance_initializer appearance,void *appearance_context) {
    if(!actor || size<NFLSTREET_ACTOR_MODEL_STORAGE || !input ||
        !input->bytes || input->size<0x45 || !allocate || !appearance) return 0;
    const nflstreet_actor_model_descriptor *d=&input->models;
    uint32_t tokens[]={d->skeleton.token,d->primary[0].token,d->primary[1].token,
        d->secondary[0].token,d->secondary[1].token,d->composite.token};
    for(unsigned i=0;i<6;++i) if(read(input->bytes+4+4*i)!=tokens[i]) return 0;
    if(!nflstreet_actor_model_initialize(actor,size,d,allocate,allocate_context)) return 0;
    write(actor+0x135c,read(input->bytes+0x1c));
    if(!nflstreet_actor_tail_initialize(actor+0x1368,size-0x1368,&input->tail)) return 0;
    return appearance(appearance_context,actor,size,input->bytes,input->size)!=0;
}
