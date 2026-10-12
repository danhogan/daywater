#include "model_relocate.h"
#include "model_chunk_relocate.h"
#include <string.h>
static uint32_t read(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void write(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
const nflstreet_model_chunk_registration *nflstreet_model_default_registry(size_t *count)
{
    static const nflstreet_model_chunk_registration registry[]={
        {0x496e666fu,1,NULL},{0x4c697465u,0,nflstreet_model_chunk_relocate},
        {0x4d61746cu,1,nflstreet_model_chunk_relocate},{0x54657874u,0,NULL},
        {0x536b656cu,1,nflstreet_model_chunk_relocate},{0x47656f6du,2,nflstreet_model_chunk_relocate},
        {0x4578746eu,0,nflstreet_model_chunk_relocate},{0x53776170u,0,nflstreet_model_chunk_relocate},
        {0x436c7374u,0,nflstreet_model_chunk_relocate},{0x5478666du,1,nflstreet_model_chunk_relocate},
        {0x496e7374u,1,nflstreet_model_chunk_relocate},{0x52696e74u,1,nflstreet_model_chunk_relocate},
        {0x46616465u,0,NULL},{0x44496e66u,1,nflstreet_model_chunk_relocate},
        {0x4d416e6du,1,nflstreet_model_chunk_relocate}
    };
    if(count)*count=sizeof registry/sizeof registry[0];
    return registry;
}
int nflstreet_model_relocate(uint8_t *bytes,size_t size,uint32_t identity,
    const nflstreet_model_chunk_registration *registry,size_t count)
{
    if(!bytes||size<16||!identity||size>UINT32_MAX-identity||
        memcmp(bytes,"TMdl",4)||(!registry&&count))return 0;
    if(bytes[15]==1)return 1;
    unsigned rows=(unsigned)bytes[8]<<8|bytes[9];
    size_t directory=(size_t)bytes[10]<<8|bytes[11];
    if((rows&&!directory)||directory>size||(size_t)rows*16>size-directory)return 0;
    for(unsigned i=0;i<rows;++i){
        uint8_t *row=bytes+directory+i*16;
        if(row[15])continue;
        const nflstreet_model_chunk_registration *found=NULL;
        for(size_t j=0;j<count;++j)if(registry[j].tag==read(row)){found=registry+j;break;}
        if(!found)write(row+4,0);
        else if(read(row+8)){
            if(found->version!=row[12])write(row+4,0);
            else {
                size_t offset=read(row+4),length=read(row+8);
                if(offset>size||length>size-offset)return 0;
                write(row+4,offset?identity+(uint32_t)offset:0);
                if(found->handler&&(!offset||!found->handler(bytes,size,identity,found->tag,offset,length)))return 0;
            }
        }
        row[15]=1;
    }
    bytes[15]=1;
    return 1;
}
