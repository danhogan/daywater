/* Owned selected slot textures with original palette recoloring. The color
   table, the pladyncl op lists and the 14 captured players' destinations are
   compared with the live snapshot. All roster rows in both captured team
   contexts then load, apply and recolor natively. Slot1 is the separate face
   path: captured players use their live slot1 request, roster rows keep the
   template. Color state is record +4C..+63 (captured team values). */
#include "slot_textures.h"
#include "palette_recolor.h"
#include "slot_materials.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CAPTURED_PLAYERS 14u
#define PLAYER_RECORDS 0x802aaa60u
#define REQUEST_RECORDS 0x802799ecu
#define DESTINATIONS 0x802ab8f4u
#define TEMPLATE_SIZES 0x802abfbcu
#define SCRATCH 0x81100000u
#define MAX_FLUSHES 64u
#define COLOR_TABLE 0x8030a898u
#define COLOR_ENTRIES 140u
#define DYNCLUT_TABLE (0x803a9a40u-0x5b48u)
#define COLOR_STATES 0x802abe6cu
#define BINDING_TABLE 0x80276c6cu
#define MATERIAL_HEAP 0x81160000u
#define MATERIAL_ARENA 0x10000u
#define MATERIAL_BLOCKS (NFLSTREET_GEAR_TEXTURE_SLOTS*5u)
#define MATERIAL_FLUSHES 256u
static uint8_t *load_data(const char *path,size_t *size){FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return b;}
static uint32_t be32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
typedef struct {uint32_t token,size;} flush_request;
typedef struct {
 const nflstreet_slot_textures *owner;flush_request flushes[NFLSTREET_GEAR_TEXTURE_SLOTS][MAX_FLUSHES];
 unsigned flush_count[NFLSTREET_GEAR_TEXTURE_SLOTS],palettes;uint32_t palette_value[NFLSTREET_GEAR_TEXTURE_SLOTS];int failed;
} host_log;
static int slot_of(const nflstreet_slot_textures *t,uint32_t token){
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(token>=t->slots[s].token&&token-t->slots[s].token<t->slots[s].template_size)return(int)s;
 return -1;
}
static int host_flush(void *opaque,uint32_t token,uint32_t size){host_log *h=opaque;int s=slot_of(h->owner,token);
 if(s<0||h->flush_count[s]>=MAX_FLUSHES)return 0;h->flushes[s][h->flush_count[s]++]=(flush_request){token,size};return 1;}
typedef struct {nflstreet_palette_context compose;host_log *host;} palette_host;
static int host_palette(void *opaque,unsigned slot,uint32_t palette,uint8_t *d,size_t n,uint32_t token){palette_host *p=opaque;
 if(!nflstreet_palette_compose_slot(&p->compose,slot,palette,d,n,token))return 0;p->host->palette_value[slot]=palette;++p->host->palettes;return 1;}
/* Byte range of destination palette record0 data (801C5958(dest,0) -> +8 pointer, +4 size). */
static int palette_data(const nflstreet_slot_textures *t,unsigned s,size_t *from,size_t *to){
 const nflstreet_slot_texture *slot=t->slots+s;const uint8_t *d=nflstreet_slot_texture_bytes(t,s);
 uint32_t records=be32(d+28);if(records<slot->token||records-slot->token>slot->template_size-12u)return 0;
 const uint8_t *r=d+(records-slot->token);uint32_t at=be32(r+8),size=be32(r+4);
 if(at<slot->token||at-slot->token>slot->template_size||size>slot->template_size-(at-slot->token))return 0;
 *from=at-slot->token;*to=*from+size;return 1;
}
#ifdef NFLSTREET_SLOT_ORACLE
static unsigned slot_sweeps,slot_layouts,slot_templates,slot_players,slot_loads,slot_flushes,slot_compositions;
static int initialize_slot_oracle(const char *,const char *,const nflstreet_gear_texture_catalog *,const nflstreet_archive *,
 const nflstreet_dynclut_table *,const nflstreet_color_table *);
static int original_slot_open(const nflstreet_slot_textures *);
static int original_slot_apply(const nflstreet_slot_textures *,const int32_t *,const uint32_t *,const uint8_t *,const host_log *);
#endif
static nflstreet_gear_records records;
static host_log host;
typedef struct {unsigned layout,identical,composed,players,flushes,palettes;uint32_t digest;} totals;
static nflstreet_dynclut_table dynclut;static nflstreet_color_table colors;
/* 8018B7B8 for 801EBD38: blocks keep the allocator's fill byte, as original
   allocations keep their prior contents. Live mode hands out the captured
   allocation addresses in order instead of fresh tokens. */
typedef struct {
 uint8_t arena[MATERIAL_ARENA],fill;uint32_t used,next,tokens[MATERIAL_BLOCKS],sizes[MATERIAL_BLOCKS];unsigned count;
 const uint32_t *expected;unsigned expected_count;flush_request flushes[MATERIAL_FLUSHES];unsigned flush_count;
} material_heap;
static int material_allocate(void *opaque,uint32_t size,uint32_t *token,uint8_t **bytes){material_heap *h=opaque;
 uint32_t rounded=(size+31u)&~31u;if(h->count>=MATERIAL_BLOCKS||rounded>MATERIAL_ARENA-h->used)return 0;
 if(h->expected){if(h->count>=h->expected_count)return 0;*token=h->expected[h->count];}else{*token=h->next;h->next+=rounded;}
 *bytes=h->arena+h->used;memset(*bytes,h->fill,rounded);h->used+=rounded;h->tokens[h->count]=*token;h->sizes[h->count++]=size;return 1;}
static int material_flush(void *opaque,uint32_t token,uint32_t size){material_heap *h=opaque;
 if(h->flush_count>=MATERIAL_FLUSHES)return 0;h->flushes[h->flush_count++]=(flush_request){token,size};return 1;}
static void material_reset(material_heap *h,uint8_t fill,const uint32_t *expected,unsigned count){
 h->fill=fill;h->used=0;h->next=MATERIAL_HEAP;h->count=0;h->expected=expected;h->expected_count=count;h->flush_count=0;}
static int bind_materials(const nflstreet_slot_material_catalog *c,nflstreet_slot_textures *owned,material_heap *h,uint8_t fill,
 const uint32_t *expected,unsigned count,nflstreet_slot_material out[NFLSTREET_GEAR_TEXTURE_SLOTS]){
 material_reset(h,fill,expected,count);return nflstreet_slot_materials_bind(c,owned,material_allocate,h,material_flush,h,out);}
static const nflstreet_material_block *set_block(const nflstreet_texture_set *t,unsigned i){
 return i==0?&t->object:i==1?&t->entries:i==2?&t->textures:i==3?&t->palettes:&t->names;}
typedef struct {unsigned sweeps,configure_entries,sets,images,palettes,flushes,unwritten,live_sets,live_ids,live_names,live_tmem,live_instances,live_two_pass,hookups,hooked_two_pass;uint32_t digest;} material_totals;
static material_totals mt;
static nflstreet_slot_material_catalog material_catalog;
static material_heap heaps[2];
#ifdef NFLSTREET_SLOT_ORACLE
static unsigned bind_sets;
static int original_slot_bind(const nflstreet_slot_textures *,const nflstreet_slot_material *,const material_heap *,
 const nflstreet_slot_material *,const material_heap *);
static unsigned configure_sets,sweep_sets;
static int original_slot_configure(const nflstreet_slot_material *,const material_heap *,const uint8_t *,
 const nflstreet_slot_material *,const material_heap *,const uint8_t *);
static int original_texture_set(const uint8_t *,size_t,uint32_t,const uint8_t *,const nflstreet_texture_set *,const material_heap *,
 const nflstreet_texture_set *,const material_heap *);
static unsigned configure_sweeps;
static int original_configure_sweep(const nflstreet_slot_material *,const material_heap *,const uint8_t *,const uint8_t *,unsigned);
static unsigned hook_runs;
static int original_actor_hook(const nflstreet_slot_material *,const nflstreet_actor_material *,unsigned);
#endif
/* Live comparison of one captured player's binding after 80164DD8. Bytes the
   original leaves as allocator contents are skipped. Allowed later writes:
   the register-id byte of texture-object words 0..3 (GXLoadTexObj), the
   texture-object TLUT name +18 (GXInitTexObjTlut) and palette-object word +4
   bits 0..9 (GXLoadTlut TMEM offset), all written by the 801C7AD4 texture
   load when drawing. */
static int live_materials(const uint8_t *live,unsigned p,nflstreet_slot_textures *owned){
 uint32_t expected[MATERIAL_BLOCKS];unsigned count=0;nflstreet_slot_material a[NFLSTREET_GEAR_TEXTURE_SLOTS],b[NFLSTREET_GEAR_TEXTURE_SLOTS];
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const uint8_t *e=live+(BINDING_TABLE-0x80000000u)+p*0x340u+s*0x20u;uint32_t o=be32(e+0x18);
  const uint8_t *obj=live+(o-0x80000000u);expected[count++]=o;
  if(be32(obj+0x20))expected[count++]=be32(obj+0x24);if(be32(obj+0x10))expected[count++]=be32(obj+0x14);
  if(be32(obj+0x18))expected[count++]=be32(obj+0x1c);if(be32(obj))expected[count++]=be32(obj+8);}
 if(!bind_materials(&material_catalog,owned,heaps,0x00,expected,count,a)||!bind_materials(&material_catalog,owned,heaps+1,0xff,expected,count,b)||
    !nflstreet_slot_materials_configure(&material_catalog,a)||!nflstreet_slot_materials_configure(&material_catalog,b))return 0;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const uint8_t *e=live+(BINDING_TABLE-0x80000000u)+p*0x340u+s*0x20u;
  const char *name=(const char*)live+(be32(e)-0x80000000u);
  if(strcmp(name,a[s].material)||be32(e+0x14)!=a[s].entries||be32(e+0x18)!=a[s].set||be32(e+0x1c)!=a[s].destination){
   fprintf(stderr,"live binding entry mismatch player%u slot%u\n",p,s);return 0;}
  for(unsigned k=0;k<5;++k){const nflstreet_material_block *x=set_block(&a[s].texture_set,k),*y=set_block(&b[s].texture_set,k);
   const uint8_t *l=live+(x->token-0x80000000u);
   for(uint32_t i=0;i<x->size;++i){if(x->bytes[i]!=y->bytes[i]||x->bytes[i]==l[i])continue;
    uint32_t in=i%(k==2?0x2cu:0x28u);int allowed=0;
    if(k==2&&in<16&&!(in&3)){allowed=1;++mt.live_ids;}
    else if(k==2&&in>=0x18&&in<0x1c){allowed=1;++mt.live_names;}
    else if(k==3&&(in==6||in==7)){uint32_t d=be32(x->bytes+i-in+4)^be32(l+i-in+4);allowed=!(d&~0x3ffu);if(allowed)++mt.live_tmem;}
    if(!allowed){fprintf(stderr,"live material mismatch player%u slot%u block%u byte%u: %02x vs %02x (words %08x %08x)\n",p,s,k,i,x->bytes[i],l[i],be32(x->bytes+i-(i&3u)),be32(l+i-(i&3u)));return 0;}}}
  ++mt.live_sets;}
 /* The actor of this record slot (801639C8 material instances at actor+FEC). */
 nflstreet_actor_material hooked[NFLSTREET_ACTOR_MATERIALS_MAX];unsigned instances=0;
 if(!nflstreet_actor_materials_hook(&material_catalog,a,hooked,&instances))return 0;
 const uint8_t *actors=live+(be32(live+(0x803a9a40u-0x7bbcu-0x80000000u))-0x80000000u);uint32_t actor=0;
 for(unsigned i=0;i<CAPTURED_PLAYERS;++i){uint32_t x=be32(actors+4*i);if(live[x-0x80000000u+0x1363]==p)actor=x;}
 const uint32_t shaders[2]={be32(live+(0x803a9a40u-0x5638u-0x80000000u)),be32(live+(0x803a9a40u-0x5634u-0x80000000u))};
 if(!actor||be32(live+(actor-0x80000000u)+0xfec)!=instances)return 0;
 const uint8_t *list=live+(be32(live+(actor-0x80000000u)+0xff0)-0x80000000u);
 for(unsigned i=0;i<instances;++i){const uint8_t *inst=live+(be32(list+4*i)-0x80000000u);const nflstreet_actor_material *m=hooked+i;
  int ok=be32(inst+8)==shaders[m->shader]&&be32(inst+12)==m->stage_count&&!strcmp((const char*)live+(be32(live+(be32(inst+4)-0x80000000u))-0x80000000u),m->material);
  const uint8_t *st=live+(be32(inst+16)-0x80000000u);
  for(unsigned k=0;ok&&k<m->stage_count;++k)ok=be32(st+12*k)==m->stages[k].texture&&be32(st+12*k+4)==m->stages[k].palette&&be32(st+12*k+8)==m->stages[k].index;
  if(!ok){fprintf(stderr,"live actor material mismatch player%u material%u\n",p,i);return 0;}
  ++mt.live_instances;mt.live_two_pass+=m->shader;}
 return 1;
}
static int check_player(const nflstreet_gear_texture_catalog *catalog,const nflstreet_archive *textures,const nflstreet_gear_texture_plan *plan,
 const uint8_t *live,int captured,unsigned p,const uint8_t state[NFLSTREET_COLOR_STATE_BYTES],totals *t){
 int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS];uint32_t pal[NFLSTREET_GEAR_TEXTURE_SLOTS];
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){indices[s]=plan->indices[s];pal[s]=plan->palettes[s];}
 if(plan->fallback){indices[0]=plan->fallback_index;pal[0]=plan->fallback_palette;}
 if(captured){const uint8_t *r=live+(REQUEST_RECORDS-0x80000000u)+p*600u+24u;indices[1]=(int32_t)be32(r);pal[1]=be32(r+16);}
 else{indices[1]=-1;pal[1]=UINT32_MAX;}
 uint32_t base=be32(live+(DESTINATIONS-0x80000000u)+(captured?p:0u)*100u);
 nflstreet_slot_textures owned;if(!nflstreet_slot_textures_open(catalog,textures,base,&owned)){fprintf(stderr,"open failed %u\n",p);return 0;}
 int ok=0;
#ifdef NFLSTREET_SLOT_ORACLE
 if(!original_slot_open(&owned))goto done;
#endif
 memset(&host,0,sizeof host);host.owner=&owned;
 palette_host composer={{&dynclut,&colors,state,host_flush,&host},&host};
 if(!nflstreet_slot_textures_apply(&owned,textures,indices,pal,SCRATCH,host_flush,&host,host_palette,&composer)){fprintf(stderr,"apply failed %u\n",p);goto done;}
#ifdef NFLSTREET_SLOT_ORACLE
 if(!original_slot_apply(&owned,indices,pal,state,&host))goto done;
#endif
 {nflstreet_slot_material a[NFLSTREET_GEAR_TEXTURE_SLOTS],b[NFLSTREET_GEAR_TEXTURE_SLOTS];
  if(!bind_materials(&material_catalog,&owned,heaps+1,0xff,NULL,0,b)||!bind_materials(&material_catalog,&owned,heaps,0x00,NULL,0,a)){fprintf(stderr,"bind failed %u\n",p);goto done;}
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)for(unsigned k=0;k<5;++k){const nflstreet_material_block *x=set_block(&a[s].texture_set,k),*y=set_block(&b[s].texture_set,k);
   if(x->token!=y->token||x->size!=y->size)goto done;for(uint32_t i=0;i<x->size;++i)mt.unwritten+=x->bytes[i]!=y->bytes[i];
   mt.digest=fnv(mt.digest,x->bytes,x->size);mt.digest=fnv(mt.digest,&x->token,4);}
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){mt.images+=be32(a[s].texture_set.object.bytes+0x10);mt.palettes+=be32(a[s].texture_set.object.bytes+0x18);++mt.sets;}
  mt.flushes+=heaps[0].flush_count;mt.digest=fnv(mt.digest,heaps[0].flushes,heaps[0].flush_count*sizeof(flush_request));
#ifdef NFLSTREET_SLOT_ORACLE
  if(!original_slot_bind(&owned,a,heaps,b,heaps+1))goto done;
#endif
  static uint8_t unconfigured[2][MATERIAL_ARENA];for(unsigned f=0;f<2;++f)memcpy(unconfigured[f],heaps[f].arena,heaps[f].used);
  if(!nflstreet_slot_materials_configure(&material_catalog,a)||!nflstreet_slot_materials_configure(&material_catalog,b)){fprintf(stderr,"configure failed %u\n",p);goto done;}
#ifdef NFLSTREET_SLOT_ORACLE
  if(!original_slot_configure(a,heaps,unconfigured[0],b,heaps+1,unconfigured[1]))goto done;
#endif
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)mt.digest=fnv(mt.digest,a[s].texture_set.textures.bytes,a[s].texture_set.textures.size);
  /* 801639C8 material hookup; controlled variants rotate per player: empty
     tattoo slots, palette-less entries, rotated slot names. */
  {nflstreet_slot_material v[NFLSTREET_GEAR_TEXTURE_SLOTS];static uint8_t entries[NFLSTREET_GEAR_TEXTURE_SLOTS][0x40];
   unsigned variant=mt.hookups%4u;memcpy(v,a,sizeof v);
   for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){nflstreet_texture_set *t=&v[s].texture_set;
    if(t->entries.bytes&&t->entries.size<=sizeof entries[s]){memcpy(entries[s],t->entries.bytes,t->entries.size);t->entries.bytes=entries[s];}
    if(variant==1&&s>=19&&s<=22)v[s].entries=0;
    if(variant==2&&s%3==0&&t->entries.bytes){entries[s][12]=0;entries[s][13]=0;}
    if(variant==3)v[s].material=a[(s+7u)%NFLSTREET_GEAR_TEXTURE_SLOTS].material;}
   nflstreet_actor_material hooked[NFLSTREET_ACTOR_MATERIALS_MAX];unsigned instances=0;
   if(!nflstreet_actor_materials_hook(&material_catalog,v,hooked,&instances)){fprintf(stderr,"hook failed %u\n",p);goto done;}
   for(unsigned i=0;i<instances;++i){mt.digest=fnv(mt.digest,&hooked[i].shader,4);mt.digest=fnv(mt.digest,&hooked[i].stage_count,4);
    mt.digest=fnv(mt.digest,hooked[i].stages,sizeof hooked[i].stages);mt.hooked_two_pass+=hooked[i].shader;}
#ifdef NFLSTREET_SLOT_ORACLE
   if(!original_actor_hook(v,hooked,instances))goto done;
#endif
   ++mt.hookups;}
  if(captured&&!live_materials(live,p,&owned))goto done;}
 if(captured)for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const nflstreet_slot_texture *slot=owned.slots+s;
  if(slot->token!=be32(live+(DESTINATIONS-0x80000000u)+p*100u+4u*s)||slot->template_size!=be32(live+(TEMPLATE_SIZES-0x80000000u)+4u*s)){
   fprintf(stderr,"live layout mismatch player%u slot%u\n",p,s);goto done;}++t->layout;
  const uint8_t *mine=nflstreet_slot_texture_bytes(&owned,s),*theirs=live+(slot->token-0x80000000u);
  if(memcmp(mine,theirs,slot->template_size)){size_t from=0,to=0;palette_data(&owned,s,&from,&to);
   fprintf(stderr,"live bytes mismatch player%u slot%u (palette data %zu..%zu)\n",p,s,from,to);goto done;}
  ++t->identical;t->composed+=slot->applied_palette!=UINT32_MAX;
 }
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){t->digest=fnv(t->digest,host.flushes[s],host.flush_count[s]*sizeof(flush_request));t->flushes+=host.flush_count[s];
  t->digest=fnv(t->digest,&owned.slots[s].applied_index,4);t->digest=fnv(t->digest,&owned.slots[s].applied_palette,4);}
 t->digest=fnv(t->digest,owned.block,owned.block_size);t->palettes+=host.palettes;++t->players;ok=1;
done:nflstreet_slot_textures_close(&owned);return ok;
}
/* Controlled texture sets: synthetic unrelocated MMAPv2 files with 0..3
   images, 0..2 palettes, all GX texture formats, power-of-two and other sizes,
   1..4 mip levels, palette formats 0/1/2/6 around the 0x7F size threshold,
   plus the empty (token 0) set. */
#define SWEEP_MMAP 0x81170000u
#define SWEEP_CASES 512u
#define CONFIGURE_ROUNDS 64u
static uint32_t material_seed=0x2468aceu;
static uint32_t material_random(void){material_seed=material_seed*1664525u+1013904223u;return material_seed>>8;}
static void put32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void put16b(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static size_t synthetic_mmap(uint8_t *b,size_t cap,unsigned n){
 static const uint16_t formats[]={0,1,2,3,4,5,6,8,9,10,14},sides[]={1,2,4,8,16,32,64,128,256,3,5,24,48,100},palette_formats[]={0,1,2,6};
 static const uint32_t palette_sizes[]={0x20,0x7f,0x80,0x200};
 unsigned images=n%4,palettes=(n/4)%3;memset(b,0,cap);size_t at=40;
 memcpy(b,"MMAP",4);put16b(b+4,2);put16b(b+12,images);put16b(b+14,images);put16b(b+16,palettes);
 size_t rows=at;at+=images*12u;size_t headers=at;at+=images*16u;size_t pals=at;at+=palettes*12u;size_t names=at;at+=images*16u;
 put32(b+20,(uint32_t)rows);put32(b+24,(uint32_t)headers);put32(b+28,(uint32_t)pals);put32(b+32,(uint32_t)names);
 for(unsigned i=0;i<images;++i){uint8_t *r=b+rows+i*12u,*h=b+headers+i*16u;
  put16b(r,material_random()%3);put16b(r+2,1u+material_random()%4u);put16b(r+4,images-1u-i);put16b(r+8,palettes?material_random()%palettes:0);
  put16b(h,sides[material_random()%14]);put16b(h+2,sides[material_random()%14]);put16b(h+4,formats[material_random()%11]);
  at=(at+31u)&~(size_t)31u;put32(h+8,0x40);put32(h+12,(uint32_t)at);at+=0x40;}
 for(unsigned i=0;i<palettes;++i){uint8_t *r=b+pals+i*12u;uint32_t size=palette_sizes[material_random()%4];
  put16b(r,1);put16b(r+2,palette_formats[material_random()%4]);at=(at+31u)&~(size_t)31u;put32(r+4,size);put32(r+8,(uint32_t)at);at+=size;}
 return at;
}
static int material_sweep(void){
 static uint8_t unrelocated[0x2000],relocated[0x2000];static material_heap h[2];
 for(unsigned n=0;n<=SWEEP_CASES;++n){size_t size=n<SWEEP_CASES?synthetic_mmap(unrelocated,sizeof unrelocated,n):0;uint32_t token=n<SWEEP_CASES?SWEEP_MMAP:0;
  nflstreet_texture_set t[2];
  for(unsigned f=0;f<2;++f){memcpy(relocated,unrelocated,sizeof relocated);material_reset(h+f,f?0xff:0x00,NULL,0);
   if(!nflstreet_texture_set_build(token?relocated:NULL,size,token,material_allocate,h+f,material_flush,h+f,t+f)){fprintf(stderr,"sweep build failed %u\n",n);return 0;}
   for(unsigned k=0;k<5;++k){const nflstreet_material_block *x=set_block(t+f,k);mt.digest=fnv(mt.digest,x->bytes,x->size);}}
#ifdef NFLSTREET_SLOT_ORACLE
  if(!original_texture_set(unrelocated,size,token,relocated,t,h,t+1,h+1))return 0;
#endif
  ++mt.sweeps;}
 /* 80164DD8 with random settings lists over 25 synthetic sets named like the slots. */
 static uint8_t buffers[NFLSTREET_GEAR_TEXTURE_SLOTS][0x2000],list[64*(NFLSTREET_SLOT_MATERIAL_NAME_BYTES+24u)];
 static const float biases[]={-6.0f,-4.0f,-3.99f,-1.3f,0.0f,0.49f,3.99f,4.0f,7.5f};
 for(unsigned round=0;round<CONFIGURE_ROUNDS;++round){nflstreet_slot_material m[NFLSTREET_GEAR_TEXTURE_SLOTS];material_reset(h,0x00,NULL,0);
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){size_t size=synthetic_mmap(buffers[s],sizeof buffers[s],(round*25u+s)%SWEEP_CASES);
   memset(m+s,0,sizeof m[s]);m[s].material=nflstreet_slot_material_name(&material_catalog,s);m[s].destination=SWEEP_MMAP+0x2000u*s;
   if(!nflstreet_texture_set_build(buffers[s],size,m[s].destination,material_allocate,h,material_flush,h,&m[s].texture_set))return 0;
   m[s].set=m[s].texture_set.object.token;m[s].entries=be32(m[s].texture_set.object.bytes+0x24);}
  static uint8_t unconfigured[MATERIAL_ARENA];memcpy(unconfigured,h->arena,h->used);
  unsigned count=1u+material_random()%40u;
  for(unsigned i=0;i<count;++i){uint8_t *e=list+i*(NFLSTREET_SLOT_MATERIAL_NAME_BYTES+24u),*w=e+NFLSTREET_SLOT_MATERIAL_NAME_BYTES;memset(e,0,NFLSTREET_SLOT_MATERIAL_NAME_BYTES+24u);
   strcpy((char*)e,nflstreet_slot_material_name(&material_catalog,material_random()%NFLSTREET_GEAR_TEXTURE_SLOTS));
   float bias=biases[material_random()%9];uint32_t bits;memcpy(&bits,&bias,4);
   w[0]=(uint8_t)(material_random()%3==0);put32(w+4,material_random()%6);put32(w+8,material_random()%2);put32(w+12,bits);
   put32(w+16,material_random()%3);w[20]=(uint8_t)(material_random()%2);w[21]=(uint8_t)(material_random()%2);}
  nflstreet_slot_material_catalog custom=material_catalog;custom.settings=list;custom.setting_count=count;
  if(!nflstreet_slot_materials_configure(&custom,m)){fprintf(stderr,"configure sweep failed %u\n",round);return 0;}
  for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)mt.digest=fnv(mt.digest,m[s].texture_set.textures.bytes,m[s].texture_set.textures.size);
#ifdef NFLSTREET_SLOT_ORACLE
  if(!original_configure_sweep(m,h,unconfigured,list,count))return 0;
#endif
  mt.configure_entries+=count;}
 return 1;
}
int main(int argc,char **argv){
#ifdef NFLSTREET_SLOT_ORACLE
 if(argc!=9)return 2;
#else
 if(argc!=8)return 2;
#endif
 size_t sizes[4],ram_size=0;uint8_t *data[4];for(unsigned i=0;i<4;++i)if(!(data[i]=load_data(argv[1+i],sizes+i)))return 3;
 uint8_t *live=load_data(argv[6],&ram_size);if(!live||ram_size<0x01800000u)return 3;
 size_t material_size=0;uint8_t *materials=load_data(argv[7],&material_size);
 if(!materials||!nflstreet_slot_material_catalog_open(materials,material_size,&material_catalog))return 3;mt.digest=2166136261u;
 nflstreet_archive textures,dynamic;nflstreet_gear_texture_catalog catalog;nflstreet_team_table gear;
 if(!nflstreet_archive_open(data[0],sizes[0],&textures)||!nflstreet_archive_open(data[1],sizes[1],&dynamic)||
    !nflstreet_gear_texture_catalog_open(data[2],sizes[2],&catalog)||!nflstreet_team_table_open_tag(data[3],sizes[3],0x52414547u,&gear)||
    !nflstreet_color_table_build(data[3],sizes[3],&catalog,&colors)||
    !nflstreet_gear_records_build(&gear,&catalog,&records)||!nflstreet_dynclut_table_open(&dynamic,&catalog,&dynclut))return 3;
#ifdef NFLSTREET_SLOT_ORACLE
 if(!initialize_slot_oracle(argv[8],argv[6],&catalog,&textures,&dynclut,&colors))return 4;
#endif
 if(!material_sweep())return 6;
 /* Boot-built tables in the live snapshot: 8030A898 colors and r13-5B48 op lists. */
 if(memcmp(colors.bytes,live+(COLOR_TABLE-0x80000000u),COLOR_ENTRIES*16u))return 8;
 for(unsigned i=COLOR_ENTRIES*16u;i<NFLSTREET_COLOR_TABLE_BYTES;++i)if(colors.bytes[i])return 8;
 uint32_t live_ops=be32(live+(DYNCLUT_TABLE-0x80000000u));if(dynclut.entries!=129)return 8;
 for(unsigned e=0;e<dynclut.entries;++e){const uint8_t *row=live+(live_ops-0x80000000u)+8u*e;
  if(be32(row)!=dynclut.count[e])return 8;
  for(unsigned k=0;k<dynclut.count[e];++k){const uint8_t *o=live+(be32(row+4)-0x80000000u)+8u*k;const nflstreet_dynclut_op *n=dynclut.ops+dynclut.first[e]+k;
   if(be32(o)!=n->kind||o[4]!=n->start||o[5]!=n->end||o[6]!=n->color||o[7]!=n->level){fprintf(stderr,"live op mismatch entry%u op%u\n",e,k);return 8;}}}
 totals t={0};t.digest=2166136261u;unsigned roster=0;
 for(unsigned p=0;p<CAPTURED_PLAYERS;++p){nflstreet_gear_texture_plan plan;
  if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,live+(PLAYER_RECORDS-0x80000000u)+p*0x68u,&plan)||
     !check_player(&catalog,&textures,&plan,live,1,p,live+(PLAYER_RECORDS-0x80000000u)+p*0x68u+0x4cu,&t))return 6;
  if(memcmp(live+(COLOR_STATES-0x80000000u)+p*24u,live+(PLAYER_RECORDS-0x80000000u)+p*0x68u+0x4cu,24))return 8;}
 /* Controlled requests: palettes on every -1 slot (800E2E48 ignores them). */
 {nflstreet_gear_texture_plan plan;
  if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,live+(PLAYER_RECORDS-0x80000000u),&plan))return 6;
  for(unsigned s=1;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(plan.indices[s]==-1)plan.palettes[s]=0x22u;
  if(!check_player(&catalog,&textures,&plan,live,0,0,live+(PLAYER_RECORDS-0x80000000u)+0x4cu,&t))return 6;}
 for(unsigned entry=1;entry<=32;++entry){char path[4096];size_t n=0;nflstreet_team_table play;unsigned team_field=UINT32_MAX;
  if(snprintf(path,sizeof path,"%s/db_teams/%05u.bin",argv[5],entry)>=(int)sizeof path)return 5;uint8_t *db=load_data(path,&n);
  if(!db||!nflstreet_team_table_open_tag(db,n,0x59414c50u,&play))return 5;
  for(unsigned i=0;i<play.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&play,i,&f))return 5;if(f.tag==0x44494754u)team_field=i;}
  for(unsigned row=0;row<play.used;++row){uint32_t team;if(!nflstreet_team_table_number(&play,row,team_field,&team))return 5;if(!team)continue;
   for(unsigned context=0;context<2;++context){uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];nflstreet_gear_texture_plan plan;
    memcpy(player,live+(PLAYER_RECORDS-0x80000000u)+(context?7u:0u)*0x68u,sizeof player);
    if(!nflstreet_gear_player_record_from_play(&play,row,&records,player)||
       !nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,player,&plan)||
       !check_player(&catalog,&textures,&plan,live,0,row,player+0x4c,&t))return 6;}
   ++roster;}
  free(db);
 }
 printf("Selected slot textures: %u players (%u captured, 1 controlled, %u roster rows x2 contexts); %u color entries and %u pladyncl op lists (%u ops) match live; %u live slots match layout/size, %u byte-identical (%u recolored); %u cache flushes, %u palette compositions; digest %08x.\n",
  t.players,CAPTURED_PLAYERS,roster,COLOR_ENTRIES,dynclut.entries,dynclut.total,t.layout,t.identical,t.composed,t.flushes,t.palettes,t.digest);
printf("Slot materials: %u controlled texture sets, %u controlled 80164DD8 settings in %u lists; %u texture sets (%u images, %u palettes), %u cache flushes, %u bytes left as allocator contents; %u live sets match the captured binding table and objects after 80164DD8 (draw-time writes skipped: %u register-id, %u TLUT-name, %u TLUT TMEM bytes); %u live actor material instances match the 801639C8 hookup (%u TwoPass); %u controlled hookups (%u TwoPass instances); digest %08x.\n",
  mt.sweeps,mt.configure_entries,CONFIGURE_ROUNDS,mt.sets,mt.images,mt.palettes,mt.flushes,mt.unwritten,mt.live_sets,mt.live_ids,mt.live_names,mt.live_tmem,mt.live_instances,mt.live_two_pass,mt.hookups,mt.hooked_two_pass,mt.digest);
#ifdef NFLSTREET_SLOT_ORACLE
 printf("Original slot material checks: %u controlled and %u selected 801EBD38 texture sets x2 allocator fills match every byte, token and flush; %u selected and %u controlled 80164DD8 runs match every texture object; %u 801639C8 material hookups match every instance shader, stage count and stage.\n",sweep_sets,bind_sets,configure_sets,configure_sweeps,hook_runs);
 printf("Original slot texture checks: %u 800D9948 sweep cases; %u 800E2958 layouts, %u 800E2AF0 template sets, %u 800E2CAC players (%u slot loads, %u flushes, %u 800D9C0C compositions); archive acquisition supplied.\n",
  slot_sweeps,slot_layouts,slot_templates,slot_players,slot_loads,slot_flushes,slot_compositions);
#endif
 nflstreet_dynclut_table_close(&dynclut);for(unsigned i=0;i<4;++i)free(data[i]);free(live);free(materials);return roster==402?0:9;
}
