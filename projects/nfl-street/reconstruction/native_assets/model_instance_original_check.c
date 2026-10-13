#define main source_actor_fixture_main
#include "actor_model_init_original_check.c"
#undef main
#include "model_instance.h"
static int invoke(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t entry,uint32_t a,uint32_t b,uint32_t d,uint32_t e){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=d;c.gpr[6]=e;c.pc=entry;
 for(unsigned step=0;c.pc&&step<20000;++step){c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;}return !c.pc;
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();uint8_t*ram=load(argv[2]);if(!m||!ram)return 3;unsigned cases=0;
 for(unsigned fill=0;fill<4;++fill)for(unsigned mask=0;mask<8;++mask){uint32_t model=mask&1?0x80400000u:0,palette=mask&2?0x80500000u:0,materials=mask&4?0x80600000u:0;
  uint8_t native[240];for(unsigned i=0;i<240;++i)native[i]=fill==3?(uint8_t)(i*37+11):(fill==0?0:fill==1?0xa5:0xff);
  memcpy(ptr(ram,ACTOR,240),native,240);
  if(!invoke(m,ram,0x80162fc4u,ACTOR,0,palette,materials)||!invoke(m,ram,0x801ebfe0u,ACTOR,model,0,0)||
    !nflstreet_model_instance_initialize(native,model,palette,materials)||memcmp(native,ptr(ram,ACTOR,240),240))return 4;++cases;
 }
 free(ram);FreeLibrary(h);printf("Model instances: %u original initialize/select sequences, all240 bytes exact; 4 allocator fills x8 model/palette/material masks; original helpers executed; no full actor or draw.\n",cases);return 0;
}
