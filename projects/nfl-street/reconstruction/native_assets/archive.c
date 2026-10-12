#include "archive.h"
#include "lzh1.h"
#include <stdlib.h>
#include <string.h>
static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
int nflstreet_archive_get(const nflstreet_archive *a, unsigned index, nflstreet_archive_entry *e) {
    if (!a || !e || index >= a->count) return 0;
    const uint8_t *row = a->bytes + a->directory + index*8;
    size_t offset = be32(row), size = be32(row+4);
    /* Offsets include the DATA chunk header, not just its payload. */
    if (offset < 8 || offset > a->data_size || size > a->data_size-offset) return 0;
    nflstreet_archive_entry v = {a->bytes+a->data+offset, size, 0, (uint32_t)size};
    if (a->compression) {
        row = a->bytes+a->compression+index*8;
        v.codec=be32(row); v.decoded_size=be32(row+4);
        if (!v.codec && v.decoded_size != size) return 0;
    }
    *e=v; return 1;
}
int nflstreet_archive_open(const uint8_t *b, size_t n, nflstreet_archive *out) {
    if (!b || !out || n<16 || memcmp(b,"TERF",4)) return 0;
    size_t p=be32(b+4);
    if (p<16 || p>n) return 0;
    nflstreet_archive a={0}; a.bytes=b; a.size=n; a.count=(uint16_t)((unsigned)b[14]<<8|b[15]);
    while (p<n) {
        if (n-p<8) return 0;
        size_t length=be32(b+p+4);
        if (length<8 || length>n-p) return 0;
        if (!memcmp(b+p,"DIR1",4)) {
            if (a.directory || (size_t)a.count*8>length-8) return 0;
            a.directory=p+8;
        } else if (!memcmp(b+p,"COMP",4)) {
            if (a.compression || (size_t)a.count*8>length-8) return 0;
            a.compression=p+8;
        } else if (!memcmp(b+p,"DATA",4)) {
            if (a.data) return 0;
            a.data=p; a.data_size=length;
        }
        p+=length;
    }
    if (!a.directory || !a.data) return 0;
    for (unsigned i=0;i<a.count;++i) {
        nflstreet_archive_entry e;
        if (!nflstreet_archive_get(&a,i,&e)) return 0;
    }
    *out=a; return 1;
}
int nflstreet_archive_plain(const nflstreet_archive_entry *e, const uint8_t **b, size_t *n) {
    if (!e || !b || !n || e->codec || e->decoded_size!=e->size) return 0;
    *b=e->bytes; *n=e->size; return 1;
}
int nflstreet_archive_decode(const nflstreet_archive_entry *e,uint8_t **bytes,size_t *size){
    if(!e||!bytes||!size||(e->codec!=0&&e->codec!=5)||e->decoded_size>128*1024*1024)return 0;
    if(!e->codec&&e->decoded_size!=e->size)return 0;
    uint8_t *b=malloc(e->decoded_size?e->decoded_size:1);if(!b)return 0;
    if(e->codec==5){if(!nflstreet_lzh1_decode(e->bytes,e->size,b,e->decoded_size)){free(b);return 0;}}
    else memcpy(b,e->bytes,e->size);
    *bytes=b;*size=e->decoded_size;return 1;
}
int nflstreet_archive_find(const nflstreet_archive *a,const char *name,unsigned *out){
    if(!a||!a->bytes||!name||!out||a->size<16||!*name)return 0;
    size_t p=be32(a->bytes+4);unsigned found=0,selected=0;
    if(p<16||p>a->size)return 0;
    while(p<a->size){if(a->size-p<8)return 0;size_t n=be32(a->bytes+p+4);
        if(n<8||n>a->size-p)return 0;
        if(!memcmp(a->bytes+p,"HSH1",4)){
            if(n<24||be32(a->bytes+p+8)!=1||be32(a->bytes+p+12)!=4||be32(a->bytes+p+16)!=4||(size_t)a->count*4>n-24)return 0;
            uint32_t seed=be32(a->bytes+p+20),hash=0;size_t length=0;
            for(const unsigned char *s=(const unsigned char *)name;*s;++s){
                if(*s>127||++length>1024)return 0;
                unsigned c=*s;if(c>='a'&&c<='z')c-=32;hash=hash*seed+c;
            }
            for(unsigned i=0;i<a->count;++i)if(be32(a->bytes+p+24+i*4)==hash){++found;selected=i;}
        }
        p+=n;
    }
    if(found!=1)return 0;
    *out=selected;return 1;
}
