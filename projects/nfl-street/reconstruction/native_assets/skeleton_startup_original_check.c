#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#include "skeleton_startup.h"
static uint8_t *load_data(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");if(!f||fseek(f,0,SEEK_END))return NULL;long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET))return NULL;
 uint8_t *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n)return NULL;fclose(f);*size=(size_t)n;return b;
}
int main(int argc,char **argv){
 if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc *m=get();uint8_t *ram=load(argv[2]);size_t an=0,cn=0;
 uint8_t *archive_bytes=load_data(argv[3],&an),*catalog_bytes=load_data(argv[4],&cn);
 nflstreet_archive archive;nflstreet_skeleton_startup_catalog catalog;nflstreet_skeleton_startup bundle={0};
 if(!m||!ram||!archive_bytes||!catalog_bytes||!nflstreet_archive_open(archive_bytes,an,&archive)||
    !nflstreet_skeleton_startup_catalog_open(catalog_bytes,cn,&catalog)||
    !nflstreet_skeleton_startup_open(&archive,&catalog,0x81000000u,&bundle))return 4;
 if(memcmp(ptr(ram,0x80246490u,384),catalog.base_positions,384)||
    memcmp(ptr(ram,0x80246610u,12),catalog.hand_positions,12)||
    memcmp(ptr(ram,0x8024661cu,114),catalog.hair_positions,114)||
    be(ptr(ram,0x80246690u,4))!=catalog.scale_word)return 5;
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
 c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.pc=0x80041108u;
 unsigned requests=0;uint32_t handle=be(ptr(ram,0x803a9a40u-0x50bcu,4));
 for(unsigned step=0;c.pc&&step<2000000;++step){
  if(c.pc==0x801a69c0u){
   if(requests>=6||c.gpr[3]!=handle||c.gpr[4]!=bundle.resources[requests].archive_index||c.gpr[5]!=1)return 6;
   nflstreet_archive_entry entry;uint8_t *raw=NULL;size_t size=0;const nflstreet_skeleton_resource *r=bundle.resources+requests;
   if(!nflstreet_archive_get(&archive,r->archive_index,&entry)||!nflstreet_archive_decode(&entry,&raw,&size)||
      size!=r->size||!ptr(ram,r->token,size))return 6;
   memcpy(ptr(ram,r->token,size),raw,size);free(raw);c.gpr[3]=r->token;c.pc=c.lr;++requests;continue;
  }c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"startup exception %08x\n",c.pc);return 7;}
 }
 if(c.pc||requests!=6)return 7;
 for(unsigned i=0;i<6;++i){const nflstreet_skeleton_resource *r=bundle.resources+i;
  if(be(ptr(ram,0x803a9a40u-0x7bd4u+4*i,4))!=r->token||memcmp(ptr(ram,r->token,r->size),r->bytes,r->size)){
   fprintf(stderr,"startup resource mismatch entry%u\n",r->archive_index);return 8;}}
 const uint32_t outputs[]={0x8026c2acu,0x8026c2b0u,0x8026c2b4u,0x8026c2a0u,0x8026c2a4u,0x8026c2a8u};
 for(unsigned i=0;i<6;++i)if(be(ptr(ram,outputs[i],4))!=bundle.base_vectors[i])return 9;
 /* Missing final asset must release partial construction and retain output. */
 nflstreet_archive short_archive=archive;short_archive.count=101;
 nflstreet_skeleton_startup marker={0},rejected;marker.base_vectors[0]=0x12345678u;rejected=marker;
 if(nflstreet_skeleton_startup_open(&short_archive,&catalog,0x81000000u,&rejected)||memcmp(&marker,&rejected,sizeof marker))return 10;
 printf("Skeleton startup: 6 acquisitions, 6 complete initialized files, 6 published references, 6 vector words exact; partial-load rejection.\n");
 nflstreet_skeleton_startup_close(&bundle);free(archive_bytes);free(catalog_bytes);free(ram);FreeLibrary(h);return 0;
}
