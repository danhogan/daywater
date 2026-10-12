#include "face_resource_pair.h"
#include "model_relocate.h"
#include <stdlib.h>
#include <string.h>

void nflstreet_face_resource_pair_close(nflstreet_face_resource_pair *pair)
{
    if(!pair)return;
    free(pair->bytes[0]);free(pair->bytes[1]);memset(pair,0,sizeof *pair);
}

int nflstreet_face_resource_pair_open(const nflstreet_archive *archive,unsigned index,
    uint32_t identity_base,nflstreet_face_resource_pair *out)
{
    if(!archive||!out||!identity_base||index>=archive->count||archive->count-index<2)return 0;
    nflstreet_face_resource_pair pair={0};uint64_t cursor=identity_base;
    size_t count=0;const nflstreet_model_chunk_registration *registry=nflstreet_model_default_registry(&count);
    for(unsigned i=0;i<2;++i){
        nflstreet_archive_entry entry;
        pair.indices[i]=index+i;
        if(!nflstreet_archive_get(archive,index+i,&entry)||
           !nflstreet_archive_decode(&entry,pair.bytes+i,pair.sizes+i))goto fail;
        if(cursor>UINT32_MAX||pair.sizes[i]>UINT32_MAX-cursor)goto fail;
        pair.tokens[i]=(uint32_t)cursor;
        if(!nflstreet_model_relocate(pair.bytes[i],pair.sizes[i],pair.tokens[i],registry,count))goto fail;
        nflstreet_model_view_source source={pair.bytes[i],pair.sizes[i],pair.tokens[i]};
        if(!nflstreet_model_view_attach(pair.views[i],sizeof pair.views[i],&source))goto fail;
        cursor+=((uint64_t)pair.sizes[i]+31u)&~(uint64_t)31u;
    }
    *out=pair;return 1;
fail:
    nflstreet_face_resource_pair_close(&pair);return 0;
}
