/* Reuse data readers and the independently exercised stage harness helpers.
   Its main remains compiled but is not run by this joined-owner gate. */
#define main slot_stage_main
#include "slot_textures_check.c"
#undef main
#include "player_surface.h"
#include "../menus/team_identity.h"
/* Only player_surface.c is built with these malloc/free aliases. This counts
   its material-block lifetime across success and every partial failure. */
static unsigned surface_live_allocations,surface_allocation_attempts,surface_fail_at=UINT32_MAX;
void *nflstreet_surface_test_malloc(size_t size){if(surface_allocation_attempts++==surface_fail_at)return NULL;void *p=malloc(size);if(p)++surface_live_allocations;return p;}
void nflstreet_surface_test_free(void *p){if(p){if(!surface_live_allocations)abort();--surface_live_allocations;}free(p);}
typedef struct {flush_request calls[2048];unsigned count,fail_at;} surface_log;
static int surface_flush(void *opaque,uint32_t token,uint32_t size){surface_log *l=opaque;
 if(l->count==l->fail_at||l->count>=2048)return 0;l->calls[l->count++]=(flush_request){token,size};return 1;}
static int compare_surface(const nflstreet_player_surface_context *context,const nflstreet_gear_texture_plan *plan,
 int32_t face,uint32_t face_palette,const uint8_t *state,unsigned *failures){
 surface_log joined={.fail_at=UINT32_MAX},stages={.fail_at=UINT32_MAX};
 nflstreet_player_surface_context c=*context;c.flush=surface_flush;c.flush_opaque=&joined;
 nflstreet_player_surface s={0};if(!nflstreet_player_surface_open(&c,plan,face,face_palette,state,&s))return 0;
 nflstreet_slot_textures t={0};nflstreet_slot_material binding[NFLSTREET_GEAR_TEXTURE_SLOTS];
 nflstreet_actor_material instances[NFLSTREET_ACTOR_MATERIALS_MAX];unsigned count=0;int ok=0;
 int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS];uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS];
 memcpy(indices,plan->indices,sizeof indices);memcpy(palettes,plan->palettes,sizeof palettes);
 if(plan->fallback){indices[0]=plan->fallback_index;palettes[0]=plan->fallback_palette;}
 indices[1]=face;palettes[1]=face_palette;
 nflstreet_palette_context composer={c.dynclut,c.colors,state,surface_flush,&stages};
 material_reset(heaps,c.allocator_fill,NULL,0);
 if(!nflstreet_slot_textures_open(c.gear,c.textures,c.destination_token,&t)||
    !nflstreet_slot_textures_apply(&t,c.textures,indices,palettes,c.scratch_token,surface_flush,&stages,nflstreet_palette_compose_slot,&composer)||
    !nflstreet_slot_materials_bind(c.materials,&t,material_allocate,heaps,surface_flush,&stages,binding)||
    !nflstreet_slot_materials_configure(c.materials,binding)||!nflstreet_actor_materials_hook(c.materials,binding,instances,&count))goto done;
 if(t.block_size!=s.textures.block_size||memcmp(t.block,s.textures.block,t.block_size)||
    memcmp(t.slots,s.textures.slots,sizeof t.slots)||count!=s.instance_count||
    memcmp(instances,s.instances,count*sizeof *instances)||heaps->count!=s.allocation_count||
    heaps->used!=s.material_used||joined.count!=stages.count||memcmp(joined.calls,stages.calls,joined.count*sizeof *joined.calls))goto done;
 for(unsigned i=0;i<NFLSTREET_GEAR_TEXTURE_SLOTS;++i){
  if(binding[i].entries!=s.bindings[i].entries||binding[i].set!=s.bindings[i].set||binding[i].destination!=s.bindings[i].destination)goto done;
  for(unsigned k=0;k<5;++k){const nflstreet_material_block *a=set_block(&binding[i].texture_set,k),*b=set_block(&s.bindings[i].texture_set,k);
   if(a->token!=b->token||a->size!=b->size||(a->size&&memcmp(a->bytes,b->bytes,a->size)))goto done;}}
 /* Output reuse rejects, preserving every owned pointer and byte. */
 nflstreet_player_surface before=s;
 if(nflstreet_player_surface_open(&c,plan,face,face_palette,state,&s)||memcmp(&s,&before,sizeof s))goto done;
 ++*failures;ok=1;
done:nflstreet_slot_textures_close(&t);nflstreet_player_surface_close(&s);nflstreet_player_surface_close(&s);return ok&&!surface_live_allocations;
}
int main(int argc,char **argv){
 if(argc!=8)return 2;size_t n[6];uint8_t *b[6];unsigned map[]={1,2,3,4,6,7};
 for(unsigned i=0;i<6;++i)if(!(b[i]=load_data(argv[map[i]],n+i)))return 3;
 nflstreet_archive textures,dynamic;nflstreet_gear_texture_catalog catalog;nflstreet_team_table gear;
 if(n[4]<0x1800000||!nflstreet_archive_open(b[0],n[0],&textures)||!nflstreet_archive_open(b[1],n[1],&dynamic)||
    !nflstreet_gear_texture_catalog_open(b[2],n[2],&catalog)||!nflstreet_team_table_open_tag(b[3],n[3],0x52414547u,&gear)||
    !nflstreet_slot_material_catalog_open(b[5],n[5],&material_catalog)||!nflstreet_gear_records_build(&gear,&catalog,&records)||
    !nflstreet_color_table_build(b[3],n[3],&catalog,&colors)||!nflstreet_dynclut_table_open(&dynamic,&catalog,&dynclut))return 3;
 nflstreet_player_surface_context c={&catalog,&material_catalog,&textures,&dynclut,&colors,0x80500000u,SCRATCH,MATERIAL_HEAP,MATERIAL_ARENA,0,NULL,NULL};
 unsigned players=0,rejections=0,selected=0;
 for(unsigned p=0;p<14;++p){const uint8_t *record=b[4]+PLAYER_RECORDS-0x80000000u+0x68u*p;
  const uint8_t *face=b[4]+REQUEST_RECORDS-0x80000000u+600u*p+24u;nflstreet_gear_texture_plan plan;
  if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,record,&plan))return 4;
  /* The semantic entry point also exercises named-lookup fallback without
     a captured face request. Record bytes are supplied fixtures here. */
  surface_log semantic_log={.fail_at=UINT32_MAX};c.flush=surface_flush;c.flush_opaque=&semantic_log;
  nflstreet_player_surface semantic={0},explicit={0};nflstreet_face_texture_plan fallback;
  if(!nflstreet_face_texture_plan_make(&textures,"MISSING_PLAYER_SURFACE",&fallback)||
     !nflstreet_player_surface_from_player(&c,&records,&dynamic,record,"MISSING_PLAYER_SURFACE",&semantic)||
     !nflstreet_player_surface_open(&c,&plan,fallback.index,UINT32_MAX,record+0x4c,&explicit))return 9;
  if(semantic.textures.block_size!=explicit.textures.block_size||
     memcmp(semantic.textures.block,explicit.textures.block,semantic.textures.block_size)||
     semantic.instance_count!=explicit.instance_count||
     memcmp(semantic.instances,explicit.instances,semantic.instance_count*sizeof *semantic.instances))return 9;
  nflstreet_player_surface_close(&semantic);nflstreet_player_surface_close(&explicit);if(surface_live_allocations)return 10;++selected;
  for(unsigned fill=0;fill<2;++fill){c.allocator_fill=fill?0xff:0;
   if(!compare_surface(&c,&plan,(int32_t)be32(face),be32(face+16),record+0x4c,&rejections))return 5;++players;}
 }
 /* Every callback failure point of a complete player build, including material
    allocation stages, must leave the output unchanged and release new storage. */
 const uint8_t *record=b[4]+PLAYER_RECORDS-0x80000000u;nflstreet_gear_texture_plan plan;
 if(!nflstreet_gear_texture_plan_build(&catalog,&records,&textures,&dynamic,record,&plan))return 4;
 surface_log log={.fail_at=UINT32_MAX};c.flush=surface_flush;c.flush_opaque=&log;
 nflstreet_player_surface complete={0};if(!nflstreet_player_surface_open(&c,&plan,-1,UINT32_MAX,record+0x4c,&complete))return 6;
 unsigned calls=log.count,allocations=complete.allocation_count;nflstreet_player_surface_close(&complete);
 for(unsigned i=0;i<calls+2;++i){nflstreet_player_surface out={.instance_count=123},before=out;
  log.count=0;log.fail_at=i;
  nflstreet_player_surface_context broken=c;
  if(i==calls){log.fail_at=UINT32_MAX;broken.material_capacity=0x40;}
  if(i==calls+1){log.fail_at=UINT32_MAX;broken.scratch_token=broken.destination_token;}
  if(nflstreet_player_surface_open(&broken,&plan,-1,UINT32_MAX,record+0x4c,&out)||memcmp(&out,&before,sizeof out)||surface_live_allocations)return 7;++rejections;
 }
 for(unsigned i=0;i<allocations;++i){nflstreet_player_surface out={0};
  log.count=0;log.fail_at=UINT32_MAX;surface_allocation_attempts=0;surface_fail_at=i;
  if(nflstreet_player_surface_open(&c,&plan,-1,UINT32_MAX,record+0x4c,&out)||surface_live_allocations||out.textures.block)return 11;++rejections;
 }
 surface_fail_at=UINT32_MAX;
 /* Derive a real roster name using the existing normalization owner, rather
    than baking a proprietary name into this checker. */
 {char path[4096],last[64],first[64],name[32];size_t size;unsigned found=0;
  if(snprintf(path,sizeof path,"%s/db_teams/00001.bin",argv[5])>=(int)sizeof path)return 12;
  uint8_t *db=load_data(path,&size);nflstreet_team_table play;if(!db||!nflstreet_team_table_open_tag(db,size,0x59414c50u,&play))return 12;
  unsigned li=UINT32_MAX,fi=UINT32_MAX;
  for(unsigned i=0;i<play.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&play,i,&f))return 12;
   if(f.tag==0x414e4c50u)li=i;if(f.tag==0x414e4650u)fi=i;}
  for(unsigned row=0;row<play.used&&!found;++row){nflstreet_face_texture_plan face;
   if(!nflstreet_team_table_text(&play,row,li,last,sizeof last)||!nflstreet_team_table_text(&play,row,fi,first,sizeof first)||
      !nflstreet_team_identity_name(last,first,26,name)||!nflstreet_face_texture_plan_make(&textures,name,&face))return 12;
   if(face.fallback||face.index<0)continue;
   log.count=0;log.fail_at=UINT32_MAX;nflstreet_player_surface s={0};
   if(!nflstreet_player_surface_from_player(&c,&records,&dynamic,record,name,&s)||s.textures.slots[1].applied_index!=face.index)return 12;
   nflstreet_player_surface_close(&s);if(surface_live_allocations)return 12;found=1;++selected;
  }free(db);if(!found)return 12;
 }
 nflstreet_dynclut_table_close(&dynclut);for(unsigned i=0;i<6;++i)free(b[i]);
 printf("Player surface owner: %u builds (14 captured requests x2 allocator fills) match separate texture/recolor/bind/configure/hook stages including every flush; %u failure/reuse rejections preserve output; %u semantic record/name builds agree; material allocations balance; close is repeatable.\n",players,rejections,selected);return 0;
}
