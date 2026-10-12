#define NFLSTREET_MODEL_VIEW_HELPERS_ONLY 1
#include "model_view_original_check.c"
#include "model_relocate.h"
#include "model_chunk_relocate.h"
static int original_relocate(const ModernGekkoModuleDesc *m,uint8_t *ram,uint32_t base){
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
    c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=base;c.pc=0x801c5ec8u;
    for(unsigned step=0;c.pc&&step<2000000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"relocation original pc%08x ex%08x\n",c.pc,c.exception);return 0;}}
    return !c.pc;
}
int main(int argc,char **argv){
    if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get)return 3;const ModernGekkoModuleDesc *m=get();uint8_t *baseline=load(argv[2]);size_t archive_size=0;
    uint8_t *archive=file_bytes(argv[3],&archive_size);if(!m||!baseline||!archive)return 4;
    uint32_t count=be(ptr(baseline,0x803a9a40u-0x5688u,4)),table=be(ptr(baseline,0x803a9a40u-0x568cu,4));
    nflstreet_model_chunk_registration registry[64];if(count>64)return 4;
    for(unsigned i=0;i<count;++i){uint8_t *r=ptr(baseline,table+i*12,12);if(!r)return 4;
        registry[i]=(nflstreet_model_chunk_registration){be(r),be(r+4),be(r+8)?nflstreet_model_chunk_relocate:NULL};}
    size_t default_count=0;const nflstreet_model_chunk_registration *defaults=nflstreet_model_default_registry(&default_count);
    if(default_count!=count)return 4;
    for(unsigned i=0;i<count;++i)if(defaults[i].tag!=registry[i].tag||defaults[i].version!=registry[i].version||!!defaults[i].handler!=!!registry[i].handler)return 4;
    nflstreet_body_resource_pool pool={0};if(!nflstreet_body_resource_pool_open(archive,archive_size,0x10000000u,&pool))return 5;
    unsigned cases=0;
    for(unsigned entry=0;entry<9;++entry)for(unsigned variant=0;variant<4;++variant){
        nflstreet_team_body_binding *asset=pool.assets+entry;uint8_t *native=malloc(asset->size),*ram=malloc(RAM_SIZE);
        if(!native||!ram)return 4;memcpy(native,asset->bytes,asset->size);memcpy(ram,baseline,RAM_SIZE);
        unsigned directory=be16(native+10);
        if(variant==1)put(native+directory,0x12345678u);
        if(variant==2)native[directory+12]^=0x80;
        if(variant==3)put(native+directory+8,0);
        memcpy(ptr(ram,SOURCE,asset->size),native,asset->size);
        int ok=original_relocate(m,ram,SOURCE)&&nflstreet_model_relocate(native,asset->size,SOURCE,registry,count)&&
            !memcmp(native,ptr(ram,SOURCE,asset->size),asset->size);
        if(!ok){fprintf(stderr,"relocate mismatch entry%u variant%u\n",entry,variant);
            for(size_t j=0;j<asset->size;++j)if(native[j]!=ptr(ram,SOURCE,asset->size)[j]){fprintf(stderr,"firstbyte%zx native%02x original%02x\n",j,native[j],ptr(ram,SOURCE,asset->size)[j]);break;}return 6;}
        uint8_t *once=malloc(asset->size);if(!once)return 4;memcpy(once,native,asset->size);
        ok=original_relocate(m,ram,SOURCE)&&nflstreet_model_relocate(native,asset->size,SOURCE,registry,count)&&
            !memcmp(native,once,asset->size)&&!memcmp(native,ptr(ram,SOURCE,asset->size),asset->size);
        uint8_t view[64]={0},initial[64]={0};unsigned ignored=0;nflstreet_model_view_source source={native,asset->size,SOURCE};
        ok=ok&&original_view(m,ram,native,asset->size,initial,0x801c67e4u,0,&ignored)&&
            nflstreet_model_view_attach(view,sizeof view,&source)&&!memcmp(view,ptr(ram,VIEW,64),64);
        if(!ok)return 7;++cases;free(once);free(native);free(ram);
    }
    nflstreet_body_resource_pool processed={0};if(!nflstreet_body_resource_pool_open(archive,archive_size,SOURCE,&processed))return 8;
    uint8_t *ram=malloc(RAM_SIZE);if(!ram)return 4;memcpy(ram,baseline,RAM_SIZE);
    for(unsigned i=0;i<9;++i){uint8_t *dst=ptr(ram,processed.tokens[i],processed.assets[i].size);if(!dst)return 8;memcpy(dst,processed.assets[i].bytes,processed.assets[i].size);}
    if(!nflstreet_body_resource_pool_process(&processed))return 8;
    for(unsigned i=0;i<9;++i){
        if(!original_relocate(m,ram,processed.tokens[i])||memcmp(ptr(ram,processed.tokens[i],processed.assets[i].size),processed.assets[i].bytes,processed.assets[i].size))return 8;
        uint8_t initial[64]={0};unsigned ignored=0;
        if(!original_view(m,ram,processed.assets[i].bytes,processed.assets[i].size,initial,0x801c67e4u,0,&ignored)||memcmp(ptr(ram,VIEW,52),processed.views[i],52))return 8;
    }
    printf("Relocated body models: %u whole files +%u repeated no-op calls +%u attached views +9 processed pooled assets exact.\n",cases,cases,cases);
    free(ram);nflstreet_body_resource_pool_close(&processed);
    nflstreet_body_resource_pool_close(&pool);free(baseline);free(archive);FreeLibrary(h);return 0;
}
