/* Original slot texture pipeline for one player: 800E2958 layout, 800E2AF0
   templates, 800E2E48 requests and the 800E2CAC/800E27F8 loader. Archive
   acquisition (801A824C size, 801A75B0 decode) is supplied from fresh native
   decoding, allocation returns the compared tokens, and 800D9C0C palette
   composition is recorded only. Relocation, sorting, 80164CE4 copies and
   801F0BD0 flush requests execute in the original module. */
#define main model_check_main
#include "actor_model_init_original_check.c"
#undef main
#define NFLSTREET_SLOT_ORACLE 1
#include "slot_textures_check.c"
#define R13 0x803a9a40u
#define TEXTURE_DESCRIPTOR (R13-0x5accu)
#define PLAYER_COUNT (R13-0x71f0u)
#define SCRATCH_SIZE (R13-0x71ecu)
#define LAYOUT_DONE (R13-0x71f8u)
#define RECORDS 0x802799ecu
#define DESTINATION_TABLE 0x802ab8f4u
#define SIZE_TABLE 0x802abfbcu
#define COLOR_STATE 0x802abe6cu
/* Below SCRATCH; live player destination blocks extend past 0x81700000. */
#define ORACLE_STACK 0x810f0000u
static const ModernGekkoModuleDesc *slot_module;static uint8_t *slot_ram;static const nflstreet_archive *slot_archive;
typedef struct {
 const nflstreet_slot_textures *owned;const host_log *host;const uint32_t *palettes;uint32_t scratch;
 unsigned flush_seen[NFLSTREET_GEAR_TEXTURE_SLOTS],composed[NFLSTREET_GEAR_TEXTURE_SLOTS],loads;int allocated;
} slot_run;
static int initialize_slot_oracle(const char *dll,const char *snapshot,const nflstreet_gear_texture_catalog *catalog,const nflstreet_archive *textures){
 (void)catalog;HMODULE h=LoadLibraryA(dll);if(!h)return 0;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 0;
 slot_module=get();slot_ram=load(snapshot);slot_archive=textures;return slot_module&&slot_ram;
}
static int entry_size(uint32_t index,uint32_t *size){nflstreet_archive_entry e;
 if(index>=slot_archive->count||!nflstreet_archive_get(slot_archive,index,&e))return 0;*size=e.decoded_size;return 1;}
static int supply_entry(uint32_t index,uint32_t to,uint32_t capacity){nflstreet_archive_entry e;uint8_t *b=NULL;size_t n=0;
 if(index>=slot_archive->count||!nflstreet_archive_get(slot_archive,index,&e)||!nflstreet_archive_decode(&e,&b,&n))return 0;
 uint8_t *d=ptr(slot_ram,to,n);int ok=d&&n<=capacity;if(ok)memcpy(d,b,n);free(b);return ok;}
static int slot_hook(CPUState *c,slot_run *o){
 uint32_t pc=c->pc;const nflstreet_slot_textures *t=o->owned;
 if(pc==0x801a824cu){uint32_t size;if(c->gpr[3]!=be(ptr(slot_ram,TEXTURE_DESCRIPTOR,4))||!entry_size(c->gpr[4],&size))return -1;c->gpr[3]=size;}
 else if(pc==0x801a75b0u){
  uint32_t capacity=c->gpr[5]==o->scratch?t->scratch_capacity:0;
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(c->gpr[5]==t->slots[s].token)capacity=t->slots[s].capacity;
  if(c->gpr[3]!=be(ptr(slot_ram,TEXTURE_DESCRIPTOR,4))||!capacity||!supply_entry(c->gpr[4],c->gpr[5],capacity))return -1;
  ++o->loads;c->gpr[3]=0;}
 else if(pc==0x8018b7e4u){if(o->allocated++||c->gpr[3]!=1||c->gpr[4]!=t->block_size||c->gpr[5]!=4)return -1;c->gpr[3]=t->token;}
 else if(pc==0x8018b7b8u){if(o->allocated++||c->gpr[3]!=t->scratch_capacity)return -1;c->gpr[3]=o->scratch;}
 else if(pc==0x801f0bd0u){int s=slot_of(t,c->gpr[3]);
  if(!o->host||s<0||o->flush_seen[s]>=o->host->flush_count[s])return -1;
  const flush_request *r=o->host->flushes[s]+o->flush_seen[s]++;if(r->token!=c->gpr[3]||r->size!=c->gpr[4])return -1;}
 else if(pc==0x800d9c0cu){int found=-1;
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(c->gpr[3]==be(nflstreet_slot_texture_bytes(t,s)+28))found=(int)s;
  if(found<0||!o->palettes||o->composed[found]++||c->gpr[4]!=o->palettes[found]||c->gpr[5]!=COLOR_STATE)return -1;}
 else if(pc!=0x8018baa8u&&pc!=0x801a9c54u&&pc!=0x801a9c64u&&pc!=0x801af944u&&pc!=0x801af9ccu&&pc!=0x801af57cu)return 0;
 c->pc=c->lr;return 1;
}
static int slot_call(uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t e,uint32_t f,slot_run *o){
 CPUState c={0};c.ram=slot_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=ORACLE_STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=d;c.gpr[6]=e;c.gpr[7]=f;c.pc=pc;
 for(unsigned step=0;c.pc&&step<200000000u;++step){int h=slot_hook(&c,o);if(h<0){fprintf(stderr,"slot hook mismatch at %08x\n",c.pc);return 0;}
  if(h)continue;c.downcount=-1000000000;slot_module->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"slot exception at %08x from %08x\n",c.pc,pc);return 0;}}
 if(c.pc)fprintf(stderr,"slot call %08x did not return (pc %08x)\n",pc,c.pc);return !c.pc;
}
static int original_slot_open(const nflstreet_slot_textures *t){
 *ptr(slot_ram,LAYOUT_DONE,1)=0;put(ptr(slot_ram,SCRATCH_SIZE,4),0);
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)put(ptr(slot_ram,RECORDS+24u*s,4),t->slots[s].template_index);
 slot_run o={0};o.owned=t;o.scratch=SCRATCH;
 if(!slot_call(0x800e2958u,1,0,0,0,0,&o)||o.allocated!=1||be(ptr(slot_ram,SCRATCH_SIZE,4))!=t->scratch_capacity){
  fprintf(stderr,"800E2958 run failed: allocations %d scratch %08x/%08x\n",o.allocated,be(ptr(slot_ram,SCRATCH_SIZE,4)),t->scratch_capacity);return 0;}
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const uint8_t *r=ptr(slot_ram,RECORDS+24u*s,24);
  if(be(ptr(slot_ram,DESTINATION_TABLE+4u*s,4))!=t->slots[s].token||be(ptr(slot_ram,SIZE_TABLE+4u*s,4))!=t->slots[s].template_size||
     be(r)!=t->slots[s].template_index||be(r+4)!=0||be(r+8)!=s||r[12]!=1||r[13]||r[14]||be(r+16)!=UINT32_MAX){fprintf(stderr,"800E2958 mismatch slot%u\n",s);return 0;}}
 ++slot_layouts;
 slot_run a={0};a.owned=t;a.scratch=SCRATCH;
 if(!slot_call(0x800e2af0u,0,0,0,0,0,&a)||a.loads!=NFLSTREET_GEAR_TEXTURE_SLOTS){fprintf(stderr,"800E2AF0 run failed: loads %u\n",a.loads);return 0;}
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(memcmp(ptr(slot_ram,t->slots[s].token,t->slots[s].template_size),
   nflstreet_slot_texture_bytes(t,s),t->slots[s].template_size)){fprintf(stderr,"800E2AF0 mismatch slot%u\n",s);return 0;}
 ++slot_templates;return 1;
}
static int original_slot_apply(const nflstreet_slot_textures *t,const int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS],
 const uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS],const host_log *h){
 slot_run r={0};r.owned=t;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(!slot_call(0x800e2e48u,0,s,(uint32_t)indices[s],palettes[s],0,&r))return 0;
 uint32_t effective[NFLSTREET_GEAR_TEXTURE_SLOTS];
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)effective[s]=indices[s]==-1?UINT32_MAX:palettes[s];
 slot_run o={0};o.owned=t;o.host=h;o.palettes=effective;o.scratch=SCRATCH;
 if(!slot_call(0x800e2cacu,0,0,0,0,0,&o)||o.allocated!=1){fprintf(stderr,"800E2CAC run failed: allocations %d\n",o.allocated);return 0;}
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const uint8_t *rec=ptr(slot_ram,RECORDS+24u*s,24);
  if(o.flush_seen[s]!=h->flush_count[s]||o.composed[s]!=(effective[s]!=UINT32_MAX)||rec[12]||rec[13]||rec[14]||
     be(rec)!=(uint32_t)t->slots[s].applied_index||be(rec+16)!=t->slots[s].applied_palette||
     memcmp(ptr(slot_ram,t->slots[s].token,t->slots[s].template_size),nflstreet_slot_texture_bytes(t,s),t->slots[s].template_size)){
   fprintf(stderr,"800E2CAC mismatch slot%u\n",s);return 0;}
  slot_flushes+=o.flush_seen[s];slot_compositions+=o.composed[s];}
 slot_loads+=o.loads;++slot_players;return 1;
}
