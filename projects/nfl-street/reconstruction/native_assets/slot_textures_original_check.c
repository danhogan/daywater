/* Original slot texture pipeline for one player: 800E2958 layout, 800E2AF0
   templates, 800E2E48 requests and the 800E2CAC/800E27F8 loader including
   800D9C0C recoloring, plus 800D9948 operation sweeps. Archive acquisition
   (801A824C size, 801A75B0 decode) is supplied from fresh native decoding,
   allocation returns the compared tokens, the native-built color table and
   op lists are installed at 8030A898 and r13-5B48, and the 8020380C GX FIFO
   flush is counted only. Relocation, sorting, 80164CE4 copies, recoloring and
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
#define OPS_BASE 0x81130000u
#define SWEEP 0x81140000u
static const ModernGekkoModuleDesc *slot_module;static uint8_t *slot_ram;static const nflstreet_archive *slot_archive;
typedef struct {
 const nflstreet_slot_textures *owned;const host_log *host;uint32_t scratch;
 unsigned flush_seen[NFLSTREET_GEAR_TEXTURE_SLOTS],loads,gx_flushes;int allocated;
} slot_run;
static void put16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static int slot_call(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,slot_run *);
static uint32_t sweep_seed=0x1234567u;
static uint32_t sweep_random(void){sweep_seed=sweep_seed*1103515245u+12345u;return sweep_seed>>8;}
/* 800D9948 for every kind and palette format: random palettes, colors and ranges. */
static int original_sweeps(void){
 const uint32_t pal=SWEEP,record=SWEEP+0x200,op=SWEEP+0x280,table=SWEEP+0x300,ids=SWEEP+0x310,entries=SWEEP+0x400;
 for(unsigned i=0;i<24;++i)put(ptr(slot_ram,ids+4*i,4),i);
 put(ptr(slot_ram,table,4),24);put(ptr(slot_ram,table+4,4),ids);put(ptr(slot_ram,table+8,4),entries);
 for(unsigned kind=0;kind<8;++kind)for(unsigned format=1;format<=3;++format)for(unsigned n=0;n<128;++n){
  uint8_t native[512];const uint8_t *colors[NFLSTREET_COLOR_STATE_BYTES];
  for(unsigned i=0;i<512;++i)native[i]=(uint8_t)sweep_random();
  for(unsigned i=0;i<24*16;++i)ptr(slot_ram,entries,24*16)[i]=(uint8_t)sweep_random();
  for(unsigned i=0;i<24;++i)colors[i]=ptr(slot_ram,entries+16*i,16);
  nflstreet_dynclut_op o={kind,(uint8_t)(sweep_random()%256),(uint8_t)(sweep_random()%256),(uint8_t)(sweep_random()%24),(uint8_t)(sweep_random()%4)};
  /* Kinds 1/5/6 index the palette; kind 7 uses +6 as both color id and palette index. */
  if(kind==1||kind==5||kind==6)o.color=(uint8_t)sweep_random();
  /* Every fourth case keeps start>end (no-op); blends need start<end (no divide by zero). */
  if(o.start>o.end&&n%4){uint8_t x=o.start;o.start=o.end;o.end=x;}
  if(o.start==o.end&&kind>=2&&kind<=6){if(o.end<255)++o.end;else --o.start;}
  memcpy(ptr(slot_ram,pal,512),native,512);
  put16(ptr(slot_ram,record,2),0);put16(ptr(slot_ram,record+2,2),format);put(ptr(slot_ram,record+4,4),512);put(ptr(slot_ram,record+8,4),pal);
  put(ptr(slot_ram,op,4),o.kind);uint8_t *b=ptr(slot_ram,op+4,4);b[0]=o.start;b[1]=o.end;b[2]=o.color;b[3]=o.level;
  slot_run r={0};
  if(!slot_call(0x800d9948u,op,record,table,0,0,&r)||!nflstreet_dynclut_apply((uint16_t)format,native,512,&o,1,colors)||
     memcmp(native,ptr(slot_ram,pal,512),512)){fprintf(stderr,"800D9948 sweep mismatch kind%u format%u case%u\n",kind,format,n);return 0;}
  ++slot_sweeps;
 }
 return 1;
}
static int initialize_slot_oracle(const char *dll,const char *snapshot,const nflstreet_gear_texture_catalog *catalog,const nflstreet_archive *textures,
 const nflstreet_dynclut_table *ops,const nflstreet_color_table *colors){
 (void)catalog;HMODULE h=LoadLibraryA(dll);if(!h)return 0;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 0;
 slot_module=get();slot_ram=load(snapshot);slot_archive=textures;if(!slot_module||!slot_ram)return 0;
 memcpy(ptr(slot_ram,COLOR_TABLE,COLOR_ENTRIES*16u),colors->bytes,COLOR_ENTRIES*16u);
 uint32_t list=OPS_BASE+8u*ops->entries;if(!ptr(slot_ram,OPS_BASE,8u*ops->entries+8u*ops->total))return 0;
 for(unsigned e=0;e<ops->entries;++e){put(ptr(slot_ram,OPS_BASE+8u*e,4),ops->count[e]);put(ptr(slot_ram,OPS_BASE+8u*e+4,4),ops->count[e]?list+8u*ops->first[e]:0);}
 for(unsigned k=0;k<ops->total;++k){const nflstreet_dynclut_op *o=ops->ops+k;uint8_t *d=ptr(slot_ram,list+8u*k,8);put(d,o->kind);d[4]=o->start;d[5]=o->end;d[6]=o->color;d[7]=o->level;}
 put(ptr(slot_ram,DYNCLUT_TABLE,4),OPS_BASE);
 return original_sweeps();
}
static int entry_size(uint32_t index,uint32_t *size){nflstreet_archive_entry e;
 if(index>=slot_archive->count||!nflstreet_archive_get(slot_archive,index,&e))return 0;*size=e.decoded_size;return 1;}
static int supply_entry(uint32_t index,uint32_t to,uint32_t capacity){nflstreet_archive_entry e;uint8_t *b=NULL;size_t n=0;
 if(index>=slot_archive->count||!nflstreet_archive_get(slot_archive,index,&e)||!nflstreet_archive_decode(&e,&b,&n))return 0;
 uint8_t *d=ptr(slot_ram,to,n);int ok=d&&n<=capacity;if(ok)memcpy(d,b,n);free(b);return ok;}
static int slot_hook(CPUState *c,slot_run *o){
 uint32_t pc=c->pc;const nflstreet_slot_textures *t=o->owned;
 if(!t&&pc!=0x8020380cu&&pc!=0x801f0bd0u)return 0;
 if(pc==0x801a824cu){uint32_t size;if(c->gpr[3]!=be(ptr(slot_ram,TEXTURE_DESCRIPTOR,4))||!entry_size(c->gpr[4],&size))return -1;c->gpr[3]=size;}
 else if(pc==0x801a75b0u){
  uint32_t capacity=c->gpr[5]==o->scratch?t->scratch_capacity:0;
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(c->gpr[5]==t->slots[s].token)capacity=t->slots[s].capacity;
  if(c->gpr[3]!=be(ptr(slot_ram,TEXTURE_DESCRIPTOR,4))||!capacity||!supply_entry(c->gpr[4],c->gpr[5],capacity))return -1;
  ++o->loads;c->gpr[3]=0;}
 else if(pc==0x8018b7e4u){if(o->allocated++||c->gpr[3]!=1||c->gpr[4]!=t->block_size||c->gpr[5]!=4)return -1;c->gpr[3]=t->token;}
 else if(pc==0x8018b7b8u){if(o->allocated++||c->gpr[3]!=t->scratch_capacity)return -1;c->gpr[3]=o->scratch;}
 else if(pc==0x801f0bd0u){int s=slot_of(t,c->gpr[3]);
  if(!o->host||!t||s<0||o->flush_seen[s]>=o->host->flush_count[s])return -1;
  const flush_request *r=o->host->flushes[s]+o->flush_seen[s]++;if(r->token!=c->gpr[3]||r->size!=c->gpr[4])return -1;}
 else if(pc==0x8020380cu)++o->gx_flushes;
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
 const uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS],const uint8_t *state,const host_log *h){
 memcpy(ptr(slot_ram,COLOR_STATE,NFLSTREET_COLOR_STATE_BYTES),state,NFLSTREET_COLOR_STATE_BYTES);
 slot_run r={0};r.owned=t;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(!slot_call(0x800e2e48u,0,s,(uint32_t)indices[s],palettes[s],0,&r))return 0;
 slot_run o={0};o.owned=t;o.host=h;o.scratch=SCRATCH;
 if(!slot_call(0x800e2cacu,0,0,0,0,0,&o)||o.allocated!=1){fprintf(stderr,"800E2CAC run failed: allocations %d\n",o.allocated);return 0;}
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const uint8_t *rec=ptr(slot_ram,RECORDS+24u*s,24);
  if(o.flush_seen[s]!=h->flush_count[s]||rec[12]||rec[13]||rec[14]||
     be(rec)!=(uint32_t)t->slots[s].applied_index||be(rec+16)!=t->slots[s].applied_palette||
     memcmp(ptr(slot_ram,t->slots[s].token,t->slots[s].template_size),nflstreet_slot_texture_bytes(t,s),t->slots[s].template_size)){
   fprintf(stderr,"800E2CAC mismatch slot%u\n",s);return 0;}
  slot_flushes+=o.flush_seen[s];}
 if(o.gx_flushes!=h->palettes){fprintf(stderr,"800D9C0C count mismatch %u/%u\n",o.gx_flushes,h->palettes);return 0;}
 slot_compositions+=o.gx_flushes;slot_loads+=o.loads;++slot_players;return 1;
}
/* Original 801EBD38 on each owned destination (already in slot_ram from the
   original pipeline): 8018B7B8 returns the native tokens in order with the same
   allocator fill, 801F0BD0 must match the native flush order, and every block
   byte plus the binding entry words must equal the native ones. */
#define BIND_ENTRIES 0x81150000u
typedef struct {const material_heap *heap;unsigned block,flush;} bind_run;
static int bind_call(uint32_t destination,uint32_t entry,bind_run *r){
 CPUState c={0};c.ram=slot_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=ORACLE_STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 c.gpr[3]=destination;c.gpr[4]=entry;c.gpr[5]=6;c.gpr[6]=0;c.pc=0x801ebd38u;
 for(unsigned step=0;c.pc&&step<2000000u;++step){uint32_t pc=c.pc;
  if(pc==0x8018b7b8u){const material_heap *h=r->heap;if(r->block>=h->count||c.gpr[3]!=h->sizes[r->block]||c.gpr[4])return 0;
   uint32_t token=h->tokens[r->block++],rounded=(c.gpr[3]+31u)&~31u;uint8_t *d=ptr(slot_ram,token,rounded);if(!d)return 0;
   memset(d,h->fill,rounded);c.gpr[3]=token;c.pc=c.lr;continue;}
  if(pc==0x801f0bd0u){const material_heap *h=r->heap;if(r->flush>=h->flush_count)return 0;const flush_request *f=h->flushes+r->flush++;
   if(f->token!=c.gpr[3]||f->size!=c.gpr[4])return 0;c.pc=c.lr;continue;}
  if(pc==0x801af944u||pc==0x801af9ccu){c.gpr[3]=0;c.pc=c.lr;continue;}
  if(pc==0x801af57cu){c.pc=c.lr;continue;}
  c.downcount=-1000000000;slot_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 return !c.pc;
}
static int original_slot_bind(const nflstreet_slot_textures *t,const nflstreet_slot_material *a,const material_heap *ha,
 const nflstreet_slot_material *b,const material_heap *hb){
 for(unsigned pass=0;pass<2;++pass){const nflstreet_slot_material *m=pass?b:a;bind_run r={pass?hb:ha,0,0};
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){uint32_t entry=BIND_ENTRIES+0x20u*s;memset(ptr(slot_ram,entry,0x20),0,0x20);
   if(!bind_call(t->slots[s].token,entry,&r)){fprintf(stderr,"801EBD38 run failed slot%u pass%u\n",s,pass);return 0;}
   const uint8_t *e=ptr(slot_ram,entry,0x20);
   if(be(e+0x14)!=m[s].entries||be(e+0x18)!=m[s].set||be(e+0x1c)!=m[s].destination){fprintf(stderr,"801EBD38 entry mismatch slot%u\n",s);return 0;}
   for(unsigned k=0;k<5;++k){const nflstreet_material_block *x=set_block(&m[s].texture_set,k);
    if(x->size&&memcmp(ptr(slot_ram,x->token,x->size),x->bytes,x->size)){fprintf(stderr,"801EBD38 block%u mismatch slot%u pass%u\n",k,s,pass);return 0;}}
   if(!pass)++bind_sets;}
  if(r.block!=r.heap->count||r.flush!=r.heap->flush_count){fprintf(stderr,"801EBD38 allocation/flush count mismatch\n");return 0;}}
 return 1;
}
/* Original 80164DD8(table, 0) with the default 8028DD90 list on player 0's
   binding rows (DOL names kept, +14/+18/+1C from native). Each pass first
   restores that pass's unconfigured native texture objects at their tokens. */
static int original_slot_configure(const nflstreet_slot_material *a,const material_heap *ha,const uint8_t *ua,
 const nflstreet_slot_material *b,const material_heap *hb,const uint8_t *ub){
 for(unsigned pass=0;pass<2;++pass){const nflstreet_slot_material *m=pass?b:a;const material_heap *h=pass?hb:ha;const uint8_t *u=pass?ub:ua;
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){uint8_t *e=ptr(slot_ram,BINDING_TABLE+0x20u*s,0x20);
   put(e+0x14,m[s].entries);put(e+0x18,m[s].set);put(e+0x1c,m[s].destination);
   const nflstreet_material_block *x=&m[s].texture_set.textures;
   if(x->size)memcpy(ptr(slot_ram,x->token,x->size),u+(x->bytes-h->arena),x->size);}
  CPUState c={0};c.ram=slot_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=ORACLE_STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
  c.gpr[3]=BINDING_TABLE;c.gpr[4]=0;c.pc=0x80164dd8u;
  for(unsigned step=0;c.pc&&step<2000000u;++step){c.downcount=-1000000000;slot_module->dispatch(&c,c.pc);if(c.exception)return 0;}
  if(c.pc)return 0;
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const nflstreet_material_block *x=&m[s].texture_set.textures;
   if(x->size&&memcmp(ptr(slot_ram,x->token,x->size),x->bytes,x->size)){fprintf(stderr,"80164DD8 mismatch slot%u pass%u\n",s,pass);return 0;}}
  if(!pass)++configure_sets;}
 return 1;
}
/* Original 801EBD38 on one synthetic unrelocated MMAP at token (801C5610
   relocates it inside 801C8E80): relocated bytes, all blocks, entry words,
   allocations and flushes must equal the native ones for both fills. */
static int original_texture_set(const uint8_t *unrelocated,size_t size,uint32_t token,const uint8_t *relocated,
 const nflstreet_texture_set *a,const material_heap *ha,const nflstreet_texture_set *b,const material_heap *hb){
 for(unsigned pass=0;pass<2;++pass){const nflstreet_texture_set *t=pass?b:a;bind_run r={pass?hb:ha,0,0};
  if(token)memcpy(ptr(slot_ram,token,size),unrelocated,size);
  uint32_t entry=BIND_ENTRIES;memset(ptr(slot_ram,entry,0x20),0,0x20);
  if(!bind_call(token,entry,&r)||r.block!=r.heap->count||r.flush!=r.heap->flush_count){fprintf(stderr,"801EBD38 sweep run failed (token %08x)\n",token);return 0;}
  const uint8_t *e=ptr(slot_ram,entry,0x20);
  if(be(e+0x18)!=t->object.token||be(e+0x1c)!=token||be(e+0x14)!=be(t->object.bytes+0x24)||(token&&memcmp(ptr(slot_ram,token,size),relocated,size))){
   fprintf(stderr,"801EBD38 sweep entry/relocation mismatch\n");return 0;}
  for(unsigned k=0;k<5;++k){const nflstreet_material_block *x=set_block(t,k);
   if(x->size&&memcmp(ptr(slot_ram,x->token,x->size),x->bytes,x->size)){fprintf(stderr,"801EBD38 sweep block%u mismatch pass%u\n",k,pass);return 0;}}
  if(!pass)++sweep_sets;}
 return 1;
}
/* Original 80164DD8 with a controlled list: names, binding rows and list are
   written to RAM, the unconfigured native texture sets at their tokens. */
#define SWEEP_TABLE 0x81158000u
#define SWEEP_NAMES 0x81159000u
#define SWEEP_LIST 0x8115a000u
static int original_configure_sweep(const nflstreet_slot_material *m,const material_heap *h,const uint8_t *unconfigured,
 const uint8_t *list,unsigned count){
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){uint8_t *e=ptr(slot_ram,SWEEP_TABLE+0x20u*s,0x20),*n=ptr(slot_ram,SWEEP_NAMES+16u*s,16);
  memcpy(n,m[s].material,16);memset(e,0,0x20);put(e,SWEEP_NAMES+16u*s);put(e+0x14,m[s].entries);put(e+0x18,m[s].set);put(e+0x1c,m[s].destination);
  for(unsigned k=0;k<5;++k){const nflstreet_material_block *x=set_block(&m[s].texture_set,k);
   if(x->size)memcpy(ptr(slot_ram,x->token,x->size),unconfigured+(x->bytes-h->arena),x->size);}}
 memset(ptr(slot_ram,SWEEP_TABLE+0x20u*NFLSTREET_GEAR_TEXTURE_SLOTS,0x20),0,0x20);
 for(unsigned i=0;i<=count;++i){uint8_t *d=ptr(slot_ram,SWEEP_LIST+28u*i,28);memset(d,0,28);if(i==count)break;
  const uint8_t *e=list+i*(NFLSTREET_SLOT_MATERIAL_NAME_BYTES+24u);unsigned s=0;
  while(strcmp(m[s].material,(const char*)e))++s;put(d,SWEEP_NAMES+16u*s);memcpy(d+4,e+NFLSTREET_SLOT_MATERIAL_NAME_BYTES,24);}
 CPUState c={0};c.ram=slot_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=ORACLE_STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 c.gpr[3]=SWEEP_TABLE;c.gpr[4]=SWEEP_LIST;c.pc=0x80164dd8u;
 for(unsigned step=0;c.pc&&step<2000000u;++step){c.downcount=-1000000000;slot_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 if(c.pc)return 0;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const nflstreet_material_block *x=&m[s].texture_set.textures;
  if(x->size&&memcmp(ptr(slot_ram,x->token,x->size),x->bytes,x->size)){fprintf(stderr,"80164DD8 sweep mismatch slot%u\n",s);return 0;}}
 ++configure_sweeps;return 1;
}
/* Original 801639C8 material hookup on a fresh 45-entry array: 801EBDD4,
   801C7528(8028DD10), 801EBBA8 with a model whose +18 is 8028DD10 and a scratch
   binding table (DOL name pointers, native words), then the eight tattoo
   801EBC78 calls. Entries blocks carry the variant edits. Allocation 8018B7B8
   bumps in scratch; the whole RAM image is restored afterwards so instance
   pools do not drain across players. */
#define HOOK_ARRAY 0x8115b000u
#define HOOK_MODEL 0x8115b100u
#define HOOK_TABLE 0x8115b200u
#define HOOK_HEAP 0x8115c000u
static uint8_t *hook_backup;
static int hook_call(uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t e,uint32_t f,uint32_t g,uint32_t *heap){
 CPUState c={0};c.ram=slot_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=ORACLE_STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 c.gpr[3]=a;c.gpr[4]=b;c.gpr[5]=d;c.gpr[6]=e;c.gpr[7]=f;c.gpr[8]=g;c.pc=pc;
 for(unsigned step=0;c.pc&&step<5000000u;++step){uint32_t p=c.pc;
  if(p==0x8018b7b8u){uint32_t n=(c.gpr[3]+31u)&~31u;if(*heap+n>HOOK_HEAP+0x1000u)return 0;memset(ptr(slot_ram,*heap,n),0,n);c.gpr[3]=*heap;*heap+=n;c.pc=c.lr;continue;}
  if(p==0x801af944u||p==0x801af9ccu){c.gpr[3]=0;c.pc=c.lr;continue;}
  if(p==0x801af57cu){c.pc=c.lr;continue;}
  c.downcount=-1000000000;slot_module->dispatch(&c,c.pc);if(c.exception)return 0;}
 return !c.pc;
}
static int original_actor_hook(const nflstreet_slot_material *v,const nflstreet_actor_material *expected,unsigned count){
 static const uint8_t materials[8]={10,11,0x13,0x14,0xe,0xf,0x17,0x18},slots[8]={19,19,20,20,21,21,22,22};
 if(!hook_backup&&!(hook_backup=malloc(RAM_SIZE)))return 0;memcpy(hook_backup,slot_ram,RAM_SIZE);
 int ok=0;uint32_t heap=HOOK_HEAP;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){uint8_t *e=ptr(slot_ram,HOOK_TABLE+0x20u*s,0x20);memset(e,0,0x20);
  uint32_t name=0;for(unsigned k=0;k<NFLSTREET_GEAR_TEXTURE_SLOTS;++k){uint32_t q=be(ptr(slot_ram,BINDING_TABLE+0x20u*k,4));
   if(!strcmp((const char*)ptr(slot_ram,q,1),v[s].material))name=q;}
  if(!name)goto done;put(e,name);put(e+0x14,v[s].entries);put(e+0x18,v[s].set);put(e+0x1c,v[s].destination);
  const nflstreet_material_block *x=&v[s].texture_set.entries;if(x->size)memcpy(ptr(slot_ram,x->token,x->size),x->bytes,x->size);}
 memset(ptr(slot_ram,HOOK_TABLE+0x20u*NFLSTREET_GEAR_TEXTURE_SLOTS,0x20),0,0x20);
 memset(ptr(slot_ram,HOOK_MODEL,0x40),0,0x40);put(ptr(slot_ram,HOOK_MODEL+0x18,4),0x8028dd10u);memset(ptr(slot_ram,HOOK_ARRAY,8),0,8);
 if(!hook_call(0x801ebdd4u,HOOK_ARRAY,45,0,0,0,0,&heap)||!hook_call(0x801c7528u,HOOK_ARRAY,0x8028dd10u,0,0,0,0,&heap)||
    !hook_call(0x801ebba8u,HOOK_ARRAY,HOOK_MODEL,HOOK_TABLE,0,0,0,&heap))goto done;
 for(unsigned k=0;k<8;++k){uint32_t e=HOOK_TABLE+0x20u*slots[k];if(!hook_call(0x801ebc78u,HOOK_ARRAY,materials[k],1,e,e,0,&heap))goto done;}
 {uint32_t shaders[2]={be(ptr(slot_ram,R13-0x5638u,4)),be(ptr(slot_ram,R13-0x5634u,4))};
  if(be(ptr(slot_ram,HOOK_ARRAY,4))!=count)goto done;uint32_t list=be(ptr(slot_ram,HOOK_ARRAY+4,4));
  for(unsigned i=0;i<count;++i){uint32_t inst=be(ptr(slot_ram,list+4*i,4));const nflstreet_actor_material *m=expected+i;
   if(!inst||be(ptr(slot_ram,inst+8,4))!=shaders[m->shader]||be(ptr(slot_ram,inst+12,4))!=m->stage_count){fprintf(stderr,"801639C8 hookup shader mismatch material%u\n",i);goto done;}
   uint32_t st=be(ptr(slot_ram,inst+16,4));
   for(unsigned k=0;k<m->stage_count;++k){const uint8_t *q=ptr(slot_ram,st+12*k,12);
    if(be(q)!=m->stages[k].texture||be(q+4)!=m->stages[k].palette||be(q+8)!=m->stages[k].index){fprintf(stderr,"801639C8 hookup stage mismatch material%u stage%u\n",i,k);goto done;}}}}
 ok=1;++hook_runs;
done:
 memcpy(slot_ram,hook_backup,RAM_SIZE);return ok;
}
