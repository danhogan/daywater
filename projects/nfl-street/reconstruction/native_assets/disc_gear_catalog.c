#include "disc_gear_catalog.h"
#include "disc_dol.h"
#include "dol_sections.h"
#include <stdlib.h>
#include <string.h>
/* Layout comes solely from export_gear_texture_catalog.py, pinned GNNE69 r0.
   Only addresses/lengths and serialization metadata are compiled in. */
static void put(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static int empty(const nflstreet_disc_gear_catalog *o){
 return o&&!o->bytes&&!o->size&&!o->catalog.window&&!o->catalog.categories&&!o->catalog.tags&&
 !o->catalog.excluded_dynamic_name&&!o->catalog.templates&&!o->catalog.color_names&&!o->catalog.level_names&&!o->catalog.color_tags;
}
static int copy_span(uint8_t *to,const uint8_t *dol,const nflstreet_dol_header *h,uint32_t address,uint32_t size){
 for(unsigned i=0;i<18;++i){const nflstreet_dol_section *s=i<7?&h->text[i]:&h->data[i-7];
  if(address>=s->address&&(uint64_t)address+size<=(uint64_t)s->address+s->size){
   memcpy(to,dol+s->offset+(address-s->address),size);return 1;}}
 return 0;
}
void nflstreet_disc_gear_catalog_close(nflstreet_disc_gear_catalog *o){
 if(o){free(o->bytes);memset(o,0,sizeof *o);}
}
int nflstreet_disc_gear_catalog_open_dol(const uint8_t *dol,size_t size,nflstreet_disc_gear_catalog *out){
 nflstreet_dol_header h;
 if(!empty(out)||!nflstreet_dol_verify_revision(dol,size)||!nflstreet_dol_parse_header(dol,size,&h))return 0;
 nflstreet_disc_gear_catalog o={0};o.bytes=malloc(NFLSTREET_DISC_GEAR_CATALOG_BYTES);if(!o.bytes)return 0;
 o.size=NFLSTREET_DISC_GEAR_CATALOG_BYTES;uint8_t *p=o.bytes;
 memcpy(p,"GTC4",4);put(p+4,4);put(p+8,0x802714f0u);put(p+12,0x868u);p+=16;
 #define COPY(address,length) do {if(!copy_span(p,dol,&h,address,length))goto fail;p+=length;} while(0)
 COPY(0x802714f0u,0x868u);
 COPY(0x802859f8u,48u);
 COPY(0x80285968u,104u);
 COPY(0x80252cf8u,32u);
 for(unsigned slot=0;slot<25;++slot){COPY(0x802799ecu+24u*slot,4u);}
 COPY(0x8027132cu,360u);
 COPY(0x80271494u,60u);
 for(unsigned item=0;item<13;++item){COPY(0x8028575cu+16u*item+8u,4u);}
 #undef COPY
 if((size_t)(p-o.bytes)!=o.size||!nflstreet_gear_texture_catalog_open(o.bytes,o.size,&o.catalog))goto fail;
 *out=o;return 1;
fail:nflstreet_disc_gear_catalog_close(&o);return 0;
}
int nflstreet_disc_gear_catalog_open_disc(const char *path,nflstreet_disc_gear_catalog *out){
 if(!path||!empty(out))return 0;uint8_t *dol=NULL;size_t size=0;
 if(!nflstreet_disc_read_dol(path,&dol,&size))return 0;
 int ok=nflstreet_disc_gear_catalog_open_dol(dol,size,out);free(dol);return ok;
}
