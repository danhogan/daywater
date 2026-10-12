#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#include "actor_factory.h"
#define FACTORY_TABLE 0x815b0000u
#define CONSTRUCTOR_TOKEN 0x817e1000u
#define OBJECT_SPAN (NFLSTREET_ACTOR_MODEL_STORAGE+16u)
typedef struct {
    uint32_t owner, size, calls, constructor_calls, descriptor, failure, mutation;
    uint8_t storage[OBJECT_SPAN], before_constructor[OBJECT_SPAN];
} factory_trace;
static int factory_allocate(void *opaque,uint32_t owner,uint32_t size,nflstreet_factory_memory *out){
    factory_trace *t=opaque;t->owner=owner;t->size=size;++t->calls;
    if(t->failure)return 0;
    *out=(nflstreet_factory_memory){ACTOR,t->storage,sizeof t->storage};return 1;
}
static void constructor(void *opaque,nflstreet_factory_memory *memory,const void *input){
    factory_trace *t=opaque;++t->constructor_calls;t->descriptor=*(const uint32_t *)input;
    memcpy(t->before_constructor,memory->bytes,sizeof t->before_constructor);
    if(t->mutation){memory->bytes[2]+=7;put(memory->bytes+0x14,t->descriptor);}
}
static int factory_original(const ModernGekkoModuleDesc *m,uint8_t *ram,uint32_t type,
    uint8_t header,uint32_t size,uint32_t owner,unsigned callback,factory_trace *t,uint32_t *result){
    put(ptr(ram,0x803a9a40u-0x4f14u,4),FACTORY_TABLE);
    uint8_t *record=ptr(ram,FACTORY_TABLE+type*20,20);memset(record,0,20);
    put(record,size);put(record+0x10,owner);if(callback)put(record+8,CONSTRUCTOR_TOKEN);
    memcpy(ptr(ram,ACTOR,OBJECT_SPAN),t->storage,OBJECT_SPAN);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
    c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=type;c.gpr[4]=header;c.gpr[5]=0x12345678u;
    c.pc=0x801956a4u;c.cycle_budget=200000000;
    for(unsigned step=0;c.pc&&step<100000;++step){
        if(c.pc==0x801812d4u){t->owner=c.gpr[3];t->size=size;++t->calls;c.gpr[3]=t->failure?0:ACTOR;c.pc=c.lr;continue;}
        if(c.pc==0x8017bc54u){if(c.gpr[3]!=ACTOR||c.gpr[4]||c.gpr[5]!=size)return 0;
            memset(ptr(ram,ACTOR,size),0,size);c.pc=c.lr;continue;}
        if(c.pc==CONSTRUCTOR_TOKEN){if(c.gpr[3]!=ACTOR)return 0;
            ++t->constructor_calls;t->descriptor=c.gpr[4];memcpy(t->before_constructor,ptr(ram,ACTOR,OBJECT_SPAN),OBJECT_SPAN);
            if(t->mutation){ptr(ram,ACTOR,OBJECT_SPAN)[2]+=7;put(ptr(ram,ACTOR+0x14,4),t->descriptor);}c.pc=c.lr;continue;}
        c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;
    }
    if(c.pc)return 0;*result=c.gpr[3];memcpy(t->storage,ptr(ram,ACTOR,OBJECT_SPAN),OBJECT_SPAN);return 1;
}
int main(int argc,char **argv){
    if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc *m=get();uint8_t *baseline=load(argv[2]);
    if(!m||m->cpu_state_size!=sizeof(CPUState)||!baseline){free(baseline);FreeLibrary(h);return 4;}
    uint32_t table=be(ptr(baseline,0x803a9a40u-0x4f14u,4));uint8_t *source=ptr(baseline,table,20);
    if(!source||be(source)!=NFLSTREET_ACTOR_MODEL_STORAGE){free(baseline);FreeLibrary(h);return 4;}
    uint32_t source_size=be(source),owner=be(source+0x10),input=0x12345678u;
    const unsigned types[]={0,1,255,256};unsigned cases=0;
    for(unsigned k=0;k<4;++k)for(unsigned header=0;header<2;++header)for(unsigned callback=0;callback<3;++callback)
    for(unsigned failure=0;failure<2;++failure)for(unsigned prefill=0;prefill<2;++prefill){
        uint8_t *ram=malloc(RAM_SIZE);if(!ram)return 4;memcpy(ram,baseline,RAM_SIZE);
        factory_trace o={0},n={0};o.failure=n.failure=failure;o.mutation=n.mutation=callback==2;
        for(unsigned j=0;j<OBJECT_SPAN;++j)o.storage[j]=n.storage[j]=prefill?(uint8_t)(j*13+0xa7):0;
        unsigned type=types[k],size=k?0x40:source_size;uint32_t original_result=0;
        nflstreet_factory_record records[257]={0};records[type]=(nflstreet_factory_record){size,owner,callback?constructor:NULL,&n};
        nflstreet_factory_memory result;int ok=factory_original(m,ram,type,(uint8_t)(header*255),size,owner,callback,&o,&original_result);
        int created=nflstreet_actor_create(records,257,type,(uint8_t)(header*255),&input,factory_allocate,&n,&result);
        ok=ok&&created==!failure&&result.token==original_result&&!memcmp(&o,&n,sizeof o);
        free(ram);if(!ok){fprintf(stderr,"factory mismatch type%u header%u callback%u fail%u prefill%u\n",type,header,callback,failure,prefill);free(baseline);FreeLibrary(h);return 5;}
        ++cases;
    }
    printf("Factory original801956A4: %u cases exact; full storage, allocation and constructor entry.\n",cases);
    free(baseline);FreeLibrary(h);return 0;
}
