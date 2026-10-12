#include "player_appearance_check_fixture.h"
#include "player_startup.h"
typedef struct { allocations model;appearance_trace appearance; } startup_trace;
static int startup_original(const ModernGekkoModuleDesc*m,uint8_t*ram,const uint8_t*initial,const uint8_t*descriptor,startup_trace*t){
    memcpy(ptr(ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),initial,NFLSTREET_ACTOR_MODEL_STORAGE);memcpy(ptr(ram,DESCRIPTOR,DESCRIPTOR_SIZE),descriptor,DESCRIPTOR_SIZE);
    put(ptr(ram,MODEL_TOKEN+0x18,4),NODE_LIST_TOKEN);put(ptr(ram,0x815d0200u+0x18,4),0x11330000u);for(unsigned i=0;i<2;++i)put(ptr(ram,SIDE_TOKEN+i*0x1000u+0x18,4),0x11220000u+i);
    put(ptr(ram,0x803a9a40u-0x58e8u,4),2);put(ptr(ram,0x803a9a40u-0x6238u,4),3);memset(ptr(ram,REGISTRY_TOKEN,REGISTRY_SIZE),0xa5,REGISTRY_SIZE);memset(ptr(ram,BINDINGS_TOKEN,REGISTRY_SIZE),0xa5,REGISTRY_SIZE);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=ACTOR;c.gpr[4]=DESCRIPTOR;c.pc=0x800e1f7cu;c.cycle_budget=200000000;
    uint32_t channel_count=0;int channel_request=0;unsigned initial_clear_calls=0;
    for(unsigned step=0;c.pc&&step<400000;++step){
        int handled=appearance_boundary(&c,ram,&t->appearance);if(handled<0)return 0;if(handled)continue;
        if(c.pc==0x8015e910u){channel_count=c.gpr[3]&0xffffu;channel_request=1;}
        if(c.pc==0x801c9784u){if(c.gpr[3]!=45||c.gpr[4]!=4||t->appearance.node_allocations++)return 0;memset(ptr(ram,NODE_HEAP,512),0xa5,512);c.gpr[3]=NODE_HEAP;c.pc=c.lr;continue;}
        if(c.pc==0x8018b7b8u){
            if(c.gpr[3]==256&&!channel_request){if(c.gpr[4]||t->appearance.channel_allocations++)return 0;memset(ptr(ram,CHANNEL_HEAP,512),0xa5,512);c.gpr[3]=CHANNEL_HEAP;c.pc=c.lr;continue;}
            allocations*at=&t->model;if(at->count>=MAX_BLOCKS||c.gpr[3]>HEAP_STRIDE||c.gpr[4])return 0;unsigned i=at->count++;block*b=at->blocks+i;
            b->size=c.gpr[3];b->count=channel_request?channel_count:c.gpr[3]/64;b->kind=channel_request?NFLSTREET_ACTOR_CHANNEL:NFLSTREET_ACTOR_TRANSFORMS;
            memset(ptr(ram,HEAP+i*HEAP_STRIDE,HEAP_STRIDE),0xa5,HEAP_STRIDE);c.gpr[3]=HEAP+i*HEAP_STRIDE;c.pc=c.lr;channel_request=0;continue;}
        if(c.pc==0x8017bc54u){if(c.gpr[3]==ACTOR){if(c.gpr[4]!=NFLSTREET_ACTOR_MODEL_STORAGE||c.gpr[5])return 0;++initial_clear_calls;}
            uint8_t*p=ptr(ram,c.gpr[3],c.gpr[5]);if(!p)return 0;memset(p,(uint8_t)c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
        c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"startup exception %08x pc%08x\n",c.exception,c.pc);return 0;}
    }
    if(c.pc||initial_clear_calls!=1)return 0;
    for(unsigned i=0;i<t->model.count;++i)memcpy(t->model.blocks[i].storage,ptr(ram,HEAP+i*HEAP_STRIDE,HEAP_STRIDE),HEAP_STRIDE);
    memcpy(t->appearance.node_memory,ptr(ram,NODE_HEAP,512),512);memcpy(t->appearance.channel_memory,ptr(ram,CHANNEL_HEAP,512),512);return 1;
}
static int startup_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const nflstreet_player_constructor_input*source,
    const nflstreet_player_appearance_catalog*catalog,unsigned mask,unsigned prefill,unsigned mutation){
    uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    nflstreet_player_constructor_input input=*source;uint8_t descriptor[DESCRIPTOR_SIZE];memcpy(descriptor,source->bytes,sizeof descriptor);
    nflstreet_actor_model_resource*resources[]={&input.models.skeleton,&input.models.primary[0],&input.models.primary[1],&input.models.secondary[0],&input.models.secondary[1],&input.models.composite};
    for(unsigned i=0;i<6;++i){if(!(mask&(1u<<i)))memset(resources[i],0,sizeof *resources[i]);put(descriptor+4+4*i,resources[i]->token);}input.bytes=descriptor;
    uint8_t initial[NFLSTREET_ACTOR_MODEL_STORAGE],native[NFLSTREET_ACTOR_MODEL_STORAGE];for(unsigned i=0;i<sizeof initial;++i)initial[i]=prefill?(uint8_t)(i*29+0x49):0;memcpy(native,initial,sizeof native);
    startup_trace*o=calloc(1,sizeof *o),*n=calloc(1,sizeof *n);if(!o||!n){free(o);free(n);free(ram);return 0;}o->appearance.mutation=n->appearance.mutation=mutation;
    if(!startup_original(m,ram,initial,descriptor,o)){free(o);free(n);free(ram);return 0;}
    uint8_t actors[REGISTRY_SIZE],bindings[REGISTRY_SIZE];memset(actors,0xa5,sizeof actors);memset(bindings,0xa5,sizeof bindings);
    nflstreet_player_appearance_registry registry={actors,sizeof actors,REGISTRY_TOKEN,2,bindings,sizeof bindings,3};
    nflstreet_player_appearance_context context={ACTOR,NODE_LIST_TOKEN,catalog,&registry,appearance_service,&n->appearance,appearance_allocate,&n->appearance};
    int ok=nflstreet_player_initialize(native,sizeof native,&input,allocate,&n->model,&context)&&
        !memcmp(native,ptr(ram,ACTOR,sizeof native),sizeof native)&&!memcmp(o,n,sizeof *o)&&
        !memcmp(actors,ptr(ram,REGISTRY_TOKEN,REGISTRY_SIZE),REGISTRY_SIZE)&&!memcmp(bindings,ptr(ram,BINDINGS_TOKEN,REGISTRY_SIZE),REGISTRY_SIZE)&&
        registry.actor_count==be(ptr(ram,0x803a9a40u-0x58e8u,4))&&registry.binding_count==be(ptr(ram,0x803a9a40u-0x6238u,4));
    if(!ok){fprintf(stderr,"startup mismatch mask%u prefill%u mutation%u requests%u/%u blocks%u/%u\n",mask,prefill,mutation,o->appearance.count,n->appearance.count,o->model.count,n->model.count);
        for(unsigned i=0;i<sizeof native;++i)if(native[i]!=ptr(ram,ACTOR,sizeof native)[i]){fprintf(stderr,"actorbyte%04x original%02x native%02x\n",i,ptr(ram,ACTOR,sizeof native)[i],native[i]);break;}}
    else printf("startup mask=%u prefilled=%u mutation=%u: actor1A90 +%u model blocks +two appearance allocations +two registries +%u requests exact\n",mask,prefill,mutation,o->model.count,o->appearance.count);
    free(o);free(n);free(ram);return ok;
}
int main(int argc,char**argv){
    if(argc!=3){fprintf(stderr,"Usage: player-startup-original-check.exe originalDLL savedRAM\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    uint8_t*baseline=load(argv[2]);if(!baseline){FreeLibrary(h);return 4;}uint8_t*slot=ptr(baseline,0x803a9a40u-0x7bbcu,4);uint8_t*list=slot?ptr(baseline,be(slot),4):NULL;uint8_t*a=list?ptr(baseline,be(list),NFLSTREET_ACTOR_MODEL_STORAGE):NULL;
    if(!a){free(baseline);FreeLibrary(h);return 4;}nflstreet_player_constructor_input input={0};uint8_t descriptor[DESCRIPTOR_SIZE]={0};input.bytes=descriptor;input.size=sizeof descriptor;
    nflstreet_actor_model_descriptor*d=&input.models;d->skeleton=resource(baseline,be(a+0x64));d->primary[0]=resource(baseline,be(a+0xbc));d->primary[1]=resource(baseline,be(a+0x104));
    d->secondary[0]=resource(baseline,be(a+0x194));d->secondary[1]=resource(baseline,be(a+0x210));d->composite=resource(baseline,be(a+0x70));d->initial_word_bits=be(ptr(baseline,0x8025283cu,4));
    put(descriptor+0x1c,be(a+0x135c));for(unsigned i=0;i<7;++i)put(descriptor+0x20+i*4,be(a+0x400+i*4));memcpy(descriptor+0x40,a+0x1360,5);
    input.tail.repeated_float_bits=be(ptr(baseline,0x80252654u,4));input.tail.default_float_bits=be(ptr(baseline,0x802526b0u,4));for(unsigned i=0;i<4;++i)input.tail.defaults[i]=be(ptr(baseline,0x80270c84u+4*i,4));
    uint32_t indexed[2],maps[2][25],side_textures[2]={0x11220000u,0x11220001u},finish[25][7];
    for(unsigned i=0;i<2;++i){indexed[i]=be(ptr(baseline,0x8028dd64u+4*i,4));for(unsigned j=0;j<25;++j)maps[i][j]=be(ptr(baseline,0x8028cf08u+100*i+4*j,4));}
    for(unsigned i=0;i<25;++i)for(unsigned j=0;j<7;++j)finish[i][j]=be(ptr(baseline,0x8028dd90u+28*i+4*j,4));
    nflstreet_player_appearance_catalog catalog={indexed,2,maps,2,be(ptr(baseline,0x80268d88u,4)),0x8028d480u,side_textures,2,finish,25};
    int ok=1;for(unsigned mask=0;mask<64&&ok;++mask)for(unsigned prefill=0;prefill<2&&ok;++prefill)for(unsigned mutation=0;mutation<2&&ok;++mutation)ok=startup_case(m,baseline,&input,&catalog,mask,prefill,mutation);
    free(baseline);FreeLibrary(h);if(ok)puts("Joined original800E1F7C-801639C8 startup differential passed256 cases; required unresolved resource/render services explicit, no complete fresh player or playable match.");return ok?0:5;
}
