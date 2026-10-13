#define main model_stage_main
#include "actor_model_init_original_check.c"
#undef main
#define NFLSTREET_QUICK_ROSTER_ORACLE
#include "quick_match_roster_check.c"
static const ModernGekkoModuleDesc *flag_module;static uint8_t *flag_ram;
static int initialize_flag_oracle(const char *dll,const char *snapshot){HMODULE h=LoadLibraryA(dll);if(!h)return 0;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get)return 0;flag_module=get();flag_ram=load(snapshot);return flag_module&&flag_ram;}
static int flag_oracle(uint32_t prior,unsigned side,int selected,uint32_t expected){
 CPUState c={0};c.ram=flag_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x810f0000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 c.gpr[3]=0x81160000u;c.gpr[4]=side;c.gpr[5]=(uint32_t)selected;c.pc=0x8014fd50u;unsigned reads=0,writes=0;
 for(unsigned step=0;c.pc&&step<10000;++step){if(c.pc==0x80149614u){if(c.gpr[3]!=0x81160000u||c.gpr[4]!=0x47494c50u)return 0;
   ++reads;c.gpr[3]=prior;c.pc=c.lr;continue;}
  if(c.pc==0x8014982cu){if(c.gpr[3]!=0x81160000u||c.gpr[4]!=0x47494c50u||c.gpr[5]!=expected)return 0;++writes;c.pc=c.lr;continue;}
  c.downcount=-1000000000;flag_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 return !c.pc&&reads==1&&writes==1;
}
