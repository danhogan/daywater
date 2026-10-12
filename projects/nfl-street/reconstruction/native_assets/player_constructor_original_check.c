/* Fresh isolated constructor oracle; player appearance is an explicit supplied
   boundary. Reuse source resource and allocation checks from the base oracle. */
#define main bounded_model_checker_main
#include "actor_model_init_original_check.c"
#undef main
#include "player_constructor.h"
#define DESCRIPTOR 0x817d0100u
#define DESCRIPTOR_SIZE 0x45u
typedef struct {
    allocations allocations;
    unsigned appearance_calls, mutate;
    uint8_t appearance_actor[NFLSTREET_ACTOR_MODEL_STORAGE];
    uint8_t appearance_descriptor[DESCRIPTOR_SIZE];
} constructor_trace;
static void appearance_mutation(uint8_t*actor){
    /* Explicit test mutation proves supplied owner sees the completed core and
       its writes survive constructor return; these are not game semantics. */
    actor[0x1000]^=0x5a;put(actor+0x135c,be(actor+0x135c)^0x12345678u);actor[0x1a8f]=0x79;
}
static void appearance_record(constructor_trace*t,uint8_t*actor,const uint8_t*descriptor){
    ++t->appearance_calls;memcpy(t->appearance_actor,actor,sizeof t->appearance_actor);
    memcpy(t->appearance_descriptor,descriptor,DESCRIPTOR_SIZE);if(t->mutate)appearance_mutation(actor);
}
static int constructor_original(const ModernGekkoModuleDesc*m,uint8_t*ram,const uint8_t initial[NFLSTREET_ACTOR_MODEL_STORAGE],
    const uint8_t descriptor[DESCRIPTOR_SIZE],constructor_trace*t){
    memcpy(ptr(ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),initial,NFLSTREET_ACTOR_MODEL_STORAGE);
    memcpy(ptr(ram,DESCRIPTOR,DESCRIPTOR_SIZE),descriptor,DESCRIPTOR_SIZE);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
    c.gpr[3]=ACTOR;c.gpr[4]=DESCRIPTOR;c.pc=0x800e1f7cu;c.cycle_budget=200000000;
    uint32_t channel_count=0;int channel_request=0;unsigned initial_clear_calls=0;
    for(unsigned step=0;c.pc&&step<300000;++step){
        if(c.pc==0x8015e910u){channel_count=c.gpr[3]&0xffffu;channel_request=1;}
        if(c.pc==0x8018b7b8u){allocations*at=&t->allocations;if(at->count>=MAX_BLOCKS||c.gpr[3]>HEAP_STRIDE||c.gpr[4])return 0;
            unsigned i=at->count++;block*b=at->blocks+i;b->size=c.gpr[3];b->count=channel_request?channel_count:c.gpr[3]/64;b->kind=channel_request?NFLSTREET_ACTOR_CHANNEL:NFLSTREET_ACTOR_TRANSFORMS;
            memset(ptr(ram,HEAP+i*HEAP_STRIDE,HEAP_STRIDE),0xa5,HEAP_STRIDE);c.gpr[3]=HEAP+i*HEAP_STRIDE;c.pc=c.lr;channel_request=0;continue;}
        if(c.pc==0x8017bc54u){if(c.gpr[3]==ACTOR){if(c.gpr[4]!=NFLSTREET_ACTOR_MODEL_STORAGE||c.gpr[5])return 0;++initial_clear_calls;}
            uint8_t*p=ptr(ram,c.gpr[3],c.gpr[5]);if(!p)return 0;memset(p,(uint8_t)c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
        if(c.pc==0x801639c8u){if(c.gpr[3]!=ACTOR||c.gpr[4]!=DESCRIPTOR)return 0;
            appearance_record(t,ptr(ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),ptr(ram,DESCRIPTOR,DESCRIPTOR_SIZE));c.pc=c.lr;continue;}
        c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"constructor exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}
    }
    if(c.pc||initial_clear_calls!=1)return 0;
    for(unsigned i=0;i<t->allocations.count;++i)memcpy(t->allocations.blocks[i].storage,ptr(ram,HEAP+i*HEAP_STRIDE,HEAP_STRIDE),HEAP_STRIDE);
    return 1;
}
static int appearance_native(void*opaque,uint8_t*actor,size_t actor_size,const uint8_t*descriptor,size_t descriptor_size){
    if(actor_size!=NFLSTREET_ACTOR_MODEL_STORAGE||descriptor_size!=DESCRIPTOR_SIZE)return 0;
    appearance_record(opaque,actor,descriptor);return 1;
}
static int constructor_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const nflstreet_player_constructor_input*source,unsigned mask,unsigned prefilled,unsigned mutation){
    uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    nflstreet_player_constructor_input input=*source;uint8_t descriptor[DESCRIPTOR_SIZE];memcpy(descriptor,source->bytes,sizeof descriptor);
    nflstreet_actor_model_resource*resources[]={&input.models.skeleton,&input.models.primary[0],&input.models.primary[1],&input.models.secondary[0],&input.models.secondary[1],&input.models.composite};
    for(unsigned i=0;i<6;++i){if(!(mask&(1u<<i)))memset(resources[i],0,sizeof *resources[i]);put(descriptor+4+4*i,resources[i]->token);}input.bytes=descriptor;
    uint8_t initial[NFLSTREET_ACTOR_MODEL_STORAGE],native[NFLSTREET_ACTOR_MODEL_STORAGE];
    for(unsigned i=0;i<sizeof initial;++i)initial[i]=prefilled?(uint8_t)(0x39u+i*37u):0;memcpy(native,initial,sizeof native);
    constructor_trace*o=calloc(1,sizeof *o),*n=calloc(1,sizeof *n);if(!o||!n){free(o);free(n);free(ram);return 0;}o->mutate=n->mutate=mutation;
    if(!constructor_original(m,ram,initial,descriptor,o)||!nflstreet_player_construct(native,sizeof native,&input,allocate,&n->allocations,appearance_native,n)){
        fprintf(stderr,"constructor mask%u prefill%u mutation%u executionfailed\n",mask,prefilled,mutation);free(o);free(n);free(ram);return 0;}
    int ok=!memcmp(native,ptr(ram,ACTOR,sizeof native),sizeof native)&&!memcmp(o,n,sizeof *o);
    if(!ok){fprintf(stderr,"constructor mismatch mask%u prefill%u mutation%u calls%u/%u blocks%u/%u\n",mask,prefilled,mutation,o->appearance_calls,n->appearance_calls,o->allocations.count,n->allocations.count);
        for(unsigned i=0;i<sizeof native;++i)if(native[i]!=ptr(ram,ACTOR,sizeof native)[i]){fprintf(stderr,"firstactorbyte%04x original%02x/native%02x\n",i,ptr(ram,ACTOR,sizeof native)[i],native[i]);break;}}
    else printf("constructor mask=%u prefilled=%u mutation=%u: full actor1A90 +%u blocks +appearance45 exact\n",mask,prefilled,mutation,o->allocations.count);
    free(o);free(n);free(ram);return ok;
}
int main(int argc,char**argv){
    if(argc!=3){fprintf(stderr,"Usage: player-constructor-original-check.exe originalDLL savedRAM\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    uint8_t*baseline=load(argv[2]);if(!baseline){FreeLibrary(h);return 4;}
    uint8_t*slot=ptr(baseline,0x803a9a40u-0x7bbcu,4);uint8_t*list=slot?ptr(baseline,be(slot),4):NULL;uint8_t*a=list?ptr(baseline,be(list),NFLSTREET_ACTOR_MODEL_STORAGE):NULL;
    if(!a){free(baseline);FreeLibrary(h);return 4;}nflstreet_player_constructor_input input={0};uint8_t descriptor[DESCRIPTOR_SIZE];
    for(unsigned i=0;i<sizeof descriptor;++i)descriptor[i]=(uint8_t)(0x27u+17u*i); /* Explicit remaining descriptor bytes. */
    input.bytes=descriptor;input.size=sizeof descriptor;nflstreet_actor_model_descriptor*d=&input.models;
    d->skeleton=resource(baseline,be(a+0x64));d->primary[0]=resource(baseline,be(a+0xbc));d->primary[1]=resource(baseline,be(a+0x104));
    d->secondary[0]=resource(baseline,be(a+0x194));d->secondary[1]=resource(baseline,be(a+0x210));d->composite=resource(baseline,be(a+0x70));d->initial_word_bits=be(ptr(baseline,0x8025283cu,4));
    if(!d->skeleton.token||!d->primary[0].token||!d->primary[1].token||!d->secondary[0].token||!d->secondary[1].token||!d->composite.token){free(baseline);FreeLibrary(h);return 4;}
    input.tail.repeated_float_bits=be(ptr(baseline,0x80252654u,4));input.tail.default_float_bits=be(ptr(baseline,0x802526b0u,4));
    for(unsigned i=0;i<4;++i)input.tail.defaults[i]=be(ptr(baseline,0x80270c84u+i*4,4));
    printf("Source constructor constants=%08x/%08x/%08x/%08x/%08x/%08x/%08x\n",d->initial_word_bits,input.tail.repeated_float_bits,input.tail.default_float_bits,input.tail.defaults[0],input.tail.defaults[1],input.tail.defaults[2],input.tail.defaults[3]);
    int ok=1;for(unsigned mask=0;mask<64&&ok;++mask)for(unsigned prefilled=0;prefilled<2&&ok;++prefilled)for(unsigned mutation=0;mutation<2&&ok;++mutation)ok=constructor_case(m,baseline,&input,mask,prefilled,mutation);
    free(baseline);FreeLibrary(h);if(ok)puts("Original800E1F7C + base and tail helpers differential passed256 fresh/prefilled cases; appearance is supplied, no complete player created.");return ok?0:5;
}
