#include "model_postattach.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static uint16_t half(const uint8_t *p) { return (uint16_t)((unsigned)p[0]<<8|p[1]); }
static uint32_t word(const uint8_t *p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static void put_half(uint8_t *p,uint16_t v) { p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v; }
static void put_word(uint8_t *p,uint32_t v) { p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v; }
static uint8_t *resolve(const nflstreet_model_postattach_segment *s,size_t n,
                        uint32_t token,size_t length) {
    for(size_t i=0;i<n;++i) {
        if(!s[i].bytes||token<s[i].token)continue;
        size_t off=(size_t)((uint64_t)token-s[i].token);
        if(off<=s[i].size&&length<=s[i].size-off)return s[i].bytes+off;
    }
    return NULL;
}
static int add(uint32_t base,size_t offset,uint32_t *out) {
    if(offset>UINT32_MAX-base)return 0;*out=base+(uint32_t)offset;return 1;
}
static const char *string_at(const nflstreet_model_postattach_segment *s,size_t n,
                             uint32_t token) {
    for(size_t i=0;i<n;++i) {
        if(!s[i].bytes||token<s[i].token)continue;
        size_t off=(size_t)((uint64_t)token-s[i].token);
        if(off>=s[i].size)continue;
        const char *p=(const char *)s[i].bytes+off;
        if(memchr(p,0,s[i].size-off))return p;
    }
    return NULL;
}
static int allocation(nflstreet_model_postattach_allocate fn,void *opaque,
                      uint32_t count,uint32_t width,
                      nflstreet_model_postattach_segment *out) {
    if(count&&width>SIZE_MAX/count)return 0;
    if(!fn(opaque,count,width,out)||!out->bytes||!out->token||
       out->size<(size_t)count*width)return 0;
    return 1;
}
int nflstreet_model_postattach_bind(uint32_t view_token,uint32_t target_token,
    const nflstreet_model_postattach_segment *segments,size_t segment_count,
    nflstreet_model_postattach_allocate allocate,
    nflstreet_model_postattach_release release,void *opaque) {
    if(!segments||!allocate||!release)return 0;
    uint8_t *view=resolve(segments,segment_count,view_token,52);
    if(!view)return 0;
    uint32_t skel_token=word(view+0x10),clst_token=word(view+0x0c);
    if(!skel_token||!clst_token)return 1;
    uint8_t *skel=resolve(segments,segment_count,skel_token,16);
    uint8_t *clst=resolve(segments,segment_count,clst_token,8);
    if(!skel||!clst)return 0;
    uint32_t count=word(skel),names_token=word(skel+4),rows16_token=word(skel+8),
             rows6_token=word(skel+12),clst_token_data=word(clst);
    if(!names_token)return 1;
    uint8_t *target=resolve(segments,segment_count,target_token,16);
    if(!target)return 0;
    uint32_t clst_count=word(clst+4),target_table=word(target+12);
    uint16_t target_count=half(target+6);
    if(count>UINT16_MAX)return 0;
    size_t n=count;
    uint8_t *names=resolve(segments,segment_count,names_token,n*16),
            *rows16=resolve(segments,segment_count,rows16_token,n*16),
            *rows6=resolve(segments,segment_count,rows6_token,n*6),
            *clusters=resolve(segments,segment_count,clst_token_data,(size_t)clst_count*4),
            *table=resolve(segments,segment_count,target_table,(size_t)target_count*20);
    if(!names||!rows16||!rows6||!clusters||!table)return 0;
    (void)table;
    /* Resolve complete strings before allocating or touching source storage. */
    int16_t *lookup=malloc((n?n:1)*sizeof *lookup);
    if(!lookup)return 0;
    for(size_t i=0;i<n;++i) {
        uint32_t token;if(!add(names_token,i*16,&token))goto invalid;
        const char *name=string_at(segments,segment_count,token);
        if(!name)goto invalid;
        lookup[i]=-1;
        for(unsigned j=0;j<target_count;++j) {
            uint32_t t;if(!add(target_table,(size_t)j*20+4,&t))goto invalid;
            const char *candidate=string_at(segments,segment_count,t);
            if(!candidate)goto invalid;
            if(!strcmp(name,candidate)){lookup[i]=(int16_t)j;break;}
        }
    }
    for(uint32_t i=0;i<clst_count;++i)if(clusters[(size_t)i*4]>=count)goto invalid;
    for(size_t i=0;i<n;++i)if(lookup[i]>=0) {
        if((unsigned)lookup[i]>=count)goto invalid;
        int16_t parent=(int16_t)half(rows16+i*16+12);
        if(parent!=-1&&((unsigned)parent>=count))goto invalid;
    }
    nflstreet_model_postattach_segment map={0},copy6={0},copy16={0};
    if(!allocation(allocate,opaque,count,2,&map)) {
        put_word(skel+4,0);free(lookup);return 1;
    }
    for(size_t i=0;i<n;++i)put_half(map.bytes+i*2,(uint16_t)lookup[i]);
    /* 801CA368: each cluster starts with an old skeleton index. Search
       backward to the first named predecessor; unmatched zero becomesFF. */
    for(uint32_t i=0;i<clst_count;++i) {
        uint8_t *p=clusters+(size_t)i*4;int old=p[0];
        if(old>0&&lookup[old]==-1)
            do { --old; } while(old>0&&lookup[old]==-1);
        p[0]=old>=0&&lookup[old]!=-1?(uint8_t)lookup[old]:255;
    }
    if(!allocation(allocate,opaque,count,6,&copy6))goto fail_after_map;
    memcpy(copy6.bytes,rows6,n*6);
    if(!allocation(allocate,opaque,count,16,&copy16))goto fail_after_copy6;
    memcpy(copy16.bytes,rows16,n*16);
    for(size_t i=0;i<n;++i)if(lookup[i]!=-1) {
        unsigned dst=(unsigned)lookup[i];
        if(dst>=count)goto fail_after_copy16;
        memcpy(rows6+(size_t)dst*6,copy6.bytes+i*6,6);
        memcpy(rows16+(size_t)dst*16,copy16.bytes+i*16,16);
        int16_t parent=(int16_t)half(copy16.bytes+i*16+12);
        put_half(rows16+(size_t)dst*16+12,parent==-1?0xffffu:
                 (uint16_t)lookup[(unsigned)parent]);
    }
    release(opaque,copy6.token);release(opaque,copy16.token);
    release(opaque,map.token);put_word(skel+4,0);free(lookup);return 1;
fail_after_copy16:release(opaque,copy16.token);
fail_after_copy6:release(opaque,copy6.token);
fail_after_map:release(opaque,map.token);
invalid:free(lookup);return 0;
}
