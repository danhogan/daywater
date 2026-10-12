/* Original 8014B918 records, 8014F60C/800DE000 player fields, 800DDBE8 and
   800E3218 sweeps, and complete 800DE3B8 plans. 801B1AC8 table search is
   supplied from fresh rows, but every item is filled by original
   801C4388/801B94C8 with the 801BFFDC type-0 terminator. 800E217C color,
   800E2E48 request-record and 800E3020 state services are recorded only. */
#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#include "gear_texture.h"
#define DB_RECORD 0x81580000u
#define CURSOR 0x81503000u
#define CURSOR_HANDLE 0x81503100u
#define SCRATCH_GEAR 0x81502000u
#define SCRATCH_PLAYER 0x81506000u
#define SCRATCH_OUT 0x81507000u
#define OBJECT 0x81504000u
#define COLOR_TARGET 0x5au
#define CURSOR_GLOBAL 0x802ab03cu
#define PLAYER_BASE 0x802aaa60u
static const ModernGekkoModuleDesc *gear_module;static uint8_t *gear_ram;
static unsigned gear_record_cases,player_rows,player_fields,kind_cases,fallback_cases,plan_cases,slot_writes,color_writes,gear_queries;
static const nflstreet_team_table *query_table;static unsigned query_row;static int query_ready;
typedef struct {
 const nflstreet_gear_records *records;const nflstreet_gear_texture_plan *plan;
 unsigned slot,queries,selections,colors,slots,states,bad;uint32_t channel,value;
} oracle_run;
static int initialize_gear_oracle(const char *dll,const char *snapshot){HMODULE h=LoadLibraryA(dll);if(!h)return 0;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 0;
 gear_module=get();gear_ram=load(snapshot);return gear_module&&gear_ram;}
/* Leaf original routine on a separate stack, between outer dispatch steps. */
static int leaf(uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t e,uint32_t f){
 CPUState c={0};c.ram=gear_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81780000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=d;c.gpr[6]=e;c.gpr[7]=f;c.pc=pc;
 for(unsigned step=0;c.pc&&step<100000;++step){c.downcount=-1000000000;gear_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 return !c.pc;
}
/* 801BFFDC item loop for the selected fresh row; a missing tag stops with 9. */
static int fill_items(uint32_t list,uint32_t *status){
 const nflstreet_team_table *t=query_table;if(!t||!query_ready||t->record_bytes>4096)return 0;
 uint8_t *record=ptr(gear_ram,DB_RECORD,t->record_bytes+8u);if(!record)return 0;
 memcpy(record,t->bytes+t->records_at+(size_t)query_row*t->record_bytes,t->record_bytes);memset(record+t->record_bytes,0,8);
 for(unsigned j=0;j<64;++j){uint32_t item=list+16u*j;uint8_t *p=ptr(gear_ram,item,16);if(!p)return 0;uint32_t tag=be(p+8);
  if(tag==UINT32_MAX){*status=0;return 1;}
  unsigned found=0,index=0;nflstreet_team_field f;
  for(unsigned i=0;i<t->field_count;++i){if(!nflstreet_team_table_field(t,i,&f))return 0;if(f.tag==tag){if(found++)return 0;index=i;}}
  if(!found){*status=9;return 1;}
  if(!nflstreet_team_table_field(t,index,&f))return 0;
  if(f.type<2){uint32_t to=be(p),count=f.width>>3;
   if(!ptr(gear_ram,to,count+1u)||!leaf(0x801c4388u,DB_RECORD,f.bit_offset,f.width,to,count))return 0;
   if(!f.type)*ptr(gear_ram,to+count,1)=0;}
  else if(f.type<4){if(!leaf(0x801b94c8u,DB_RECORD,f.bit_offset,f.width,item,f.type==2))return 0;}
  else return 0;
 }
 return 0;
}
static int hook(CPUState *c,oracle_run *o){
 uint32_t pc=c->pc;
 if(pc==0x801b1ac8u){uint32_t status;if(!fill_items(c->gpr[6],&status))return -1;++o->queries;c->gpr[3]=status;}
 else if(pc==0x8014b8a0u){unsigned row;
  if(!o->records||c->gpr[3]!=CURSOR_GLOBAL||c->gpr[6]||!nflstreet_gear_records_find(o->records,c->gpr[4],c->gpr[5],&row))return -1;
  if(o->plan&&(o->selections>=12||o->plan->gear_rows[o->selections]!=row))return -1;
  ++o->selections;query_row=row;query_ready=1;}
 else if(pc==0x80041e18u)c->gpr[3]=OBJECT;
 else if(pc==0x800e217cu){
  if(o->plan){const nflstreet_gear_color_request *r=o->plan->colors+o->colors;
   if(o->colors>=o->plan->color_count||c->gpr[3]!=COLOR_TARGET||c->gpr[4]!=r->channel||c->gpr[5]!=r->value)return -1;}
  else{if(c->gpr[3]!=COLOR_TARGET)return -1;o->channel=c->gpr[4];o->value=c->gpr[5];}
  ++o->colors;}
 else if(pc==0x800e2e48u){const nflstreet_gear_texture_plan *p=o->plan;unsigned i=o->slots;
  if(!p||c->gpr[3]!=o->slot||c->gpr[7])return -1;
  if(i<NFLSTREET_GEAR_TEXTURE_SLOTS){if(c->gpr[4]!=i||c->gpr[5]!=(uint32_t)p->indices[i]||c->gpr[6]!=p->palettes[i])return -1;}
  else if(i!=NFLSTREET_GEAR_TEXTURE_SLOTS||!p->fallback||c->gpr[4]||c->gpr[5]!=(uint32_t)p->fallback_index||c->gpr[6]!=p->fallback_palette)return -1;
  ++o->slots;}
 else if(pc==0x800e3020u){
  if(o->states++||o->slots!=NFLSTREET_GEAR_TEXTURE_SLOTS||c->gpr[3]!=o->slot||c->gpr[4]!=PLAYER_BASE+o->slot*0x68u+0x4cu)return -1;}
 else if(pc==0x801af944u||pc==0x801af9ccu)c->gpr[3]=0;
 else if(pc!=0x801af57cu)return 0;
 c->pc=c->lr;return 1;
}
static int execute(uint32_t pc,const uint32_t r[8],uint32_t stack8,uint32_t stack12,oracle_run *o,uint32_t *result){
 CPUState c={0};c.ram=gear_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 for(unsigned i=0;i<8;++i)c.gpr[3+i]=r[i];put(ptr(gear_ram,0x81700008u,4),stack8);put(ptr(gear_ram,0x8170000cu,4),stack12);c.pc=pc;
 for(unsigned step=0;c.pc&&step<5000000;++step){int h=hook(&c,o);if(h<0){fprintf(stderr,"oracle hook mismatch at %08x\n",c.pc);return 0;}
  if(h)continue;c.downcount=-1000000000;gear_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 if(result)*result=c.gpr[3];return !c.pc;
}
static int install_one(const nflstreet_archive *a,uint32_t desc){size_t at=be(a->bytes+4);unsigned found=0;
 while(at<a->size){if(a->size-at<8)return 0;size_t size=be(a->bytes+at+4);if(size<8||size>a->size-at)return 0;
  if(!memcmp(a->bytes+at,"HSH1",4)){if(found++||size<24||(size_t)a->count*4>size-24||be(a->bytes+at+8)!=1||be(a->bytes+at+12)!=4||be(a->bytes+at+16)!=4)return 0;
   memset(ptr(gear_ram,desc,0x80),0,0x80);uint8_t *p=ptr(gear_ram,desc+0x14,2);p[0]=(uint8_t)(a->count>>8);p[1]=(uint8_t)a->count;
   put(ptr(gear_ram,desc+0x1c,4),1);put(ptr(gear_ram,desc+0x44,4),desc+0x50);put(ptr(gear_ram,desc+0x48,4),desc+0x70);put(ptr(gear_ram,desc+0x4c,4),desc+0x100);
   put(ptr(gear_ram,desc+0x50,4),1);put(ptr(gear_ram,desc+0x58,4),4);put(ptr(gear_ram,desc+0x70,4),be(a->bytes+at+20));
   uint8_t *hashes=ptr(gear_ram,desc+0x100,(size_t)a->count*4);if(!hashes)return 0;memcpy(hashes,a->bytes+at+24,(size_t)a->count*4);
  }at+=size;
 }return found==1;
}
/* Fresh platex/pladyncl HSH1 descriptors at r13-5ACC/r13-5094, the slots the
   captured contexts register for these two archives. */
static int install_gear_hashes(const nflstreet_archive *a,const nflstreet_archive *b){
 if(!install_one(a,0x816d0000u)||!install_one(b,0x816e0000u))return 0;
 put(ptr(gear_ram,0x803a9a40u-0x5accu,4),0x816d0000u);put(ptr(gear_ram,0x803a9a40u-0x5094u,4),0x816e0000u);
 *ptr(gear_ram,0x803a9a40u-0x6538u,1)=1;put(ptr(gear_ram,CURSOR,4),CURSOR_HANDLE);put(ptr(gear_ram,CURSOR+4,4),0);
 uint8_t *object=ptr(gear_ram,OBJECT,0x1400);if(!object)return 0;object[0x1362]=COLOR_TARGET;return 1;
}
static int original_sweeps(void){
 static const uint32_t packed=0x11223344u;
 for(unsigned kind=0;kind<256;++kind)for(unsigned v=0;v<4;++v){
  uint32_t a4=0x37,a5=v==3?UINT32_MAX:0x12u,a6=v&1u,a7=v>>1;oracle_run o={0};
  put(ptr(gear_ram,SCRATCH_OUT,4),0xa5a5a5a5u);put(ptr(gear_ram,SCRATCH_OUT+4,4),0xa5a5a5a5u);put(ptr(gear_ram,SCRATCH_OUT+16,4),packed);
  const uint32_t r[8]={kind,0x15,SCRATCH_OUT+16,a4,a5,a6,a7,COLOR_TARGET};
  if(!execute(0x800ddbe8u,r,SCRATCH_OUT,SCRATCH_OUT+4,&o,NULL))return 0;
  int32_t index=(int32_t)0xa5a5a5a5u;uint32_t palette=0xa5a5a5a5u,value=0;
  int wrote=nflstreet_gear_texture_kind(kind,packed,a4,a5,a6,a7,&index,&palette,&value);
  if(be(ptr(gear_ram,SCRATCH_OUT,4))!=(uint32_t)index||be(ptr(gear_ram,SCRATCH_OUT+4,4))!=palette||o.colors!=(unsigned)wrote||
     (wrote&&(o.channel!=0x15||o.value!=value))){fprintf(stderr,"selector kind mismatch %u/%u\n",kind,v);return 0;}
  ++kind_cases;
 }
 for(uint32_t code=0;code<0x10000u;++code){oracle_run o={0};const uint32_t r[8]={code,SCRATCH_OUT,SCRATCH_OUT+4};
  if(!execute(0x800e3218u,r,0,0,&o,NULL))return 0;int32_t index;uint32_t palette;nflstreet_gear_texture_fallback(code,&index,&palette);
  if(be(ptr(gear_ram,SCRATCH_OUT,4))!=(uint32_t)index||be(ptr(gear_ram,SCRATCH_OUT+4,4))!=palette){fprintf(stderr,"fallback mismatch %u\n",code);return 0;}
  ++fallback_cases;
 }
 return 1;
}
static int original_gear_record(const nflstreet_team_table *t,unsigned row,const nflstreet_gear_texture_catalog *catalog){
 for(unsigned fill=0;fill<2;++fill){uint8_t native[NFLSTREET_GEAR_RECORD_BYTES];memset(native,fill?0xa5:0,sizeof native);
  memcpy(ptr(gear_ram,SCRATCH_GEAR,sizeof native),native,sizeof native);
  if(!nflstreet_gear_record_from_table(t,row,catalog,native))return 0;
  oracle_run o={0};const uint32_t r[8]={CURSOR,SCRATCH_GEAR};query_table=t;query_row=row;query_ready=1;
  if(!execute(0x8014b918u,r,0,0,&o,NULL)||o.queries!=1||memcmp(native,ptr(gear_ram,SCRATCH_GEAR,sizeof native),sizeof native)){
   fprintf(stderr,"gear record mismatch row%u fill%u\n",row,fill);return 0;}
  ++gear_record_cases;
 }
 return 1;
}
static uint16_t field16(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static int original_player_fields(const nflstreet_team_table *play,unsigned row,const nflstreet_team_table *gear,const nflstreet_gear_records *g,const uint8_t native[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]){
 static const struct {uint32_t pc;unsigned offset,width,add;} getters[6]={
  {0x8014fc3cu,0x40,2,1},{0x8014ec90u,0x42,2,0},{0x8014ecb8u,0x44,2,0},{0x8014f050u,0x4a,1,0},{0x8014ec40u,0x4b,1,0},{0x8014ec68u,0x49,1,0}};
 uint8_t *scratch=ptr(gear_ram,SCRATCH_PLAYER,NFLSTREET_GEAR_PLAYER_RECORD_BYTES);memset(scratch,0xa5,NFLSTREET_GEAR_PLAYER_RECORD_BYTES);
 put(ptr(gear_ram,CURSOR+8,4),0x59414c50u);query_table=play;query_row=row;query_ready=1;
 oracle_run o={0};const uint32_t r[8]={CURSOR,SCRATCH_PLAYER};
 if(!execute(0x8014f60cu,r,0,0,&o,NULL)||o.queries!=12||memcmp(scratch,native,11)||scratch[11]!=0xa5||memcmp(scratch+0x0c,native+0x0c,48)){
  fprintf(stderr,"8014F60C mismatch row%u\n",row);return 0;}
 for(unsigned i=0x3c;i<NFLSTREET_GEAR_PLAYER_RECORD_BYTES;++i)if(scratch[i]!=0xa5)return 0;
 for(unsigned i=0;i<6;++i){oracle_run q={0};const uint32_t a[8]={CURSOR};uint32_t value;
  if(!execute(getters[i].pc,a,0,0,&q,&value)||q.queries!=1)return 0;value+=getters[i].add;
  uint32_t expected=getters[i].width==2?field16(native+getters[i].offset):native[getters[i].offset];
  if((value&(getters[i].width==2?0xffffu:0xffu))!=expected){fprintf(stderr,"player getter %08x mismatch row%u\n",getters[i].pc,row);return 0;}
 }
 unsigned slot=row%14u;memcpy(ptr(gear_ram,PLAYER_BASE+slot*0x68u,NFLSTREET_GEAR_PLAYER_RECORD_BYTES),native,NFLSTREET_GEAR_PLAYER_RECORD_BYTES);
 put(ptr(gear_ram,CURSOR+8,4),0x52414547u);
 oracle_run h={0};h.records=g;const uint32_t s[8]={slot};uint32_t state,gear1;query_table=gear;query_ready=0;
 if(!execute(0x800ddebcu,s,0,0,&h,&state)||(state&0xffffu)!=field16(native+0x46))return 0;
 oracle_run k={0};k.records=g;query_ready=0;
 if(!execute(0x800ddf44u,s,0,0,&k,&gear1)||(gear1&0xffu)!=native[0x48]){fprintf(stderr,"800DDF44 mismatch row%u\n",row);return 0;}
 player_fields+=11+12+6+2;++player_rows;return 1;
}
static int original_plan(const nflstreet_team_table *gear,const nflstreet_gear_records *g,const uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],
 unsigned slot,const nflstreet_gear_texture_plan *plan){
 uint32_t base=PLAYER_BASE+slot*0x68u;uint8_t *record=ptr(gear_ram,base,NFLSTREET_GEAR_PLAYER_RECORD_BYTES);
 memcpy(record,player,NFLSTREET_GEAR_PLAYER_RECORD_BYTES);record[0x65]=plan->record_65^1u;
 ptr(gear_ram,OBJECT,0x1400)[0x1363]=(uint8_t)slot;put(ptr(gear_ram,CURSOR+8,4),0x52414547u);query_table=gear;query_ready=0;
 oracle_run o={0};o.records=g;o.plan=plan;o.slot=slot;const uint32_t r[8]={0};
 if(!execute(0x800de3b8u,r,0,0,&o,NULL)||o.selections!=12||o.queries!=12||o.colors!=plan->color_count||o.states!=1||
    o.slots!=NFLSTREET_GEAR_TEXTURE_SLOTS+plan->fallback||record[0x65]!=plan->record_65||
    memcmp(record,player,0x65)||memcmp(record+0x66,player+0x66,2)){fprintf(stderr,"800DE3B8 mismatch slot%u\n",slot);return 0;}
 ++plan_cases;slot_writes+=o.slots;color_writes+=o.colors;gear_queries+=o.selections;return 1;
}
#define NFLSTREET_GEAR_ORACLE 1
#include "gear_texture_check.c"
