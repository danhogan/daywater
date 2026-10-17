#include "skeleton_disc.h"
#include "disc_dol.h"
#include "disc_files.h"
#include "dol_sections.h"
#include <stdlib.h>
#include <string.h>
struct nflstreet_skeleton_disc_source {
    uint8_t base[384], hand[12], hair[114];
    uint32_t scale;
    uint8_t *archive_bytes;
    nflstreet_archive archive;
};
/* Evidence: export_skeleton_startup_catalog.py and original80041108 helpers.
   All ranges must fit one initialized section; never read a saved MEM1 image. */
static const uint8_t *span(const uint8_t *dol,const nflstreet_dol_header *h,uint32_t address,size_t size) {
    for(unsigned i=0;i<18;++i){
        const nflstreet_dol_section *s=i<7?&h->text[i]:&h->data[i-7];
        if(address>=s->address && (uint64_t)address-s->address<=s->size &&
            size<=s->size-((uint64_t)address-s->address))return dol+s->offset+(address-s->address);
    }
    return NULL;
}
void nflstreet_skeleton_disc_close(nflstreet_skeleton_disc_source *s){
    if(s){free(s->archive_bytes);free(s);}
}
nflstreet_skeleton_disc_source *nflstreet_skeleton_disc_open(const char *iso){
    uint8_t *dol=NULL;size_t dol_size=0,archive_size=0;nflstreet_dol_header h;
    nflstreet_skeleton_disc_source *s=NULL;nflstreet_disc_files *disc=NULL;
    if(!nflstreet_disc_read_dol(iso,&dol,&dol_size) || !nflstreet_dol_parse_header(dol,dol_size,&h))goto fail;
    s=calloc(1,sizeof *s);if(!s)goto fail;
    const uint8_t *base=span(dol,&h,0x80246490u,384);
    const uint8_t *hand=span(dol,&h,0x80246610u,12);
    const uint8_t *hair=span(dol,&h,0x8024661cu,114);
    const uint8_t *scale=span(dol,&h,0x80246690u,4);
    if(!base||!hand||!hair||!scale)goto fail;
    memcpy(s->base,base,384);
    memcpy(s->hand,hand,12);
    memcpy(s->hair,hair,114);
    s->scale=(uint32_t)scale[0]<<24|(uint32_t)scale[1]<<16|(uint32_t)scale[2]<<8|scale[3];
    disc=nflstreet_disc_open(iso);if(!disc)goto fail;
    if(!nflstreet_disc_read(disc,"animdata.dat",&s->archive_bytes,&archive_size)||
        !nflstreet_archive_open(s->archive_bytes,archive_size,&s->archive))goto fail;
    nflstreet_disc_close(disc);free(dol);return s;
fail:
    nflstreet_disc_close(disc);free(dol);nflstreet_skeleton_disc_close(s);return NULL;
}
int nflstreet_skeleton_disc_catalog(const nflstreet_skeleton_disc_source *s,nflstreet_skeleton_startup_catalog *out){
    if(!s||!out)return 0;
    nflstreet_skeleton_startup_catalog c={s->base,s->hand,s->hair,s->scale};*out=c;return 1;
}
int nflstreet_skeleton_disc_startup(const nflstreet_skeleton_disc_source *s,uint32_t identity,nflstreet_skeleton_startup *out){
    nflstreet_skeleton_startup_catalog c;
    if(!out)return 0;
    for(unsigned i=0;i<6;++i){
        const nflstreet_skeleton_resource *r=out->resources+i;
        if(r->bytes||r->size||r->token||r->archive_index||out->base_vectors[i])return 0;
    }
    if(!nflstreet_skeleton_disc_catalog(s,&c))return 0;
    return nflstreet_skeleton_startup_open(&s->archive,&c,identity,out);
}
