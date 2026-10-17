#include "disc_slot_materials.h"
#include "disc_dol.h"
#include "dol_sections.h"
#include <stdlib.h>
#include <string.h>
/* Source-backed locations/format: export_slot_material_catalog.py (GNNE69 r0).
   The builder is private: structural tests include this translation unit, while
   both public constructors always require the exact whole-DOL revision pin. */
#define SM_BINDING UINT32_C(0x80276c6c)
#define SM_SETTINGS UINT32_C(0x8028dd90)
#define SM_DESCRIPTORS UINT32_C(0x8028dd10)
#define SM_CAPACITY (8u+25u*16u+4u+64u*40u+4u+64u*48u+4u+2u*(20u+8u*20u))
typedef struct {const uint8_t *dol;size_t size;nflstreet_dol_header header;uint8_t *bytes;size_t used;} sm_builder;
static uint32_t sm_read(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void sm_write(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static const uint8_t *sm_span(const sm_builder *b,uint64_t address,size_t size){
 if(address>UINT32_MAX||size>UINT64_C(0x100000000)-address)return NULL;
 for(unsigned i=0;i<18;++i){const nflstreet_dol_section *s=i<7?b->header.text+i:b->header.data+i-7;
  if(address>=s->address&&address-s->address<=s->size&&size<=s->size-(address-s->address)){
   size_t at=s->offset+(size_t)(address-s->address);if(at>b->size||size>b->size-at)return NULL;return b->dol+at;}}
 return NULL;
}
static int sm_append(sm_builder *b,const void *p,size_t n){
 if(!p||n>SM_CAPACITY-b->used)return 0;memcpy(b->bytes+b->used,p,n);b->used+=n;return 1;
}
static int sm_word(sm_builder *b,uint32_t n){uint8_t word[4];sm_write(word,n);return sm_append(b,word,4);}
static int sm_name(sm_builder *b,uint32_t pointer){
 uint8_t name[16]={0};
 /* Match exporter: first terminating byte must be at index 1..15. Each byte
    is resolved separately because loaded names may cross section boundaries. */
 for(unsigned i=0;i<16;++i){const uint8_t *p=sm_span(b,(uint64_t)pointer+i,1);if(!p)return 0;name[i]=*p;
  if(i&&!*p)return sm_append(b,name,sizeof name);}
 return 0;
}
static int sm_build(const uint8_t *dol,size_t size,nflstreet_disc_slot_materials *out){
 sm_builder b={0};nflstreet_disc_slot_materials result={0};b.dol=dol;b.size=size;
 if(!nflstreet_dol_parse_header(dol,size,&b.header))return 0;
 b.bytes=malloc(SM_CAPACITY);if(!b.bytes)return 0;
 if(!sm_append(&b,"SMC1",4)||!sm_word(&b,1))goto fail;
 for(unsigned i=0;i<25;++i){const uint8_t *p=sm_span(&b,SM_BINDING+32u*i,4);if(!p||!sm_name(&b,sm_read(p)))goto fail;}
 size_t count_at=b.used;if(!sm_word(&b,0))goto fail;unsigned settings=0;
 for(;;){const uint8_t *p=sm_span(&b,SM_SETTINGS+28u*settings,28);if(!p)goto fail;
  if(!sm_read(p))break;if(settings==64)goto fail;
  if(!sm_name(&b,sm_read(p))||!sm_append(&b,p+4,24))goto fail;++settings;}
 sm_write(b.bytes+count_at,settings);
 const uint8_t *p=sm_span(&b,SM_DESCRIPTORS,8);if(!p)goto fail;
 uint32_t count=sm_read(p),table=sm_read(p+4);if(count>64||!sm_word(&b,count))goto fail;
 for(uint32_t i=0;i<count;++i){p=sm_span(&b,(uint64_t)table+48u*i,48);
  if(!p||p[8]||p[9]!=1||!sm_name(&b,sm_read(p))||!sm_name(&b,sm_read(p+4))||!sm_name(&b,sm_read(p+12)))goto fail;}
 static const uint32_t shaders[2]={0x80293acc,0x80293b40};static const char *names[2]={"OnePass","TwoPass"};
 if(!sm_word(&b,2))goto fail;
 for(unsigned k=0;k<2;++k){size_t at=b.used;if(!sm_name(&b,shaders[k])||strcmp((const char*)b.bytes+at,names[k]))goto fail;
  p=sm_span(&b,shaders[k]+0x18u,4);if(!p)goto fail;uint32_t stages=sm_read(p);
  if(!stages||stages>8||!sm_word(&b,stages))goto fail;
  p=sm_span(&b,shaders[k]+0x1cu,20u*stages);if(!sm_append(&b,p,20u*stages))goto fail;}
 result.bytes=b.bytes;result.size=b.used;
 if(!nflstreet_slot_material_catalog_open(result.bytes,result.size,&result.catalog))goto fail;
 *out=result;return 1;
fail:free(b.bytes);return 0;
}
static int sm_empty(const nflstreet_disc_slot_materials *out){
 if(!out||out->bytes||out->size||out->catalog.names||out->catalog.settings||out->catalog.descriptors||
  out->catalog.setting_count||out->catalog.descriptor_count)return 0;
 for(unsigned i=0;i<2;++i)if(out->catalog.shaders[i]||out->catalog.shader_stages[i])return 0;return 1;
}
int nflstreet_disc_slot_materials_from_dol(const uint8_t *dol,size_t size,nflstreet_disc_slot_materials *out){
 if(!sm_empty(out)||!nflstreet_dol_verify_revision(dol,size))return 0;return sm_build(dol,size,out);
}
int nflstreet_disc_slot_materials_from_disc(const char *iso,nflstreet_disc_slot_materials *out){
 uint8_t *dol=NULL;size_t size=0;if(!sm_empty(out)||!nflstreet_disc_read_dol(iso,&dol,&size))return 0;
 int ok=nflstreet_disc_slot_materials_from_dol(dol,size,out);free(dol);return ok;
}
void nflstreet_disc_slot_materials_destroy(nflstreet_disc_slot_materials *out){if(out){free(out->bytes);memset(out,0,sizeof *out);}}
