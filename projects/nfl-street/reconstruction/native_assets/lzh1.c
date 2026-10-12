#include "lzh1.h"
#include <string.h>
typedef struct {const uint8_t *bytes;size_t size,bit;int error;} bits;
typedef struct {unsigned count[16],first[16],offset[16],symbols[285];} tree;
static uint32_t take(bits *b,unsigned n){
    uint32_t v=0;
    for(unsigned i=0;i<n;++i){if(b->bit/8>=b->size){b->error=1;return 0;}
        v=v<<1|((b->bytes[b->bit/8]>>(7-b->bit%8))&1);++b->bit;}
    return v;
}
static int read_tree(bits *b,tree *t,unsigned n){
    unsigned lengths[285];memset(t,0,sizeof *t);
    for(unsigned i=0;i<n;++i){lengths[i]=take(b,4);if(lengths[i])++t->count[lengths[i]];}
    unsigned offset=0;
    for(unsigned i=1;i<16;++i){
        t->first[i]=(t->first[i-1]+t->count[i-1])*2;
        if(t->first[i]+t->count[i]>(1u<<i))return 0;
        t->offset[i]=offset;offset+=t->count[i];
    }
    if(b->error||!offset)return 0;
    unsigned next[16];memcpy(next,t->offset,sizeof next);
    for(unsigned i=0;i<n;++i)if(lengths[i])t->symbols[next[lengths[i]]++]=i;
    return 1;
}
static unsigned symbol(bits *b,const tree *t){
    unsigned code=0;
    for(unsigned i=1;i<16;++i){code=code*2+take(b,1);
        if(code>=t->first[i]&&code-t->first[i]<t->count[i])return t->symbols[t->offset[i]+code-t->first[i]];
    }
    b->error=1;return 0;
}
int nflstreet_lzh1_decode(const uint8_t *input,size_t size,uint8_t *output,size_t decoded){
    static const unsigned lengths[28]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227};
    static const unsigned lextra[28]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,0};
    static const unsigned distance[30]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    if(!input||(!output&&decoded)||size>SIZE_MAX/8)return 0;
    bits b={input,size,0,0};size_t at=0;uint32_t sum1=1,sum2=0;
    while(!take(&b,1)){
        tree lit,dist;if(!read_tree(&b,&lit,285)||!read_tree(&b,&dist,30))return 0;
        for(;;){unsigned s=symbol(&b,&lit);if(b.error)return 0;
            if(s==256)break;
            if(s<256){if(at>=decoded)return 0;output[at++]=(uint8_t)s;}
            else {unsigned j=s-257;if(j>=28)return 0;
                size_t n=lengths[j]+take(&b,lextra[j]);unsigned d=symbol(&b,&dist);if(d>=30)return 0;
                unsigned extra=d<4?0:(d/2-1);size_t back=distance[d]+take(&b,extra);
                if(b.error||back>at||!back||n>decoded-at)return 0;
                for(size_t i=0;i<n;++i){output[at]=output[at-back];++at;}
            }
        }
        if(b.error)return 0;
    }
    uint32_t expected=take(&b,32);if(b.error||at!=decoded)return 0;
    for(size_t i=0;i<decoded;++i){sum1=(sum1+output[i])%65521;sum2=(sum2+sum1)%65521;}
    return expected==(sum2<<16|sum1);
}
