#include "player_surface.h"
#include "face_texture.h"
#include <stdlib.h>
#include <string.h>
void nflstreet_player_surface_close(nflstreet_player_surface *s){
 if(!s)return;for(unsigned i=0;i<s->allocation_count;++i)free(s->allocations[i].bytes);
 nflstreet_slot_textures_close(&s->textures);memset(s,0,sizeof *s);
}
static int overlaps(uint32_t a,size_t an,uint32_t b,size_t bn){return(uint64_t)a<(uint64_t)b+bn&&(uint64_t)b<(uint64_t)a+an;}
typedef struct {nflstreet_player_surface *surface;const nflstreet_player_surface_context *context;} heap;
static int allocate(void *opaque,uint32_t size,uint32_t *token,uint8_t **bytes){
 heap *h=opaque;nflstreet_player_surface *s=h->surface;const nflstreet_player_surface_context *c=h->context;
 if(size>UINT32_MAX-31u||s->allocation_count>=NFLSTREET_SURFACE_BLOCKS)return 0;
 uint32_t rounded=(size+31u)&~31u;
 if(rounded>c->material_capacity-s->material_used)return 0;
 uint8_t *p=malloc(rounded);if(!p)return 0;memset(p,c->allocator_fill,rounded);
 *token=c->material_token+s->material_used;*bytes=p;s->material_used+=rounded;
 s->allocations[s->allocation_count++]=(nflstreet_material_block){*token,size,p};return 1;
}
int nflstreet_player_surface_open(const nflstreet_player_surface_context *c,const nflstreet_gear_texture_plan *p,
 int32_t face_index,uint32_t face_palette,const uint8_t state[NFLSTREET_COLOR_STATE_BYTES],nflstreet_player_surface *out){
 if(!c||!p||!state||!out||out->textures.block||out->allocation_count||!c->gear||!c->materials||!c->textures||
    !c->dynclut||!c->colors||!c->flush||!c->scratch_token||!c->material_token||
    (c->material_token&31u)||!c->material_capacity||c->material_capacity>UINT32_MAX-c->material_token)return 0;
 nflstreet_player_surface s={0};
 if(!nflstreet_slot_textures_open(c->gear,c->textures,c->destination_token,&s.textures))return 0;
 if(s.textures.scratch_capacity>UINT32_MAX-c->scratch_token||
    overlaps(c->destination_token,s.textures.block_size,c->scratch_token,s.textures.scratch_capacity)||
    overlaps(c->destination_token,s.textures.block_size,c->material_token,c->material_capacity)||
    overlaps(c->scratch_token,s.textures.scratch_capacity,c->material_token,c->material_capacity))goto fail;
 int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS];uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS];
 memcpy(indices,p->indices,sizeof indices);memcpy(palettes,p->palettes,sizeof palettes);
 if(p->fallback){indices[0]=p->fallback_index;palettes[0]=p->fallback_palette;}
 indices[1]=face_index;palettes[1]=face_palette;
 nflstreet_palette_context palette={c->dynclut,c->colors,state,c->flush,c->flush_opaque};heap h={&s,c};
 if(!nflstreet_slot_textures_apply(&s.textures,c->textures,indices,palettes,c->scratch_token,c->flush,c->flush_opaque,nflstreet_palette_compose_slot,&palette)||
    !nflstreet_slot_materials_bind(c->materials,&s.textures,allocate,&h,c->flush,c->flush_opaque,s.bindings)||
    !nflstreet_slot_materials_configure(c->materials,s.bindings)||
    !nflstreet_actor_materials_hook(c->materials,s.bindings,s.instances,&s.instance_count))goto fail;
 *out=s;return 1;
fail:nflstreet_player_surface_close(&s);return 0;
}

int nflstreet_player_surface_from_player(const nflstreet_player_surface_context *c,const nflstreet_gear_records *gear,
 const nflstreet_archive *dynamic,const uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],const char *name,nflstreet_player_surface *out){
 if(!c||!gear||!dynamic||!player||!name||!out)return 0;
 nflstreet_gear_texture_plan plan;nflstreet_face_texture_plan face;
 if(!nflstreet_gear_texture_plan_build(c->gear,gear,c->textures,dynamic,player,&plan)||
    !nflstreet_face_texture_plan_make(c->textures,name,&face))return 0;
 return nflstreet_player_surface_open(c,&plan,face.index,UINT32_MAX,player+0x4c,out);
}
