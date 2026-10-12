#include "selected_appearance_assets.h"
#include "skeleton_startup.h"
#include "face_postattach.h"
#include "hair_postattach.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *owned_inputs[5];static nflstreet_body_resource_pool owned_bodies;
static nflstreet_archive local_hair,animations;static nflstreet_headwear_catalog gear;
static nflstreet_skeleton_startup skeletons;static unsigned asset_assemblies,asset_guards;
static uint8_t *input(const char *path,size_t *size){FILE *f=fopen(path,"rb");if(!f||fseek(f,0,SEEK_END))return NULL;long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET))return NULL;
 uint8_t *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n)return NULL;fclose(f);*size=(size_t)n;return b;}
static int initialize_assets(char **argv){
 size_t sizes[5];for(unsigned i=0;i<5;++i)if(!(owned_inputs[i]=input(argv[5+i],sizes+i)))return 0;
 nflstreet_skeleton_startup_catalog catalog;
 return nflstreet_body_resource_pool_open(owned_inputs[0],sizes[0],0x20000000u,&owned_bodies)&&nflstreet_body_resource_pool_process(&owned_bodies)&&
  nflstreet_archive_open(owned_inputs[1],sizes[1],&local_hair)&&nflstreet_archive_open(owned_inputs[2],sizes[2],&animations)&&
  nflstreet_headwear_catalog_open(owned_inputs[3],sizes[3],&gear)&&
  nflstreet_skeleton_startup_catalog_open(owned_inputs[4],sizes[4],&catalog)&&nflstreet_skeleton_startup_open(&animations,&catalog,0x30000000u,&skeletons);
}
typedef struct {void *bytes[15];uint32_t tokens[15];unsigned count,frees;} temporary;
static int allocate_asset(void *opaque,uint32_t n,uint32_t width,nflstreet_model_postattach_segment *out){
 temporary *t=opaque;if(t->count>=15||(uint64_t)n*width>0x10000u)return 0;unsigned i=t->count++;size_t size=(size_t)n*width;
 t->bytes[i]=malloc(size?size:1);if(!t->bytes[i])return 0;t->tokens[i]=0x70000000u+i*0x10000u;
 *out=(nflstreet_model_postattach_segment){t->tokens[i],t->bytes[i],size};return 1;
}
static void release_asset(void *opaque,uint32_t token){temporary *t=opaque;for(unsigned i=0;i<t->count;++i)if(t->tokens[i]==token&&t->bytes[i]){free(t->bytes[i]);t->bytes[i]=NULL;++t->frees;return;}}
static int check_joined_assets(const nflstreet_team_player_resolver *resolver,const nflstreet_archive *textures,
 const uint8_t *db,size_t size,uint32_t team,uint32_t player){
 nflstreet_selected_appearance_context context={resolver,&owned_bodies,&gear,&local_hair,textures,0,0x10000000u,0x50000000u,0x60000000u};
 if(!asset_assemblies){nflstreet_selected_appearance_assets marker={0},rejected;marker.geometry.base.manifest.player_id=0x12345678u;
  const uint32_t collisions[]={0x10000000u,0x20000000u,0x50000000u};
  for(unsigned i=0;i<3;++i){context.texture_identity=collisions[i];rejected=marker;
   if(nflstreet_selected_appearance_assets_open(db,size,team,player,&context,&rejected)||memcmp(&rejected,&marker,sizeof marker))return 0;++asset_guards;}
  context.texture_identity=0x60000000u;
 }
 for(unsigned variant=0;variant<3;++variant){context.body_variant=variant;nflstreet_selected_appearance_assets r={0};temporary t={0};
  if(!nflstreet_selected_appearance_assets_open(db,size,team,player,&context,&r)||r.geometry.base.manifest.player_id!=player||
     !nflstreet_face_resource_pair_bind(&r.geometry.base.face,skeletons.resources,0x18000000u,allocate_asset,release_asset,&t)||
     !nflstreet_hair_resources_bind(&r.geometry.hair,skeletons.resources+5,0x19000000u,allocate_asset,release_asset,&t)||t.count!=t.frees)return 0;
  uint8_t request[24];memset(request,0xa5,24);
  if(!nflstreet_texture_request(request,24,r.face_texture.plan.index,UINT32_MAX,0)||request[12]!=1||request[13]!=0)return 0;
  ++asset_assemblies;nflstreet_selected_appearance_assets_close(&r);
 }return 1;
}
static void close_assets(void){nflstreet_body_resource_pool_close(&owned_bodies);nflstreet_skeleton_startup_close(&skeletons);for(unsigned i=0;i<5;++i)free(owned_inputs[i]);}
#define NFLSTREET_SELECTED_ASSETS 1
#include "face_texture_check.c"
