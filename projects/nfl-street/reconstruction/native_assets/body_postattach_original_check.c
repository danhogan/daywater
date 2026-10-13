#define main body_actor_fixture_main
#include "actor_model_init_original_check.c"
#undef main
#include "body_postattach.h"
#include "model_relocate.h"
#include "model_view.h"
#include "skeleton_startup.h"
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
    unsigned made,frees,fail_at;
    uint32_t free_order[PA_ALLOCATIONS];
} pa_allocations;
static int pa_allocate(void *opaque,uint32_t count,uint32_t width,
                       nflstreet_model_postattach_segment *out){
    pa_allocations *a=opaque;if((a->fail_at&&a->made+1==a->fail_at)||a->made>=PA_ALLOCATIONS||(uint64_t)count*width>PA_STRIDE)return 0;
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

static uint8_t *file_bytes(const char *name,size_t *size){FILE *f=fopen(name,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t *p=malloc((size_t)n);if(!p||fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return p;}
static int compare_allocations(const pa_allocations *a,const pa_allocations *b){
 if(a->made!=b->made||a->frees!=b->frees||a->made!=3||a->frees!=3)return 0;
 for(unsigned i=0;i<3;++i)if(a->block[i].count!=b->block[i].count||a->block[i].width!=b->block[i].width||a->block[i].token!=b->block[i].token||
  a->block[i].freed!=b->block[i].freed||memcmp(a->block[i].storage,b->block[i].storage,PA_STRIDE)||a->free_order[i]!=b->free_order[i])return 0;
 return 1;
}
int main(int argc,char **argv){
 if(argc!=6)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc *module=get();uint8_t *baseline=load(argv[2]);size_t size[3];uint8_t *data[3];
 for(unsigned i=0;i<3;++i)if(!(data[i]=file_bytes(argv[3+i],size+i)))return 3;
 nflstreet_archive animations;nflstreet_skeleton_startup_catalog catalog;nflstreet_skeleton_startup skeletons={0};
 if(!module||!baseline||!nflstreet_archive_open(data[1],size[1],&animations)||!nflstreet_skeleton_startup_catalog_open(data[2],size[2],&catalog)||
    !nflstreet_skeleton_startup_open(&animations,&catalog,0x81400000u,&skeletons))return 3;
 const nflstreet_skeleton_resource *target=skeletons.resources;unsigned cases=0,guards=0;
 uint8_t *ram=malloc(RAM_SIZE),*target_copy=malloc(target->size);if(!ram||!target_copy)return 3;memcpy(target_copy,target->bytes,target->size);
 for(unsigned k=0;k<4;++k)for(unsigned variant=0;variant<3;++variant){uint32_t tbyp=k==3?7u:k;
  nflstreet_team_body_binding raw={0};uint8_t view[52]={0};const uint32_t model_token=0x81000000u,view_token=0x81500000u;
  size_t registrations;const nflstreet_model_chunk_registration *registry=nflstreet_model_default_registry(&registrations);
  if(!nflstreet_team_body_binding_open(data[0],size[0],tbyp,variant,&raw)||!nflstreet_model_relocate(raw.bytes,raw.size,model_token,registry,registrations))return 4;
  nflstreet_model_view_source source={raw.bytes,raw.size,model_token};if(!nflstreet_model_view_attach(view,52,&source))return 4;
  memcpy(ram,baseline,RAM_SIZE);memcpy(ptr(ram,model_token,raw.size),raw.bytes,raw.size);memcpy(ptr(ram,view_token,52),view,52);
  memcpy(ptr(ram,target->token,target->size),target->bytes,target->size);
  pa_allocations original={0},native={0};nflstreet_owned_body body={0};
  if(!original_bind(module,ram,view_token,target->token,&original)||
     !nflstreet_owned_body_open(data[0],size[0],tbyp,variant,model_token,view_token,target,pa_allocate,pa_release,&native,&body))return 5;
  if(body.file.size!=raw.size||memcmp(body.file.bytes,ptr(ram,model_token,raw.size),raw.size)||memcmp(body.view,ptr(ram,view_token,52),52)||
     memcmp(target->bytes,target_copy,target->size)||memcmp(target->bytes,ptr(ram,target->token,target->size),target->size)||!compare_allocations(&original,&native)){
    fprintf(stderr,"Body comparison mismatch type%u variant%u alloc%u/%u free%u/%u\n",tbyp,variant,original.made,native.made,original.frees,native.frees);
    for(size_t d=0;d<raw.size;++d)if(body.file.bytes[d]!=ptr(ram,model_token,raw.size)[d]){fprintf(stderr,"file byte +%zx original=%02x native=%02x\n",d,ptr(ram,model_token,raw.size)[d],body.file.bytes[d]);break;}
    for(unsigned d=0;d<52;++d)if(body.view[d]!=ptr(ram,view_token,52)[d]){fprintf(stderr,"view byte +%x original=%02x native=%02x\n",d,ptr(ram,view_token,52)[d],body.view[d]);break;}
    for(unsigned a=0;a<original.made&&a<native.made;++a){fprintf(stderr,"allocation%u count%u/%u width%u/%u freed%u/%u\n",a,original.block[a].count,native.block[a].count,original.block[a].width,native.block[a].width,original.block[a].freed,native.block[a].freed);
      for(unsigned d=0;d<PA_STRIDE;++d)if(original.block[a].storage[d]!=native.block[a].storage[d]){fprintf(stderr,"storage byte +%x original=%02x native=%02x\n",d,original.block[a].storage[d],native.block[a].storage[d]);break;}}
    return 6;
  }
  nflstreet_model_postattach_segment segments[2]={{body.model_token,body.file.bytes,body.file.size},{body.view_token,body.view,52}};
  if(!original_bind(module,ram,view_token,0,&original)||!nflstreet_model_postattach_bind(body.view_token,0,segments,2,pa_allocate,pa_release,&native)||
     memcmp(body.file.bytes,ptr(ram,model_token,raw.size),raw.size)||memcmp(body.view,ptr(ram,view_token,52),52)||!compare_allocations(&original,&native))return 6;
  nflstreet_owned_body prior=body;pa_allocations untouched={0};if(nflstreet_owned_body_open(data[0],size[0],tbyp,variant,model_token,view_token,target,
     pa_allocate,pa_release,&untouched,&body)||memcmp(&prior,&body,sizeof body)||untouched.made){return 7;}
  ++guards;
  if(!cases){for(unsigned fail=1;fail<=3;++fail){pa_allocations faults={0};faults.fail_at=fail;nflstreet_owned_body out={0},saved=out;
    if(nflstreet_owned_body_open(data[0],size[0],tbyp,variant,model_token,view_token,target,pa_allocate,pa_release,&faults,&out)||
       memcmp(&out,&saved,sizeof out)||faults.made!=faults.frees||memcmp(target->bytes,target_copy,target->size)){return 7;}
  ++guards;
   }
   pa_allocations faults={0};nflstreet_owned_body out={0},saved=out;
   if(nflstreet_owned_body_open(data[0],size[0],tbyp,variant,model_token,model_token+16u,target,pa_allocate,pa_release,&faults,&out)||
      memcmp(&out,&saved,sizeof out)||faults.made){return 7;}
  ++guards;
  }
  nflstreet_owned_body_close(&body);nflstreet_owned_body_close(&body);nflstreet_team_body_binding_close(&raw);++cases;
 }
 free(ram);free(target_copy);nflstreet_skeleton_startup_close(&skeletons);free(baseline);for(unsigned i=0;i<3;++i)free(data[i]);FreeLibrary(h);
 printf("Owned body bindings: %u type/variant cases (TBYP0/1/2/7 x3); 24 original801C6B38 calls; whole files/views and target bytes exact; 36 temporary allocations, storage and release order exact; %u failure/reuse guards; target0 explicit, no actor creation.\n",cases,guards);return 0;
}
