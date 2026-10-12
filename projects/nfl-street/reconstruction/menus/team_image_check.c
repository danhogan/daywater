#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define OBJECT 0x817d1000u
#define ARGUMENT 0x817d2000u
#define RESULT 0x817d3000u
#define STACK 0x817df000u
#define SENTINEL 0xabcddcbau

typedef struct {uint16_t flags;uint32_t packed;} image_case;
static const image_case cases[]={
 {0x0010u,0xffffffffu}, /* flag10 forces assignment even for both sentinel halves */
 {0x0020u,0xffffffffu}, /* both sentinel halves leave image identity intact */
 {0x0020u,0xffff1234u}, /* one sentinel half leaves identity intact */
 {0x0020u,0x1234ffffu}, /* the other sentinel half leaves identity intact */
 {0x0010u,0xffff1234u}, /* flag10 forces assignment for a mixed sentinel */
 {0x0020u,0x12345678u}  /* valid archive/entry pair */
};
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t*load_file(const char*path,size_t*size){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(unsigned long)z>RAM_SIZE||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}
static void seed(uint8_t*ram,const image_case*t){for(unsigned i=0;i<36;++i)ram[OBJECT-BASE+i]=(uint8_t)(0x81u+i*7u);ram[OBJECT-BASE]=(uint8_t)(t->flags>>8);ram[OBJECT-BASE+1]=(uint8_t)t->flags;put(ram+ARGUMENT-BASE,t->packed);put(ram+RESULT-BASE,SENTINEL);}
static int original(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t*ret){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 c.gpr[3]=OBJECT;c.gpr[4]=7;c.gpr[5]=0;c.gpr[6]=ARGUMENT;c.gpr[7]=RESULT;c.pc=0x801d60dc;c.lr=0;c.cycle_budget=10000000;
 for(unsigned step=0;c.pc&&step<10000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"Original class0 image exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}}
 if(c.pc||c.gpr[1]!=STACK){fprintf(stderr,"Original class0 image stopped pc=%08x SP=%08x\n",c.pc,c.gpr[1]);return 0;}*ret=c.gpr[3];return 1;
}
static int compare_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,unsigned index){
 uint8_t*actual=malloc(RAM_SIZE),*native_ram=malloc(RAM_SIZE);if(!actual||!native_ram)return 0;memcpy(actual,baseline,RAM_SIZE);memcpy(native_ram,baseline,RAM_SIZE);const image_case*t=&cases[index];seed(actual,t);seed(native_ram,t);
 uint8_t before[36];memcpy(before,actual+OBJECT-BASE,sizeof before);uint32_t ret=0;if(!original(m,actual,&ret)){free(actual);free(native_ram);return 0;}
 nflstreet_team_screen state={0};state.supplied_static_archives=UINT32_MAX;nflstreet_ui_service_request q={0};q.kind=0;q.class_id=0;q.target=OBJECT;q.property=7;q.argc=1;q.args[0]=t->packed;
 uint32_t result=SENTINEL;if(!nflstreet_team_screen_service(&state,native_ram,RAM_SIZE,BASE,&q,&result)||result!=SENTINEL){fprintf(stderr,"Native team class0 property7 rejected case%u\n",index);free(actual);free(native_ram);return 0;}
 if(memcmp(actual+OBJECT-BASE,native_ram+OBJECT-BASE,sizeof before)||ret!=OBJECT||be(actual+RESULT-BASE)!=SENTINEL||be(native_ram+RESULT-BASE)!=SENTINEL){fprintf(stderr,"Class0 image object/result differs case%u ret=%08x\n",index,ret);free(actual);free(native_ram);return 0;}
 uint16_t flags=(uint16_t)(((uint16_t)actual[OBJECT-BASE]<<8)|actual[OBJECT-BASE+1]);if(flags!=(uint16_t)(t->flags|0x4000u)){fprintf(stderr,"Original class0 flags differ case%u\n",index);free(actual);free(native_ram);return 0;}
 int write_pair=(t->flags&0x10u)!=0||((t->packed>>16)!=0xffffu&&(t->packed&0xffffu)!=0xffffu);
 uint16_t expected_at8=write_pair?(uint16_t)t->packed:(uint16_t)((before[8]<<8)|before[9]);
 uint16_t expected_at10=write_pair?(uint16_t)(t->packed>>16):(uint16_t)((before[10]<<8)|before[11]);
 if((((uint16_t)actual[OBJECT-BASE+8]<<8)|actual[OBJECT-BASE+9])!=expected_at8||(((uint16_t)actual[OBJECT-BASE+10]<<8)|actual[OBJECT-BASE+11])!=expected_at10){fprintf(stderr,"Original class0 packed write differs case%u\n",index);free(actual);free(native_ram);return 0;}
 free(actual);free(native_ram);return 1;
}
static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}
int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"Usage: team-image-check.exe <original-DLL> <snapshot.mem1>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t z=0;uint8_t*baseline=load_file(argv[2],&z);if(!baseline||z!=RAM_SIZE)return 4;uint64_t hash=hash_bytes(baseline,z);
 for(unsigned i=0;i<sizeof cases/sizeof cases[0];++i)if(!compare_case(m,baseline,i)){fprintf(stderr,"Team class0 property7 comparison failed case%u flags%04x packed%08x\n",i,cases[i].flags,cases[i].packed);return 5;}
 size_t after_size=0;uint8_t*after=load_file(argv[2],&after_size);if(!after||after_size!=z||hash_bytes(after,after_size)!=hash){fprintf(stderr,"Snapshot baseline changed during check\n");return 6;}
 printf("Team class0 image property7 original exact: %u cases through 801D60DC and native team_screen_service; full36-byte object, conditional packed pair assignment, flags|4000, result sentinel and SP checked. Baseline FNV64 %016llx unchanged. Image resource loading/rendering not claimed.\n",(unsigned)(sizeof cases/sizeof cases[0]),(unsigned long long)hash);
 free(after);free(baseline);FreeLibrary(h);return 0;
}
