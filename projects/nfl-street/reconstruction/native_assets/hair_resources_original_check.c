/* Original hair planning plus whole-file relocation/attachment comparisons.
   The original archive query receives the independently decoded HSH1 result;
   formatting, fallback arithmetic and special-state dispatch execute. */
#define main actor_model_check_main
#define load actor_snapshot_load
#include "actor_model_init_original_check.c"
#undef load
#undef main
#include "hair_resources.h"
static const ModernGekkoModuleDesc *hair_module;
static uint8_t *hair_ram;
static unsigned original_plans,original_models;
static uint8_t seen[2][4096];
static int initialize_oracle(const char *dll,const char *snapshot){
    HMODULE h=LoadLibraryA(dll);if(!h)return 0;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get)return 0;hair_module=get();hair_ram=actor_snapshot_load(snapshot);return hair_module&&hair_ram;
}
static void half_put(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static int original_plan(const nflstreet_archive *roster,const char *name,uint16_t plht,
    uint16_t phar,uint16_t state,const nflstreet_hair_plan *plan){
    half_put(ptr(hair_ram,0x802aaaa2u,2),plht);half_put(ptr(hair_ram,0x802aaaa4u,2),phar);
    half_put(ptr(hair_ram,0x802aaaa6u,2),state);
    size_t length=strlen(name);if(length>=64)return 0;
    memcpy(ptr(hair_ram,0x81500000u,length+1),name,length+1);
    memset(ptr(hair_ram,0x81500100u,12),0xa5,12);
    CPUState c={0};c.ram=hair_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
    c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
    c.gpr[3]=0;c.gpr[4]=0x81500000u;c.gpr[5]=0x81500100u;c.pc=0x800debd0u;
    unsigned queries=0;
    for(unsigned step=0;c.pc&&step<200000;++step){
        if(c.pc==0x801a8088u){
            if(c.gpr[3]!=be(ptr(hair_ram,0x803a9a40u-0x5adcu,4)))return 0;
            const char *key=(const char*)ptr(hair_ram,c.gpr[4],96);if(!key||!memchr(key,0,96))return 0;
            unsigned entry;c.gpr[3]=nflstreet_archive_find(roster,key,&entry)?entry:UINT32_MAX;
            c.pc=c.lr;++queries;continue;
        }
        c.downcount=-1000000000;hair_module->dispatch(&c,c.pc);if(c.exception)return 0;
    }
    if(c.pc||queries!=((state==11||state==12||!name[0])?0u:2u))return 0;
    for(unsigned i=0;i<3;++i)if(be(ptr(hair_ram,0x81500100u+4*i,4))!=plan->indices[i]){
        fprintf(stderr,"hair plan mismatch state%u slot%u\n",state,i);return 0;}
    ++original_plans;return 1;
}
static int original_call(uint32_t pc,uint32_t a,uint32_t b){
    CPUState c={0};c.ram=hair_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
    c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
    c.gpr[3]=a;c.gpr[4]=b;c.pc=pc;
    for(unsigned step=0;c.pc&&step<200000;++step){c.downcount=-1000000000;hair_module->dispatch(&c,c.pc);if(c.exception)return 0;}
    return !c.pc;
}
static int original_resources(const nflstreet_archive *local,const nflstreet_archive *roster,
    const nflstreet_hair_plan *plan){
    for(unsigned i=0;i<3;++i){uint32_t index=plan->indices[i];if(!index)continue;
        unsigned r=index>50000u;if(r)index-=50000u;if(index>=4096)return 0;if(seen[r][index])continue;
        const nflstreet_archive *archive=r?roster:local;nflstreet_archive_entry entry;uint8_t *raw=NULL;size_t size=0;
        if(!nflstreet_archive_get(archive,index,&entry)||!nflstreet_archive_decode(&entry,&raw,&size)||!ptr(hair_ram,0x81000000u,size))return 0;
        memcpy(ptr(hair_ram,0x81000000u,size),raw,size);free(raw);
        nflstreet_hair_plan single={{plan->indices[i],0,0}};nflstreet_hair_resources native={0};
        if(!nflstreet_hair_resources_open(local,roster,&single,0x81000000u,&native))return 0;
        memset(ptr(hair_ram,0x81500200u,52),0,52);
        if(!original_call(0x801c5ec8u,0x81000000u,0)||
           !original_call(0x801c67e4u,0x81500200u,0x81000000u)||
           memcmp(native.bytes[0],ptr(hair_ram,0x81000000u,size),size)||
           memcmp(native.views[0],ptr(hair_ram,0x81500200u,52),52)){
            fprintf(stderr,"hair resource mismatch archive%u index%u\n",r,index);return 0;}
        nflstreet_hair_resources_close(&native);seen[r][index]=1;++original_models;
    }return 1;
}
#define NFLSTREET_HAIR_ORACLE 1
#include "selected_hair_resources_check.c"
