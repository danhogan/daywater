#include "hair_resources.h"
#include "model_relocate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int nflstreet_hair_plan_make(const nflstreet_archive *roster,const char *name,
    uint16_t plht,uint16_t phar,uint16_t state,nflstreet_hair_plan *out){
    if(!roster||!out)return 0;nflstreet_hair_plan p={{0}};
    /* Original800DEBD0 special states bypass named lookup. */
    if(state==11||state==12){uint32_t first=(state==11?0x3b9u:0x3cau)+(uint32_t)plht*2;
        p.indices[0]=first;p.indices[1]=first+1;p.indices[2]=state==11?0x3c9u:0x3dau;
    }else{
        uint32_t base=(uint32_t)phar*34u;
        p.indices[0]=base+(uint32_t)plht*4u+(state?3u:1u);
        p.indices[1]=p.indices[0]+1;p.indices[2]=base+(state?34u:33u);
        if(name){char trimmed[64],key[96];size_t n=0;
            while(name[n]&&name[n]!=' '){if(n>=sizeof trimmed-1)return 0;trimmed[n]=name[n];++n;}
            trimmed[n]=0;
            for(unsigned i=0;i<2;++i){int length=snprintf(key,sizeof key,"HAIR_MODEL_%s%s_%s",trimmed,state?"HAT":"",i?"1500":"2700");
                unsigned entry;if(length<0||length>=(int)sizeof key)return 0;
                if(nflstreet_archive_find(roster,key,&entry))p.indices[i]=entry+50000u;
            }
        }
    }
    *out=p;return 1;
}
void nflstreet_hair_resources_close(nflstreet_hair_resources *r){
    if(r){for(unsigned i=0;i<3;++i)free(r->bytes[i]);memset(r,0,sizeof *r);}
}
int nflstreet_hair_resources_open(const nflstreet_archive *local,const nflstreet_archive *roster,
    const nflstreet_hair_plan *plan,uint32_t identity,nflstreet_hair_resources *out){
    if(!local||!roster||!plan||!identity||!out)return 0;
    nflstreet_hair_resources r={0};uint64_t cursor=identity;size_t count=0;
    const nflstreet_model_chunk_registration *registry=nflstreet_model_default_registry(&count);
    for(unsigned i=0;i<3;++i){uint32_t index=plan->indices[i];r.indices[i]=index;
        if(!index)continue;if(index==0xffffu)goto fail;
        r.roster_archive[i]=(uint8_t)(index>50000u);index=index>50000u?index-50000u:index;
        r.archive_indices[i]=index;const nflstreet_archive *a=r.roster_archive[i]?roster:local;nflstreet_archive_entry entry;
        if(!nflstreet_archive_get(a,index,&entry)||!nflstreet_archive_decode(&entry,r.bytes+i,r.sizes+i)||
           cursor>UINT32_MAX||r.sizes[i]>UINT32_MAX-cursor)goto fail;
        r.tokens[i]=(uint32_t)cursor;
        if(!nflstreet_model_relocate(r.bytes[i],r.sizes[i],r.tokens[i],registry,count))goto fail;
        nflstreet_model_view_source source={r.bytes[i],r.sizes[i],r.tokens[i]};
        if(!nflstreet_model_view_attach(r.views[i],52,&source))goto fail;
        cursor+=((uint64_t)r.sizes[i]+31u)&~(uint64_t)31u;
    }*out=r;return 1;
fail:nflstreet_hair_resources_close(&r);return 0;
}
