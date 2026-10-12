/* Original field extraction and headwear decision; table-query services use
   fresh original-extracted GEAR records. Full original DB registration is not
   reconstructed by this controlled comparison. */
#define main actor_model_check_main
#define load actor_snapshot_load
#include "actor_model_init_original_check.c"
#undef load
#undef main
#include "headwear_state.h"
static const ModernGekkoModuleDesc *hw_module;
static uint8_t *hw_ram;
static uint32_t gear_values[512][3];
static unsigned gear_rows,field_checks,selector_checks,map_checks;
static int cpu_call(uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t e,uint32_t *result){
 CPUState c={0};c.ram=hw_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
 c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=d;c.gpr[6]=e;c.pc=pc;
 for(unsigned step=0;c.pc&&step<100000;++step){c.downcount=-1000000000;hw_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 if(result)*result=c.gpr[3];return !c.pc;
}
static int original_number(const nflstreet_team_table *t,unsigned row,unsigned fi,uint32_t *result){
 nflstreet_team_field f;uint32_t native;
 if(!nflstreet_team_table_field(t,fi,&f)||f.type!=3||t->record_bytes>4096||
    !nflstreet_team_table_number(t,row,fi,&native))return 0;
 memcpy(ptr(hw_ram,0x81500000u,t->record_bytes),t->bytes+t->records_at+(size_t)row*t->record_bytes,t->record_bytes);
 memset(ptr(hw_ram,0x81500000u+t->record_bytes,8),0,8);put(ptr(hw_ram,0x81502000u,4),0xa5a5a5a5u);
 if(!cpu_call(0x801b94c8u,0x81500000u,f.bit_offset,f.width,0x81502000u,NULL))return 0;
 *result=be(ptr(hw_ram,0x81502000u,4));if(*result!=native)return 0;++field_checks;return 1;
}
static int initialize_headwear_oracle(const char *dll,const char *snapshot,const nflstreet_headwear_catalog *catalog){
 HMODULE h=LoadLibraryA(dll);if(!h)return 0;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get)return 0;hw_module=get();hw_ram=actor_snapshot_load(snapshot);if(!hw_module||!hw_ram||catalog->table.used>512)return 0;
 gear_rows=catalog->table.used;
 for(unsigned i=0;i<gear_rows;++i)
  if(!original_number(&catalog->table,i,catalog->id_field,&gear_values[i][0])||
     !original_number(&catalog->table,i,catalog->type_field,&gear_values[i][1])||
     !original_number(&catalog->table,i,catalog->state_field,&gear_values[i][2]))return 0;
 for(unsigned i=0;i<64;++i){uint32_t value;
  if(!cpu_call(0x800dde18u,i,0,0,0,&value)||value!=nflstreet_headwear_map(i))return 0;++map_checks;}
 return 1;
}
static int original_selection(const uint8_t *db,size_t size,const nflstreet_headwear_selection *selection){
 nflstreet_team_table t;if(!nflstreet_team_table_open_tag(db,size,0x59414c50u,&t))return 0;
 uint32_t ids[2]={0};unsigned fields[2]={UINT32_MAX,UINT32_MAX};
 for(unsigned i=0;i<t.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&t,i,&f))return 0;
  if(f.tag==0x54525350u)fields[0]=i;if(f.tag==0x54414850u)fields[1]=i;}
 for(unsigned i=0;i<2;++i)if(fields[i]==UINT32_MAX||!original_number(&t,selection->play_row,fields[i],ids+i))return 0;
 if(ids[0]!=selection->gear9_id||ids[1]!=selection->gear0_id)return 0;
 put(ptr(hw_ram,0x802aaa94u,4),ids[0]);put(ptr(hw_ram,0x802aaa6cu,4),ids[1]);
 CPUState c={0};c.ram=hw_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
 c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=0;c.pc=0x800ddebc;
 uint32_t chosen=0;unsigned queries=0,reads=0;
 for(unsigned step=0;c.pc&&step<100000;++step){
  if(c.pc==0x8014b8a0u){
   unsigned q=queries++;uint32_t type=q?0:9,id=ids[q?1:0];
   if(q>=2||c.gpr[3]!=0x802ab03cu||c.gpr[4]!=type||c.gpr[5]!=id||c.gpr[6])return 0;
   unsigned found=0;for(unsigned i=0;i<gear_rows;++i)if(gear_values[i][0]==id&&gear_values[i][1]==type){chosen=gear_values[i][2];++found;}
   if(found!=1||chosen!=(q?selection->gear0_value:selection->gear9_value))return 0;c.gpr[3]=1;c.pc=c.lr;continue;
  }
  if(c.pc==0x8014ba6cu){if(c.gpr[3]!=0x802ab03cu||reads>=queries)return 0;++reads;c.gpr[3]=chosen;c.pc=c.lr;continue;}
  c.downcount=-1000000000;hw_module->dispatch(&c,c.pc);if(c.exception)return 0;
 }
 if(c.pc||c.gpr[3]!=selection->state||queries!=1u+selection->queried_gear0||reads!=queries)return 0;
 ++selector_checks;return 1;
}
#define NFLSTREET_AUTOMATIC_HEADWEAR_ORACLE 1
#define NFLSTREET_AUTOMATIC_HEADWEAR 1
#include "selected_hair_resources_check.c"
