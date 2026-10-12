/* Original 801C6B38 comparison on independently decoded face resources. */
#define main actor_model_check_main
#include "actor_model_init_original_check.c"
#undef main
#include "model_postattach.h"
#include "face_resource_pair.h"
#include "skeleton_resource.h"
static nflstreet_skeleton_resource fresh_target;

#define PA_HEAP 0x81600000u
#define PA_STRIDE 0x4000u
#define PA_ALLOCATIONS 3u
typedef struct {
    uint32_t count,width,token;
    unsigned freed;
    uint8_t storage[PA_STRIDE];
} pa_block;
typedef struct {
    pa_block block[PA_ALLOCATIONS];
    unsigned made,frees;
    uint32_t free_order[PA_ALLOCATIONS];
} pa_allocations;
static int pa_allocate(void *opaque,uint32_t count,uint32_t width,
                       nflstreet_model_postattach_segment *out){
    pa_allocations *a=opaque;if(a->made>=PA_ALLOCATIONS||(uint64_t)count*width>PA_STRIDE)return 0;
    pa_block *b=&a->block[a->made];b->count=count;b->width=width;
    b->token=PA_HEAP+a->made*PA_STRIDE;
    memset(b->storage,0xa5,sizeof b->storage);
    out->token=b->token;out->bytes=b->storage;out->size=sizeof b->storage;
    ++a->made;return 1;
}
static void pa_release(void *opaque,uint32_t token){
    pa_allocations *a=opaque;
    if(a->frees<PA_ALLOCATIONS)a->free_order[a->frees++]=token;
    for(unsigned i=0;i<a->made;++i)if(a->block[i].token==token)++a->block[i].freed;
}
static int original_bind(const ModernGekkoModuleDesc *m,uint8_t *ram,
                         uint32_t view,uint32_t target,pa_allocations *a){
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
    c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
    c.gpr[3]=view;c.gpr[4]=target;c.pc=0x801c6b38u;c.cycle_budget=200000000;
    for(unsigned step=0;c.pc&&step<2000000;++step){
        if(c.pc==0x801c9784u){
            nflstreet_model_postattach_segment out={0};
            if(!pa_allocate(a,c.gpr[3],c.gpr[4],&out))return 0;
            memset(ptr(ram,out.token,PA_STRIDE),0xa5,PA_STRIDE);
            c.gpr[3]=out.token;c.pc=c.lr;continue;
        }
        if(c.pc==0x801c97c4u){
            pa_release(a,c.gpr[3]);c.pc=c.lr;continue;
        }
        c.downcount=-1000000000;m->dispatch(&c,c.pc);
        if(c.exception){fprintf(stderr,"postattach exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}
    }
    if(c.pc)return 0;
    for(unsigned i=0;i<a->made;++i)
        memcpy(a->block[i].storage,ptr(ram,a->block[i].token,PA_STRIDE),PA_STRIDE);
    return 1;
}
static int check_face(const ModernGekkoModuleDesc *m,const uint8_t *baseline,
                      const nflstreet_face_resource_pair *pair,unsigned face,unsigned context){
    uint8_t *original_ram=malloc(RAM_SIZE),*native_ram=malloc(RAM_SIZE);
    pa_allocations *oa=calloc(1,sizeof *oa),*na=calloc(1,sizeof *na);
    if(!original_ram||!native_ram||!oa||!na)return 0;
    memcpy(original_ram,baseline,RAM_SIZE);memcpy(native_ram,baseline,RAM_SIZE);
    uint32_t token=pair->tokens[face],view=0x81500000u+face*0x100u;
    uint32_t target=fresh_target.token;
    if(!ptr(original_ram,target,fresh_target.size))return 0;
    memcpy(ptr(original_ram,target,fresh_target.size),fresh_target.bytes,fresh_target.size);
    memcpy(ptr(native_ram,target,fresh_target.size),fresh_target.bytes,fresh_target.size);
    if(!target||!ptr(original_ram,target,16)||!ptr(original_ram,token,pair->sizes[face])||
       !ptr(original_ram,view,52))return 0;
    memcpy(ptr(original_ram,token,pair->sizes[face]),pair->bytes[face],pair->sizes[face]);
    memcpy(ptr(native_ram,token,pair->sizes[face]),pair->bytes[face],pair->sizes[face]);
    memcpy(ptr(original_ram,view,52),pair->views[face],52);
    memcpy(ptr(native_ram,view,52),pair->views[face],52);
    nflstreet_model_postattach_segment segment={BASE,native_ram,RAM_SIZE};
    int good=original_bind(m,original_ram,view,target,oa)&&
        nflstreet_model_postattach_bind(view,target,&segment,1,pa_allocate,pa_release,na);
    if(good){
        if(memcmp(ptr(original_ram,token,pair->sizes[face]),ptr(native_ram,token,pair->sizes[face]),pair->sizes[face])||
           memcmp(ptr(original_ram,view,52),ptr(native_ram,view,52),52)||
           oa->made!=na->made||oa->frees!=na->frees)good=0;
    }
    if(good)for(unsigned i=0;i<oa->made;++i){
        pa_block *ob=&oa->block[i],*nb=&na->block[i];
        if(ob->count!=nb->count||ob->width!=nb->width||ob->token!=nb->token||
           ob->freed!=nb->freed||memcmp(ob->storage,nb->storage,PA_STRIDE))good=0;
    }
    if(good)for(unsigned i=0;i<oa->frees;++i)
        if(oa->free_order[i]!=na->free_order[i])good=0;
    if(good)good=original_bind(m,original_ram,view,0,oa)&&
        nflstreet_model_postattach_bind(view,0,&segment,1,pa_allocate,pa_release,na)&&
        oa->made==3&&na->made==3&&oa->frees==3&&na->frees==3&&
        !memcmp(ptr(original_ram,token,pair->sizes[face]),ptr(native_ram,token,pair->sizes[face]),pair->sizes[face])&&
        !memcmp(ptr(original_ram,view,52),ptr(native_ram,view,52),52);
    if(!good){
        fprintf(stderr,"context%u face%u postattach mismatch alloc%u/%u free%u/%u\n",
                context,face,oa->made,na->made,oa->frees,na->frees);
        for(size_t i=0;i<pair->sizes[face];++i)
            if(ptr(original_ram,token,pair->sizes[face])[i]!=ptr(native_ram,token,pair->sizes[face])[i]){
                fprintf(stderr,"first resource byte+%zx %02x/%02x\n",i,
                   ptr(original_ram,token,pair->sizes[face])[i],ptr(native_ram,token,pair->sizes[face])[i]);break;}
    }else printf("context%u face%u: whole mutated file/view, 3 allocations, storage, free order and repeated no-op exact\n",context,face);
    free(oa);free(na);free(original_ram);free(native_ram);return good;
}
int main(int argc,char **argv){
    if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get)return 3;const ModernGekkoModuleDesc *m=get();uint8_t *baseline=load(argv[2]);
    FILE *f=fopen(argv[3],"rb");if(!m||!baseline||!f)return 4;
    if(fseek(f,0,SEEK_END))return 4;long length=ftell(f);if(length<=0||fseek(f,0,SEEK_SET))return 4;
    uint8_t *archive_bytes=malloc((size_t)length);
    if(!archive_bytes||fread(archive_bytes,1,(size_t)length,f)!=(size_t)length)return 4;
    fclose(f);nflstreet_archive archive={0};
    if(!nflstreet_archive_open(archive_bytes,(size_t)length,&archive))return 4;
    f=fopen(argv[4],"rb");if(!f||fseek(f,0,SEEK_END))return 4;long target_length=ftell(f);
    if(target_length<=0||fseek(f,0,SEEK_SET))return 4;uint8_t *target_bytes=malloc((size_t)target_length);
    if(!target_bytes||fread(target_bytes,1,(size_t)target_length,f)!=(size_t)target_length)return 4;fclose(f);
    nflstreet_archive target_archive;if(!nflstreet_archive_open(target_bytes,(size_t)target_length,&target_archive)||
       !nflstreet_skeleton_resource_open(&target_archive,95,0x81400000u,&fresh_target))return 4;
    nflstreet_face_resource_pair pair={0};
    if(!nflstreet_face_resource_pair_open(&archive,12,0x81000000u,&pair))return 4;
    int ok=1;unsigned context=(unsigned)atoi(argv[2]+strlen(argv[2])-7);
    for(unsigned i=0;i<2&&ok;++i)ok=check_face(m,baseline,&pair,i,context);
    nflstreet_face_resource_pair_close(&pair);nflstreet_skeleton_resource_close(&fresh_target);
    free(target_bytes);free(archive_bytes);free(baseline);FreeLibrary(h);
    return ok?0:5;
}
