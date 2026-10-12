#include "slot_textures.h"
#include <stdlib.h>
#include <string.h>
static int decode(const nflstreet_archive *a,uint32_t index,uint8_t **bytes,size_t *size){
 nflstreet_archive_entry e;return index<a->count&&nflstreet_archive_get(a,index,&e)&&nflstreet_archive_decode(&e,bytes,size);
}
void nflstreet_slot_textures_close(nflstreet_slot_textures *t){if(t){free(t->block);memset(t,0,sizeof *t);}}
int nflstreet_slot_textures_open(const nflstreet_gear_texture_catalog *c,const nflstreet_archive *a,uint32_t token,nflstreet_slot_textures *out){
 if(!c||!a||!out||!token)return 0;nflstreet_slot_textures t;memset(&t,0,sizeof t);t.token=token;
 uint8_t *templates[NFLSTREET_GEAR_TEXTURE_SLOTS]={0};size_t total=0;int ok=0;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){nflstreet_slot_texture *slot=t.slots+s;size_t size;
  slot->template_index=nflstreet_gear_texture_template(c,s);
  if(!decode(a,slot->template_index,templates+s,&size)||size>UINT32_MAX-64u)goto done;
  slot->template_size=(uint32_t)size;slot->capacity=slot->template_size+32u-(slot->template_size&31u);
  if(total>(size_t)UINT32_MAX-token-slot->capacity)goto done;
  slot->offset=total;slot->token=token+(uint32_t)total;total+=slot->capacity;
  slot->applied_index=(int32_t)slot->template_index;slot->applied_palette=UINT32_MAX;
  if(slot->template_size>t.scratch_capacity)t.scratch_capacity=slot->template_size;
 }
 if(!(t.block=calloc(total,1)))goto done;t.block_size=total;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){uint8_t *d=t.block+t.slots[s].offset;
  memcpy(d,templates[s],t.slots[s].template_size);
  if(!nflstreet_mmap_relocate(d,t.slots[s].template_size,t.slots[s].token))goto done;}
 ok=1;
done:
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)free(templates[s]);
 if(!ok){free(t.block);return 0;}
 *out=t;return 1;
}
int nflstreet_slot_textures_apply(nflstreet_slot_textures *t,const nflstreet_archive *a,
 const int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS],const uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS],
 uint32_t scratch_token,nflstreet_texture_flush flush,nflstreet_slot_palette palette,void *opaque){
 if(!t||!t->block||!a||!indices||!palettes||!scratch_token||!flush||!palette)return 0;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s){nflstreet_slot_texture *slot=t->slots+s;
  /* 800E2E48 keeps the template request (palette -1) when the plan is -1. */
  uint32_t index=indices[s]==-1?slot->template_index:(uint32_t)indices[s],pal=indices[s]==-1?UINT32_MAX:palettes[s];
  uint8_t *scratch=NULL;size_t size=0;
  if(!decode(a,index,&scratch,&size))return 0;
  int ok=size<=t->scratch_capacity&&size<=UINT32_MAX-scratch_token&&nflstreet_mmap_relocate(scratch,size,scratch_token)&&
   nflstreet_texture_apply(t->block+slot->offset,slot->template_size,slot->token,scratch,size,scratch_token,flush,opaque);
  free(scratch);if(!ok)return 0;
  if(pal!=UINT32_MAX&&!palette(opaque,s,pal))return 0;
  slot->applied_index=(int32_t)index;slot->applied_palette=pal;
 }
 return 1;
}
