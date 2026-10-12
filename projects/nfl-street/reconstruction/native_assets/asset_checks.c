#include "archive.h"
#include "lzh1.h"
#include "texture.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void p32(uint8_t *b,uint32_t v){for(unsigned i=0;i<4;++i)b[i]=(uint8_t)(v>>(24-8*i));}
static void put(uint8_t *b,size_t *at,uint32_t value,unsigned n){
    for(unsigned i=n;i--;){if((value>>i)&1)b[*at/8]|=(uint8_t)(1u<<(7-*at%8));++*at;}
}
int main(void){
    uint8_t encoded[256]={0},out[8];size_t bits=0;
    put(encoded,&bits,0,1);
    for(unsigned i=0;i<285;++i)put(encoded,&bits,i==65||i==256?1:0,4);
    for(unsigned i=0;i<30;++i)put(encoded,&bits,i<2?1:0,4);
    put(encoded,&bits,0,1);put(encoded,&bits,1,1);put(encoded,&bits,1,1);put(encoded,&bits,0x00420042,32);
    size_t n=(bits+7)/8;
    assert(nflstreet_lzh1_decode(encoded,n,out,1)&&out[0]=='A');
    assert(!nflstreet_lzh1_decode(encoded,n,out,0));
    assert(!nflstreet_lzh1_decode(encoded,n,out,2));
    for(size_t i=0;i<n;++i)assert(!nflstreet_lzh1_decode(encoded,i,out,1));
    encoded[n-1]^=0x10;assert(!nflstreet_lzh1_decode(encoded,n,out,1));encoded[n-1]^=0x10;
    uint8_t bad[256];memcpy(bad,encoded,sizeof bad);
    /* Three one-bit symbols oversubscribe the literal tree. */
    size_t at=1+66*4;put(bad,&at,1,4);assert(!nflstreet_lzh1_decode(bad,n,out,1));
    uint8_t archive[48]={0};memcpy(archive,"TERF",4);p32(archive+4,16);archive[15]=1;
    memcpy(archive+16,"DIR1",4);p32(archive+20,16);p32(archive+24,8);p32(archive+28,8);
    memcpy(archive+32,"DATA",4);p32(archive+36,16);memcpy(archive+40,"payload!",8);
    nflstreet_archive a;nflstreet_archive_entry entry;assert(nflstreet_archive_open(archive,48,&a));
    assert(nflstreet_archive_get(&a,0,&entry)&&entry.bytes==archive+40&&entry.size==8);
    assert(!nflstreet_archive_get(&a,1,&entry));
    p32(archive+24,0);assert(!nflstreet_archive_open(archive,48,&a));p32(archive+24,8);
    p32(archive+28,9);assert(!nflstreet_archive_open(archive,48,&a));p32(archive+28,8);
    p32(archive+20,0);assert(!nflstreet_archive_open(archive,48,&a));
    uint8_t mmap[256]={0};memcpy(mmap,"MMAP",4);mmap[5]=2;mmap[15]=1;mmap[17]=1;p32(mmap+24,40);p32(mmap+28,56);
    mmap[41]=8;mmap[43]=4;mmap[45]=9;p32(mmap+48,32);p32(mmap+52,80);
    mmap[59]=2;p32(mmap+60,2);p32(mmap+64,112);mmap[112]=0xfc;mmap[113]=0;
    nflstreet_texture tex={0};assert(nflstreet_texture_open(mmap,128,&tex));
    assert(tex.width==8&&tex.height==4&&tex.bgra[0]==0xffff0000);nflstreet_texture_close(&tex);
    mmap[80]=1;assert(!nflstreet_texture_open(mmap,128,&tex));
    /* Known channel order and CMPR transparency, beyond paletted assets. */
    p32(mmap+28,0);mmap[41]=4;mmap[43]=4;mmap[45]=6;p32(mmap+48,64);
    memset(mmap+80,0,64);mmap[80]=9;mmap[81]=1;mmap[112]=2;mmap[113]=3;
    assert(nflstreet_texture_open(mmap,sizeof mmap,&tex)&&tex.bgra[0]==0x09010203);
    nflstreet_texture_close(&tex);
    mmap[41]=8;mmap[43]=8;mmap[45]=14;p32(mmap+48,32);memset(mmap+80,0,32);
    mmap[82]=255;mmap[83]=255;mmap[84]=0xc0;
    assert(nflstreet_texture_open(mmap,sizeof mmap,&tex)&&tex.bgra[0]==0x007f7f7f);
    nflstreet_texture_close(&tex);
    /* Two static names must select different image planes and palette entries. */
    memset(mmap,0,sizeof mmap);memcpy(mmap,"MMAP",4);mmap[5]=2;
    mmap[13]=2;mmap[15]=2;mmap[17]=2;
    p32(mmap+20,96);p32(mmap+24,40);p32(mmap+28,72);p32(mmap+32,120);
    for(unsigned i=0;i<2;++i){unsigned d=40+i*16,p=72+i*12;
        mmap[d+1]=8;mmap[d+3]=4;mmap[d+5]=9;p32(mmap+d+8,32);p32(mmap+d+12,160+i*32);
        mmap[p+3]=2;p32(mmap+p+4,2);p32(mmap+p+8,224+i*2);
        mmap[96+i*12+3]=1;mmap[96+i*12+5]=(uint8_t)i;mmap[96+i*12+9]=(uint8_t)i;
    }
    memcpy(mmap+120,"red",4);memcpy(mmap+136,"blue",5);
    mmap[224]=0xfc;mmap[226]=0x80;mmap[227]=0x1f;
    unsigned image,palette_index;
    assert(nflstreet_texture_find(mmap,sizeof mmap,"blue",&image,&palette_index)&&image==1&&palette_index==1);
    assert(nflstreet_texture_open_image(mmap,sizeof mmap,image,palette_index,&tex)&&tex.bgra[0]==0xff0000ff);
    nflstreet_texture_close(&tex);
    assert(!nflstreet_texture_find(mmap,sizeof mmap,"missing",&image,&palette_index));
    assert(!nflstreet_texture_open_image(mmap,sizeof mmap,2,0,&tex));
    assert(!nflstreet_texture_open_image(mmap,sizeof mmap,0,2,&tex));
    mmap[192]=1;assert(!nflstreet_texture_open_image(mmap,sizeof mmap,1,1,&tex));mmap[192]=0;
    mmap[111]=2;assert(!nflstreet_texture_find(mmap,sizeof mmap,"blue",&image,&palette_index));mmap[111]=1;
    memcpy(mmap+120,"blue",5);assert(!nflstreet_texture_find(mmap,sizeof mmap,"blue",&image,&palette_index));
    printf("asset checks passed: truncation, checksum, sizes, Huffman tree, directory/palette bounds, RGBA8, CMPR, named images and alternate palettes\n");
    return 0;
}
