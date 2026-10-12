#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#include "face_texture.h"
#define TEXTURE_DESC 0x816d0000u
static const ModernGekkoModuleDesc *texture_module;
static uint8_t *texture_ram;
static unsigned texture_cases,texture_models;
static uint8_t texture_seen[65536];
static int initialize_texture_oracle(const char *dll,const char *snapshot){
 HMODULE h=LoadLibraryA(dll);if(!h)return 0;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get)return 0;texture_module=get();texture_ram=load(snapshot);return texture_module&&texture_ram;
}
static void put16(uint8_t *p,unsigned value){p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value;}
static int install_texture_hashes(const nflstreet_archive *a){
 size_t at=be(a->bytes+4);unsigned found=0;
 while(at<a->size){if(a->size-at<8)return 0;size_t size=be(a->bytes+at+4);if(size<8||size>a->size-at)return 0;
  if(!memcmp(a->bytes+at,"HSH1",4)){
   if(found++||size<24||(size_t)a->count*4>size-24||be(a->bytes+at+8)!=1||
      be(a->bytes+at+12)!=4||be(a->bytes+at+16)!=4||!ptr(texture_ram,0x816d0100u,(size_t)a->count*4))return 0;
   memset(ptr(texture_ram,TEXTURE_DESC,0x50),0,0x50);put16(ptr(texture_ram,TEXTURE_DESC+0x14,2),a->count);
   put(ptr(texture_ram,TEXTURE_DESC+0x1c,4),1);put(ptr(texture_ram,TEXTURE_DESC+0x44,4),0x816d0050u);
   put(ptr(texture_ram,TEXTURE_DESC+0x48,4),0x816d0070u);put(ptr(texture_ram,TEXTURE_DESC+0x4c,4),0x816d0100u);
   put(ptr(texture_ram,0x816d0050u,4),1);put(ptr(texture_ram,0x816d0058u,4),4);
   put(ptr(texture_ram,0x816d0070u,4),be(a->bytes+at+20));
   memcpy(ptr(texture_ram,0x816d0100u,(size_t)a->count*4),a->bytes+at+24,(size_t)a->count*4);
  }at+=size;
 }
 put(ptr(texture_ram,0x803a9a40u-0x5accu,4),TEXTURE_DESC);*ptr(texture_ram,0x803a9a40u-0x6538u,1)=1;return found==1;
}
static int original_request(const char *name,const nflstreet_face_texture_plan *plan,uint8_t record[24],unsigned force){
 uint32_t slot=2,record_token=0x802799ecu+slot*600u+24u;
 if(strlen(name)>=128)return 0;memcpy(ptr(texture_ram,0x81500000u,strlen(name)+1),name,strlen(name)+1);
 memcpy(ptr(texture_ram,record_token,24),record,24);
 CPUState c={0};c.ram=texture_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
 c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=slot;c.gpr[4]=0x81500000u;c.gpr[5]=force;c.pc=0x800e3104u;
 unsigned queries=0;char expected[40];if(snprintf(expected,37,"PLATEX_FACE_%s",name)<0)return 0;
 char *space=strchr(expected,' ');if(space)*space=0;
 for(unsigned step=0;c.pc&&step<200000;++step){
  if(c.pc==0x801a8088u){const char *key=(const char*)ptr(texture_ram,c.gpr[4],40);
   if(!key||!memchr(key,0,40)||c.gpr[3]!=TEXTURE_DESC||queries>1||strcmp(key,queries?"PLATEX_TEMPLATE_FACE":expected))return 0;++queries;}
  if(c.pc==0x801af944u||c.pc==0x801af9ccu||c.pc==0x801af57cu){c.gpr[3]=0;c.pc=c.lr;continue;}
  c.downcount=-1000000000;texture_module->dispatch(&c,c.pc);if(c.exception)return 0;
 }
 if(c.pc||queries!=1u+plan->fallback||!nflstreet_texture_request(record,24,plan->index,UINT32_MAX,force)||
    memcmp(record,ptr(texture_ram,record_token,24),24))return 0;++texture_cases;return 1;
}
static int original_mmap_call(uint32_t token){
 CPUState c={0};c.ram=texture_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
 c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=token;c.pc=0x801c5610u;
 for(unsigned step=0;c.pc&&step<200000;++step){c.downcount=-1000000000;texture_module->dispatch(&c,c.pc);if(c.exception)return 0;}return !c.pc;
}
static int original_face_texture(const char *name,const nflstreet_face_texture_resource *resource){
 uint8_t record[24];for(unsigned i=0;i<24;++i)record[i]=(uint8_t)(i*7+3);
 put(record,UINT32_MAX);put(record+16,UINT32_MAX);
 if(!original_request(name,&resource->plan,record,0)||!original_request(name,&resource->plan,record,0))return 0;
 record[12]=9;record[13]=8;if(!original_request(name,&resource->plan,record,1))return 0;
 unsigned index=(unsigned)resource->plan.index;if(texture_seen[index])return 1;
 if(!ptr(texture_ram,resource->token,resource->size))return 0;
 memcpy(ptr(texture_ram,resource->token,resource->size),resource->raw,resource->size);
 if(!original_mmap_call(resource->token)||memcmp(resource->processed,ptr(texture_ram,resource->token,resource->size),resource->size)||
    !original_mmap_call(resource->token)||memcmp(resource->processed,ptr(texture_ram,resource->token,resource->size),resource->size))return 0;
 uint8_t *repeat=malloc(resource->size);if(!repeat)return 0;memcpy(repeat,resource->processed,resource->size);
 int ok=nflstreet_mmap_relocate(repeat,resource->size,resource->token)&&!memcmp(repeat,resource->processed,resource->size);
 free(repeat);if(!ok)return 0;texture_seen[index]=1;++texture_models;return 1;
}
static int original_texture_guards(const nflstreet_archive *archive){
 nflstreet_face_texture_plan plan;if(!nflstreet_face_texture_plan_make(archive,"DAYWATER_MISSING_FACE",&plan)||!plan.fallback)return 0;
 uint8_t record[24];memset(record,0xa5,24);if(!original_request("DAYWATER_MISSING_FACE",&plan,record,0))return 0;
 uint8_t before[24];memcpy(before,record,24);
 if(!nflstreet_texture_request(record,24,-1,0,1)||memcmp(before,record,24))return 0;
 /* Exercise original negative-index no-op directly, preserving every byte. */
 uint32_t dest=0x802799ecu+2*600u+24u;memcpy(ptr(texture_ram,dest,24),before,24);
 CPUState c={0};c.ram=texture_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 c.gpr[3]=2;c.gpr[4]=1;c.gpr[5]=UINT32_MAX;c.gpr[6]=0;c.gpr[7]=1;c.pc=0x800e2e48u;
 for(unsigned step=0;c.pc&&step<100000;++step){c.downcount=-1000000000;texture_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 if(c.pc||memcmp(before,ptr(texture_ram,dest,24),24))return 0;++texture_cases;return 1;
}
#define NFLSTREET_FACE_TEXTURE_ORACLE 1
#ifdef NFLSTREET_FACE_APPLY
#include "texture_apply_check_helpers.h"
#endif
#include "face_texture_check.c"
