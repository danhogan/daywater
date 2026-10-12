/* Controlled original 800E0AC4 acquisition path. Private assets and RAM only. */
#define main actor_model_check_main
#include "actor_model_init_original_check.c"
#undef main
#include "face_resource_pair.h"
#include "model_relocate.h"
#ifdef NFLSTREET_CHECK_BOUND_FACE
#include "face_postattach.h"
static nflstreet_skeleton_resource fresh_target;
static unsigned bind_count;
typedef struct {void *bytes[6];uint32_t tokens[6];unsigned made,freed;} temporaries;
static int allocate_temporary(void *opaque,uint32_t count,uint32_t width,nflstreet_model_postattach_segment *out){
    temporaries *t=opaque;if(t->made>=6||(uint64_t)count*width>0x10000u)return 0;
    unsigned i=t->made++;size_t size=(size_t)count*width;t->bytes[i]=malloc(size?size:1);
    if(!t->bytes[i])return 0;t->tokens[i]=0x40000000u+i*0x10000u;
    *out=(nflstreet_model_postattach_segment){t->tokens[i],t->bytes[i],size};return 1;
}
static void release_temporary(void *opaque,uint32_t token){
    temporaries *t=opaque;for(unsigned i=0;i<t->made;++i)if(t->tokens[i]==token&&t->bytes[i]){
        free(t->bytes[i]);t->bytes[i]=NULL;++t->freed;return;}
}
#endif

static nflstreet_archive face_archive;
static unsigned acquire_count,loader_count,relocate_count,attach_count;
static uint32_t loader_index[4],loader_buffer[4],attached_view[4],attached_buffer[4];
static size_t loader_size[4];
static int direct(const ModernGekkoModuleDesc *m,uint8_t *ram,uint32_t pc,uint32_t r3,uint32_t r4,uint32_t r5,uint32_t *result){
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
    c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=r3;c.gpr[4]=r4;c.gpr[5]=r5;c.pc=pc;
    for(unsigned step=0;c.pc&&step<2000000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;}
    if(result)*result=c.gpr[3];return !c.pc;
}

static int probe(const ModernGekkoModuleDesc *m,const uint8_t *baseline,unsigned index){
    uint8_t *ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    if(be(ptr(ram,0x803a9a40u-0x6234u,4)))return 0;
    put(ptr(ram,0x803a9a40u-0x6234u,4),0);
    #ifdef NFLSTREET_CHECK_BOUND_FACE
    if(!ptr(ram,fresh_target.token,fresh_target.size))return 0;
    memcpy(ptr(ram,fresh_target.token,fresh_target.size),fresh_target.bytes,fresh_target.size);
    put(ptr(ram,0x803a9a40u-0x7220u,4),fresh_target.token);
#else
    /* Historical relocation-only comparison excludes postattach binding. */
    put(ptr(ram,0x803a9a40u-0x7220u,4),0);
#endif
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
    c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
    c.gpr[3]=index;c.pc=0x800e0ac4u;c.cycle_budget=200000000;
    for(unsigned step=0;c.pc&&step<2000000;++step){
        if(c.pc==0x801eb3f4u){
            if(c.gpr[3]!=0x802ab29cu||c.gpr[5]!=0x8025300cu||c.gpr[6]||acquire_count++)return 0;
        }
        if(c.pc==0x801a75b0u){
            if(loader_count>=4)return 0;
            nflstreet_archive_entry e;uint8_t *bytes=NULL;size_t size=0;
            if(!nflstreet_archive_get(&face_archive,c.gpr[4],&e)||!nflstreet_archive_decode(&e,&bytes,&size)||!ptr(ram,c.gpr[5],size))return 0;
            memcpy(ptr(ram,c.gpr[5],size),bytes,size);free(bytes);
            loader_index[loader_count]=c.gpr[4];loader_buffer[loader_count]=c.gpr[5];loader_size[loader_count]=size;++loader_count;
            c.gpr[3]=0;c.pc=c.lr;continue;
        }
        #ifdef NFLSTREET_CHECK_BOUND_FACE
        if(c.pc==0x801c6b38u){if(c.gpr[4]!=fresh_target.token)return 0;++bind_count;}
#endif
        if(c.pc==0x801c5ec8u)++relocate_count;
        if(c.pc==0x801c67e4u){
            if(attach_count>=4)return 0;
            attached_view[attach_count]=c.gpr[3];attached_buffer[attach_count]=c.gpr[4];++attach_count;
        }
        c.downcount=-1000000000;m->dispatch(&c,c.pc);
        if(c.exception){fprintf(stderr,"exception pc%08x ex%08x\n",c.pc,c.exception);break;}
    }
    if(c.pc||acquire_count!=1||loader_count!=2||relocate_count!=2||attach_count!=2)return 0;
    #ifdef NFLSTREET_CHECK_BOUND_FACE
    if(bind_count!=2)return 0;
#endif
    uint32_t descriptor=be(ptr(ram,0x803a9a40u-0x722cu,4));
    if(descriptor!=0x802725f8u||be(ptr(ram,descriptor+0x0cu,4))!=index||
       be(ptr(ram,descriptor+0x5cu,4))!=index+1||be(ptr(ram,descriptor+0xacu,4))!=0)return 0;
    size_t registry_count=0;const nflstreet_model_chunk_registration *registry=nflstreet_model_default_registry(&registry_count);
    for(unsigned i=0;i<2;++i){
        if(loader_index[i]!=index+i||loader_buffer[i]!=attached_buffer[i]||attached_view[i]!=descriptor+0x1cu+i*0x50u)return 0;
        nflstreet_archive_entry e;uint8_t *native=NULL;size_t size=0;
        if(!nflstreet_archive_get(&face_archive,index+i,&e)||!nflstreet_archive_decode(&e,&native,&size)||size!=loader_size[i])return 0;
        nflstreet_model_view_source source={native,size,loader_buffer[i]};uint8_t view[52]={0};
        int relocated=nflstreet_model_relocate(native,size,loader_buffer[i],registry,registry_count);
        int viewed=relocated&&nflstreet_model_view_attach(view,sizeof view,&source);
#ifdef NFLSTREET_CHECK_BOUND_FACE
        temporaries temporary={0};uint32_t view_identity=0x816d0000u;
        nflstreet_model_postattach_segment segments[]={
            {loader_buffer[i],native,size},{view_identity,view,sizeof view},
            {fresh_target.token,fresh_target.bytes,fresh_target.size}};
        if(!viewed||!nflstreet_model_postattach_bind(view_identity,fresh_target.token,segments,3,
            allocate_temporary,release_temporary,&temporary)||temporary.made!=3||temporary.freed!=3)return 0;
#endif
        int same_bytes=relocated&&!memcmp(native,ptr(ram,loader_buffer[i],size),size);
        int same_view=viewed&&!memcmp(view,ptr(ram,attached_view[i],sizeof view),sizeof view);
        if(!same_bytes||!same_view){
            fprintf(stderr,"native/original resource mismatch entry%u relocated%d bytes%d viewed%d view%d\n",index+i,relocated,same_bytes,viewed,same_view);
            if(relocated)for(size_t j=0;j<size;++j)if(native[j]!=ptr(ram,loader_buffer[i],size)[j]){fprintf(stderr,"byte+%zx native%02x original%02x\n",j,native[j],ptr(ram,loader_buffer[i],size)[j]);break;}
            if(viewed)for(size_t j=0;j<52;++j)if(view[j]!=ptr(ram,attached_view[i],52)[j]){fprintf(stderr,"view+%zx native%02x original%02x\n",j,view[j],ptr(ram,attached_view[i],52)[j]);break;}
            free(native);return 0;}
        free(native);
    }
    nflstreet_face_resource_pair pair={0};
    if(!nflstreet_face_resource_pair_open(&face_archive,index,0x81000000u,&pair))return 0;
    #ifdef NFLSTREET_CHECK_BOUND_FACE
    temporaries pair_temporary={0};
    if(!nflstreet_face_resource_pair_bind(&pair,&fresh_target,0x81500000u,allocate_temporary,release_temporary,&pair_temporary)||
       pair_temporary.made!=6||pair_temporary.freed!=6)return 0;
#endif
    for(unsigned i=0;i<2;++i){
        uint32_t token=pair.tokens[i],view_token=0x81500000u+i*0x100u,result=0;
        if(pair.indices[i]!=index+i||!ptr(ram,token,pair.sizes[i]))return 0;
        nflstreet_archive_entry e;uint8_t *original=NULL;size_t size=0;
        if(!nflstreet_archive_get(&face_archive,index+i,&e)||!nflstreet_archive_decode(&e,&original,&size)||size!=pair.sizes[i])return 0;
        memcpy(ptr(ram,token,size),original,size);free(original);
        if(!direct(m,ram,0x801c5ec8u,token,0,0,&result))return 0;
        memset(ptr(ram,view_token,52),0,52);
        if(!direct(m,ram,0x801c67e4u,view_token,token,0,&result))return 0;
#ifdef NFLSTREET_CHECK_BOUND_FACE
        if(!direct(m,ram,0x801c6b38u,view_token,fresh_target.token,0,&result))return 0;
#endif
        if(memcmp(pair.bytes[i],ptr(ram,token,size),size)||memcmp(pair.views[i],ptr(ram,view_token,52),52))return 0;
        if(!direct(m,ram,0x800e0734u,4,0x12345678u,i,&result)||result!=attached_view[i])return 0;
    }
    for(unsigned i=0;i<2;++i)printf("face entry%u: original acquisition buffer%08x size%zu view%08x exact\n",index+i,loader_buffer[i],loader_size[i],attached_view[i]);
    nflstreet_face_resource_pair_close(&pair);free(ram);return 1;
}
int main(int argc,char **argv){
    #ifdef NFLSTREET_CHECK_BOUND_FACE
    if(argc!=5)return 2;
    FILE *af=fopen(argv[4],"rb");if(!af||fseek(af,0,SEEK_END))return 4;
    long an=ftell(af);if(an<=0||fseek(af,0,SEEK_SET))return 4;
    uint8_t *animation_bytes=malloc((size_t)an);nflstreet_archive animation;
    if(!animation_bytes||fread(animation_bytes,1,(size_t)an,af)!=(size_t)an)return 4;fclose(af);
    if(!nflstreet_archive_open(animation_bytes,(size_t)an,&animation)||
       !nflstreet_skeleton_resource_open(&animation,95,0x81400000u,&fresh_target))return 4;
    free(animation_bytes);
#else
    if(argc!=4)return 2;
#endif
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get)return 3;const ModernGekkoModuleDesc *m=get();uint8_t *baseline=load(argv[2]);
    size_t archive_size=0;uint8_t *archive_bytes=NULL;
    {FILE *f=fopen(argv[3],"rb");if(!f)return 4;if(fseek(f,0,SEEK_END))return 4;long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET))return 4;
     archive_bytes=malloc((size_t)n);if(!archive_bytes||fread(archive_bytes,1,(size_t)n,f)!=(size_t)n)return 4;fclose(f);archive_size=(size_t)n;}
    if(!m||!baseline||!nflstreet_archive_open(archive_bytes,archive_size,&face_archive))return 4;
    int ok=probe(m,baseline,12);
#ifdef NFLSTREET_CHECK_BOUND_FACE
    nflstreet_skeleton_resource_close(&fresh_target);
#endif
    free(archive_bytes);free(baseline);FreeLibrary(h);return ok?0:5;
}
