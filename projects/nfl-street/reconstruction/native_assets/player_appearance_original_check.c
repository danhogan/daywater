#include "player_appearance_check_fixture.h"
static int appearance_original(const ModernGekkoModuleDesc*m,uint8_t*ram,const uint8_t*initial,const uint8_t*descriptor,appearance_trace*t){
    memcpy(ptr(ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),initial,NFLSTREET_ACTOR_MODEL_STORAGE);memcpy(ptr(ram,DESCRIPTOR,DESCRIPTOR_SIZE),descriptor,DESCRIPTOR_SIZE);
    put(ptr(ram,MODEL_TOKEN+0x18,4),NODE_LIST_TOKEN);put(ptr(ram,0x815d0200u+0x18,4),0x11330000u);for(unsigned i=0;i<2;++i)put(ptr(ram,SIDE_TOKEN+i*0x1000u+0x18,4),0x11220000u+i);put(ptr(ram,0x803a9a40u-0x58e8u,4),2);put(ptr(ram,0x803a9a40u-0x6238u,4),3);
    memset(ptr(ram,REGISTRY_TOKEN,REGISTRY_SIZE),0xa5,REGISTRY_SIZE);memset(ptr(ram,BINDINGS_TOKEN,REGISTRY_SIZE),0xa5,REGISTRY_SIZE);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=ACTOR;c.gpr[4]=DESCRIPTOR;c.pc=0x801639c8u;c.cycle_budget=200000000;
    for(unsigned step=0;c.pc&&step<300000;++step){
        int handled=appearance_boundary(&c,ram,t);if(handled<0)return 0;if(handled)continue;
        if(c.pc==0x801c9784u){if(c.gpr[3]!=45||c.gpr[4]!=4||t->node_allocations++)return 0;memset(ptr(ram,NODE_HEAP,512),0xa5,512);c.gpr[3]=NODE_HEAP;c.pc=c.lr;continue;}
        if(c.pc==0x8018b7b8u){if(c.gpr[3]!=256||c.gpr[4]||t->channel_allocations++)return 0;memset(ptr(ram,CHANNEL_HEAP,512),0xa5,512);c.gpr[3]=CHANNEL_HEAP;c.pc=c.lr;continue;}
        c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"appearance exception %08x pc%08x\n",c.exception,c.pc);return 0;}
    }
    if(c.pc)return 0;memcpy(t->node_memory,ptr(ram,NODE_HEAP,512),512);memcpy(t->channel_memory,ptr(ram,CHANNEL_HEAP,512),512);return 1;
}
static int appearance_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const uint8_t*source,unsigned prefill,unsigned mutation,unsigned variant){
    uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    uint8_t descriptor[DESCRIPTOR_SIZE];memcpy(descriptor,source,sizeof descriptor);if(variant){descriptor[0x40]=1;descriptor[0x41]=3;descriptor[0x42]=1;descriptor[0x43]=1;descriptor[0x44]=2;put(descriptor+0x38,1);}
    uint8_t initial[NFLSTREET_ACTOR_MODEL_STORAGE],native[NFLSTREET_ACTOR_MODEL_STORAGE];for(unsigned i=0;i<sizeof initial;++i)initial[i]=prefill?(uint8_t)(i*29+0x49):0;memcpy(native,initial,sizeof native);
    appearance_trace o={0},n={0};o.mutation=n.mutation=mutation;
    if(!appearance_original(m,ram,initial,descriptor,&o)){free(ram);return 0;}
    uint32_t catalog_words[2],maps[2][25],side_textures[2]={0x11220000u,0x11220001u},finish[25][7];for(unsigned i=0;i<25;++i)for(unsigned j=0;j<7;++j)finish[i][j]=be(ptr(ram,0x8028dd90u+i*28+j*4,4));for(unsigned i=0;i<2;++i){catalog_words[i]=be(ptr(ram,0x8028dd64u+4*i,4));for(unsigned j=0;j<25;++j)maps[i][j]=be(ptr(ram,0x8028cf08u+i*100+4*j,4));}
    uint8_t actors[REGISTRY_SIZE],bindings[REGISTRY_SIZE];memset(actors,0xa5,sizeof actors);memset(bindings,0xa5,sizeof bindings);
    nflstreet_player_appearance_catalog catalog={catalog_words,2,maps,2,be(ptr(ram,0x80268d88u,4)),0x8028d480u,side_textures,2,finish,25};
    nflstreet_player_appearance_registry registry={actors,sizeof actors,REGISTRY_TOKEN,2,bindings,sizeof bindings,3};
    nflstreet_player_appearance_context context={ACTOR,NODE_LIST_TOKEN,&catalog,&registry,appearance_service,&n,appearance_allocate,&n};
    int ok=nflstreet_player_appearance_initialize(native,sizeof native,descriptor,sizeof descriptor,&context)&&
        !memcmp(native,ptr(ram,ACTOR,sizeof native),sizeof native)&&!memcmp(&o,&n,sizeof o)&&
        !memcmp(actors,ptr(ram,REGISTRY_TOKEN,REGISTRY_SIZE),REGISTRY_SIZE)&&!memcmp(bindings,ptr(ram,BINDINGS_TOKEN,REGISTRY_SIZE),REGISTRY_SIZE)&&
        registry.actor_count==be(ptr(ram,0x803a9a40u-0x58e8u,4))&&registry.binding_count==be(ptr(ram,0x803a9a40u-0x6238u,4));
    if(!ok){fprintf(stderr,"appearance mismatch prefill%u mutation%u variant%u requests%u/%u\n",prefill,mutation,variant,o.count,n.count);
        for(unsigned i=0;i<sizeof native;++i)if(native[i]!=ptr(ram,ACTOR,sizeof native)[i]){fprintf(stderr,"actorbyte%04x original%02x native%02x\n",i,ptr(ram,ACTOR,sizeof native)[i],native[i]);break;}
        for(unsigned i=0;i<o.count&&i<n.count;++i)if(memcmp(o.entries+i,n.entries+i,sizeof(request_entry))){fprintf(stderr,"request%u kind%u/%u args%08x,%08x/%08x,%08x\n",i,o.entries[i].request.kind,n.entries[i].request.kind,o.entries[i].request.arguments[0],o.entries[i].request.arguments[1],n.entries[i].request.arguments[0],n.entries[i].request.arguments[1]);break;}}
    else printf("appearance prefilled=%u mutation=%u variant=%u: actor1A90 +two allocations +two registries +%u requests exact\n",prefill,mutation,variant,o.count);
    free(ram);return ok;
}
int main(int argc,char**argv){
    if(argc!=3){fprintf(stderr,"Usage: player-appearance-original-check.exe originalDLL savedRAM\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    uint8_t*baseline=load(argv[2]);if(!baseline){FreeLibrary(h);return 4;}uint8_t*slot=ptr(baseline,0x803a9a40u-0x7bbcu,4);uint8_t*list=slot?ptr(baseline,be(slot),4):NULL;uint8_t*a=list?ptr(baseline,be(list),NFLSTREET_ACTOR_MODEL_STORAGE):NULL;
    if(!a){free(baseline);FreeLibrary(h);return 4;}uint8_t descriptor[DESCRIPTOR_SIZE]={0};put(descriptor+4,be(a+0x64));
    for(unsigned i=0;i<7;++i)put(descriptor+0x20+i*4,be(a+0x400+i*4));memcpy(descriptor+0x40,a+0x1360,5); /* Inverse source-owned writes, not a fresh captured descriptor. */
    int ok=1;for(unsigned prefill=0;prefill<2&&ok;++prefill)for(unsigned mutation=0;mutation<2&&ok;++mutation)for(unsigned variant=0;variant<2&&ok;++variant)ok=appearance_case(m,baseline,descriptor,prefill,mutation,variant);
    free(baseline);FreeLibrary(h);if(ok)puts("Original801639C8 and owned small helpers appearance differential passed8 cases; source-backed catalog, explicit unresolved model/node/texture services, no complete renderer or player.");return ok?0:5;
}
