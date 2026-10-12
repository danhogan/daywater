#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#include "skeleton_resource.h"
static int original_skeleton(const ModernGekkoModuleDesc *m,uint8_t *ram,uint32_t token){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
 c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=token;c.pc=0x8017eb24u;
 for(unsigned step=0;c.pc&&step<1000000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;}
 return !c.pc;
}
int main(int argc,char **argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get)return 3;const ModernGekkoModuleDesc *m=get();uint8_t *baseline=load(argv[2]);FILE *f=fopen(argv[3],"rb");
 if(!m||!baseline||!f)return 4;if(fseek(f,0,SEEK_END))return 4;long length=ftell(f);if(length<=0||fseek(f,0,SEEK_SET))return 4;
 uint8_t *bytes=malloc((size_t)length);if(!bytes||fread(bytes,1,(size_t)length,f)!=(size_t)length)return 4;fclose(f);
 nflstreet_archive archive;if(!nflstreet_archive_open(bytes,(size_t)length,&archive))return 4;
 const unsigned indices[]={95,96,97,99,100,101};
 for(unsigned i=0;i<6;++i){
  const uint32_t token=0x81000000u;nflstreet_skeleton_resource r={0};nflstreet_archive_entry entry;
  uint8_t *raw=NULL,*ram=malloc(RAM_SIZE);size_t size=0;
  if(!ram||!nflstreet_skeleton_resource_open(&archive,indices[i],token,&r)||!nflstreet_archive_get(&archive,indices[i],&entry)||
   !nflstreet_archive_decode(&entry,&raw,&size)||!ptr(ram,token,size))return 5;
  memcpy(ram,baseline,RAM_SIZE);memcpy(ptr(ram,token,size),raw,size);free(raw);
  if(!original_skeleton(m,ram,token)||memcmp(r.bytes,ptr(ram,token,size),size)){
   fprintf(stderr,"SKL1 mismatch entry%u\n",indices[i]);return 6;}
  if(!original_skeleton(m,ram,token)||!nflstreet_skeleton_relocate(r.bytes,r.size,r.token)||memcmp(r.bytes,ptr(ram,token,size),size))return 7;
  printf("SKL1 entry%u: whole relocated resource and repeated no-op exact\n",indices[i]);
  free(ram);nflstreet_skeleton_resource_close(&r);
 }
 free(bytes);free(baseline);FreeLibrary(h);return 0;
}
