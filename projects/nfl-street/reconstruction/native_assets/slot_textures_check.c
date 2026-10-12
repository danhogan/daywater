/* Owned selected slot textures. The 14 captured players are compared with the
   live destinations in the same snapshot (layout, sizes and bytes; palette
   record0 data may differ only by pending 800D9C0C composition). All roster
   rows in both captured team contexts then load and apply natively. Slot1 is
   the separate face path: captured players use their live slot1 request,
   roster rows keep the template. */
#include "slot_textures.h"
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
static int host_palette(void *opaque,unsigned slot,uint32_t palette){host_log *h=opaque;h->palette_value[slot]=palette;++h->palettes;return 1;}
/* Byte range of destination palette record0 data (801C5958(dest,0) -> +8 pointer, +4 size). */
static int palette_data(const nflstreet_slot_textures *t,unsigned s,size_t *from,size_t *to){
 const nflstreet_slot_texture *slot=t->slots+s;const uint8_t *d=nflstreet_slot_texture_bytes(t,s);
 uint32_t records=be32(d+28);if(records<slot->token||records-slot->token>slot->template_size-12u)return 0;
 const uint8_t *r=d+(records-slot->token);uint32_t at=be32(r+8),size=be32(r+4);
 if(at<slot->token||at-slot->token>slot->template_size||size>slot->template_size-(at-slot->token))return 0;
 *from=at-slot->token;*to=*from+size;return 1;
}
#ifdef NFLSTREET_SLOT_ORACLE
static unsigned slot_layouts,slot_templates,slot_players,slot_loads,slot_flushes,slot_compositions;
static int initialize_slot_oracle(const char *,const char *,const nflstreet_gear_texture_catalog *,const nflstreet_archive *);
static int original_slot_open(const nflstreet_slot_textures *);
static int original_slot_apply(const nflstreet_slot_textures *,const int32_t *,const uint32_t *,const host_log *);
#endif
static nflstreet_gear_records records;
static host_log host;
typedef struct {unsigned layout,identical,composed,players,flushes,palettes;uint32_t digest;} totals;
static int check_player(const nflstreet_gear_texture_catalog *catalog,const nflstreet_archive *textures,const nflstreet_gear_texture_plan *plan,
 const uint8_t *live,int captured,unsigned p,totals *t){
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
 if(!nflstreet_slot_textures_apply(&owned,textures,indices,pal,SCRATCH,host_flush,host_palette,&host)){fprintf(stderr,"apply failed %u\n",p);goto done;}
#ifdef NFLSTREET_SLOT_ORACLE
 if(!original_slot_apply(&owned,indices,pal,&host))goto done;
#endif
 if(captured)for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){const nflstreet_slot_texture *slot=owned.slots+s;
  if(slot->token!=be32(live+(DESTINATIONS-0x80000000u)+p*100u+4u*s)||slot->template_size!=be32(live+(TEMPLATE_SIZES-0x80000000u)+4u*s)){
   fprintf(stderr,"live layout mismatch player%u slot%u\n",p,s);goto done;}++t->layout;
  const uint8_t *mine=nflstreet_slot_texture_bytes(&owned,s),*theirs=live+(slot->token-0x80000000u);
  if(!memcmp(mine,theirs,slot->template_size)){++t->identical;continue;}
  size_t from,to;if(slot->applied_palette==UINT32_MAX||!palette_data(&owned,s,&from,&to)||memcmp(mine,theirs,from)||
     memcmp(mine+to,theirs+to,slot->template_size-to)){fprintf(stderr,"live bytes mismatch player%u slot%u\n",p,s);goto done;}
  ++t->composed;
 }
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){t->digest=fnv(t->digest,host.flushes[s],host.flush_count[s]*sizeof(flush_request));t->flushes+=host.flush_count[s];
  t->digest=fnv(t->digest,&owned.slots[s].applied_index,4);t->digest=fnv(t->digest,&owned.slots[s].applied_palette,4);}
 t->digest=fnv(t->digest,owned.block,owned.block_size);t->palettes+=host.palettes;++t->players;ok=1;
done:nflstreet_slot_textures_close(&owned);return ok;
}
int main(int argc,char **argv){
#ifdef NFLSTREET_SLOT_ORACLE
 if(argc!=8)return 2;
#else
 if(argc!=7)return 2;
#endif
 size_t sizes[4],ram_size=0;uint8_t *data[4];for(unsigned i=0;i<4;++i)if(!(data[i]=load_data(argv[1+i],sizes+i)))return 3;
 uint8_t *live=load_data(argv[6],&ram_size);if(!live||ram_size<0x01800000u)return 3;
 nflstreet_archive textures,dynamic;nflstreet_gear_texture_catalog catalog;nflstreet_team_table gear;
 if(!nflstreet_archive_open(data[0],sizes[0],&textures)||!nflstreet_archive_open(data[1],sizes[1],&dynamic)||
    !nflstreet_gear_texture_catalog_open(data[2],sizes[2],&catalog)||!nflstreet_team_table_open_tag(data[3],sizes[3],0x52414547u,&gear)||
    !nflstreet_gear_records_build(&gear,&catalog,&records))return 3;
#ifdef NFLSTREET_SLOT_ORACLE
 if(!initialize_slot_oracle(argv[7],argv[6],&catalog,&textures))return 4;
#endif
 totals t={0};t.digest=2166136261u;unsigned roster=0;
 for(unsigned p=0;p<CAPTURED_PLAYERS;++p){nflstreet_gear_texture_plan plan;
  if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,live+(PLAYER_RECORDS-0x80000000u)+p*0x68u,&plan)||
     !check_player(&catalog,&textures,&plan,live,1,p,&t))return 6;}
 /* Controlled requests: palettes on every -1 slot (800E2E48 ignores them). */
 {nflstreet_gear_texture_plan plan;
  if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,live+(PLAYER_RECORDS-0x80000000u),&plan))return 6;
  for(unsigned s=1;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(plan.indices[s]==-1)plan.palettes[s]=0x22u;
  if(!check_player(&catalog,&textures,&plan,live,0,0,&t))return 6;}
 for(unsigned entry=1;entry<=32;++entry){char path[4096];size_t n=0;nflstreet_team_table play;unsigned team_field=UINT32_MAX;
  if(snprintf(path,sizeof path,"%s/db_teams/%05u.bin",argv[5],entry)>=(int)sizeof path)return 5;uint8_t *db=load_data(path,&n);
  if(!db||!nflstreet_team_table_open_tag(db,n,0x59414c50u,&play))return 5;
  for(unsigned i=0;i<play.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&play,i,&f))return 5;if(f.tag==0x44494754u)team_field=i;}
  for(unsigned row=0;row<play.used;++row){uint32_t team;if(!nflstreet_team_table_number(&play,row,team_field,&team))return 5;if(!team)continue;
   for(unsigned context=0;context<2;++context){uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];nflstreet_gear_texture_plan plan;
    memcpy(player,live+(PLAYER_RECORDS-0x80000000u)+(context?7u:0u)*0x68u,sizeof player);
    if(!nflstreet_gear_player_record_from_play(&play,row,&records,player)||
       !nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,player,&plan)||
       !check_player(&catalog,&textures,&plan,live,0,row,&t))return 6;}
   ++roster;}
  free(db);
 }
 printf("Selected slot textures: %u players (%u captured, 1 controlled, %u roster rows x2 contexts); %u live slots match layout/size, %u byte-identical, %u differ only in palette record0 pending 800D9C0C; %u cache flushes, %u palette compositions reported; digest %08x.\n",
  t.players,CAPTURED_PLAYERS,roster,t.layout,t.identical,t.composed,t.flushes,t.palettes,t.digest);
#ifdef NFLSTREET_SLOT_ORACLE
 printf("Original slot texture checks: %u 800E2958 layouts, %u 800E2AF0 template sets, %u 800E2CAC players (%u slot loads, %u flushes, %u 800D9C0C requests); archive acquisition supplied.\n",
  slot_layouts,slot_templates,slot_players,slot_loads,slot_flushes,slot_compositions);
#endif
 for(unsigned i=0;i<4;++i)free(data[i]);free(live);return roster==402?0:9;
}
