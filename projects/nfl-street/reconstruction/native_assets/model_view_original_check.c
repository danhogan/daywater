#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#include "body_resource_pool.h"
#define SOURCE 0x81500000u
#define VIEW 0x817b0000u
static uint8_t *file_bytes(const char *path,size_t *size){
    FILE *f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
    long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return b;
}
static int original_view(const ModernGekkoModuleDesc *m,uint8_t *ram,const uint8_t *bytes,
    size_t size,const uint8_t initial[64],uint32_t function,uint32_t tag,unsigned *found){
    if(size>0x100000u)return 0;memcpy(ptr(ram,SOURCE,size),bytes,size);memcpy(ptr(ram,VIEW,64),initial,64);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
    c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=VIEW;c.gpr[4]=SOURCE;c.gpr[5]=tag;c.pc=function;
    for(unsigned step=0;c.pc&&step<100000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;}
    *found=c.gpr[3];return !c.pc;
}
int main(int argc,char **argv){
    if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get)return 3;const ModernGekkoModuleDesc *m=get();uint8_t *baseline=load(argv[2]);size_t archive_size=0;
    uint8_t *archive=file_bytes(argv[3],&archive_size);if(!m||m->cpu_state_size!=sizeof(CPUState)||!baseline||!archive)return 4;
    nflstreet_body_resource_pool pool={0};if(!nflstreet_body_resource_pool_open(archive,archive_size,0x10000000u,&pool))return 5;
    unsigned cases=0;
    for(unsigned entry=0;entry<9;++entry)for(unsigned prefill=0;prefill<2;++prefill)for(unsigned variant=0;variant<4;++variant){
        nflstreet_team_body_binding *asset=pool.assets+entry;uint8_t *bytes=malloc(asset->size),*ram=malloc(RAM_SIZE);
        if(!bytes||!ram)return 4;memcpy(bytes,asset->bytes,asset->size);memcpy(ram,baseline,RAM_SIZE);
        unsigned count=be16(bytes+8),directory=be16(bytes+10);if(count<2)return 5;
        if(variant==1)put(bytes+directory,0x12345678u); /* Missing Info; unknown found tag. */
        if(variant==2)put(bytes+directory+8,0); /* Empty first row must be skipped. */
        if(variant==3)memcpy(bytes+directory+16,bytes+directory,16); /* First nonempty duplicate wins. */
        uint8_t initial[64],native[64];for(unsigned i=0;i<64;++i)initial[i]=prefill?(uint8_t)(i*19+0x5d):0;
        memcpy(native,initial,64);unsigned ignored=0;nflstreet_model_view_source source={bytes,asset->size,SOURCE};
        int ok=original_view(m,ram,bytes,asset->size,initial,0x801c67e4u,0,&ignored)&&
            nflstreet_model_view_attach(native,64,&source)&&!memcmp(native,ptr(ram,VIEW,64),64);
        if(!prefill&&!variant)ok=ok&&!memcmp(native,pool.views[entry],52);
        const uint32_t tags[]={0x496e666fu,0x12345678u,0x47656f6du,0xffffffffu};
        for(unsigned j=0;j<4&&ok;++j){unsigned found=0,original_found=0;memcpy(native,initial,64);
            ok=original_view(m,ram,bytes,asset->size,initial,0x801c6644u,tags[j],&original_found)&&
                nflstreet_model_view_apply(native,64,&source,tags[j],&found)&&found==original_found&&!memcmp(native,ptr(ram,VIEW,64),64);}
        if(!ok){fprintf(stderr,"model view mismatch entry%u prefill%u variant%u\n",entry,prefill,variant);return 6;}
        ++cases;free(bytes);free(ram);
    }
    for(unsigned v=0;v<3;++v){const nflstreet_team_body_binding *a=NULL;const uint8_t *view=NULL;
        if(!nflstreet_body_resource_pool_get(&pool,7,v,&a,&view)||a!=pool.assets+6+v||view!=pool.views[6+v])return 7;}
    printf("Fresh body model views: %u full attachment cases +%u direct tag cases +9 pooled views +3 alias queries exact.\n",cases,cases*4);
    nflstreet_body_resource_pool_close(&pool);free(baseline);free(archive);FreeLibrary(h);return 0;
}
