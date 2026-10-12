#include "skeleton_resource.h"
#include <stdlib.h>
#include <string.h>
static uint32_t word(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
typedef struct {uint8_t *bytes,*visited;size_t size,*order,count,capacity;} traversal;
static int walk(traversal *t,size_t at,unsigned depth,unsigned *result){
    if(depth>256)return 0;
    for(;;){
        if(at>t->size||t->size-at<12||t->visited[at]||t->count==t->capacity)return 0;
        unsigned arity=t->bytes[at];if((size_t)arity*4>t->size-at-8)return 0;
        uint32_t parent=word(t->bytes+at+4);if(parent>=t->size)return 0;
        t->visited[at]=1;t->order[t->count++]=at;
        if(arity<2)*result=t->bytes[at+1];
        else for(unsigned i=0;i<arity;++i)
            if(!walk(t,word(t->bytes+at+8+4*i),depth+1,result))return 0;
        size_t next=word(t->bytes+at+8);
        if(next>=t->size||t->size-next<2)return 0;
        if(*result>=t->bytes[next+1])return 1;
        at=next;
    }
}
int nflstreet_skeleton_relocate(uint8_t *bytes,size_t size,uint32_t token){
    if(!bytes||size<16||!token||size>UINT32_MAX-token||memcmp(bytes,"SKL1",4))return 0;
    if(bytes[4]&0x80)return 1;
    unsigned count=(unsigned)bytes[6]<<8|bytes[7];
    size_t root=word(bytes+8),names=word(bytes+12);
    if(root>=size||names>size||(size_t)count*20>size-names)return 0;
    for(unsigned i=0;i<count;++i)if(word(bytes+names+20*i)>=size)return 0;
    traversal t={bytes,calloc(size,1),size,NULL,0,size/4};
    t.order=malloc(t.capacity*sizeof *t.order);unsigned result=0;
    if(!t.visited||!t.order||!walk(&t,root,0,&result)){free(t.visited);free(t.order);return 0;}
    put(bytes+8,token+(uint32_t)root);put(bytes+12,token+(uint32_t)names);
    for(size_t i=0;i<t.count;++i){size_t at=t.order[i];unsigned arity=bytes[at];
        put(bytes+at+4,word(bytes+at+4)+token);
        for(unsigned j=0;j<arity;++j)put(bytes+at+8+4*j,word(bytes+at+8+4*j)+token);
    }
    put(bytes+root+4,token+(uint32_t)root);
    for(unsigned i=0;i<count;++i)put(bytes+names+20*i,word(bytes+names+20*i)+token);
    bytes[4]|=0x80;free(t.visited);free(t.order);return 1;
}
int nflstreet_skeleton_resource_open(const nflstreet_archive *archive,unsigned index,
    uint32_t token,nflstreet_skeleton_resource *out){
    if(!archive||!out)return 0;nflstreet_archive_entry entry;nflstreet_skeleton_resource r={0};
    r.token=token;r.archive_index=index;
    if(!nflstreet_archive_get(archive,index,&entry)||!nflstreet_archive_decode(&entry,&r.bytes,&r.size))return 0;
    if(!nflstreet_skeleton_relocate(r.bytes,r.size,token)){free(r.bytes);return 0;}
    *out=r;return 1;
}
void nflstreet_skeleton_resource_close(nflstreet_skeleton_resource *r){
    if(r){free(r->bytes);memset(r,0,sizeof *r);}
}
