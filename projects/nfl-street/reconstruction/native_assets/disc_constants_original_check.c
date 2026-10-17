/* Host-loader byte contract, independent exported DOL reference; no oracle calls. */
#include "dol_sections.h"
#include "disc_dol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
static unsigned seed = 0x46e3de85u;
static unsigned rnd(void) { seed = seed * 1664525u + 1013904223u; return seed; }
static uint32_t rd(const uint8_t *p) { return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static void wr(uint8_t *p, uint32_t x) { p[0]=x>>24; p[1]=x>>16; p[2]=x>>8; p[3]=x; }
static uint8_t *read_file(const char *p, size_t *n) {
    FILE *f=fopen(p,"rb"); if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;} long z=ftell(f);
    if(z<=0 || fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t *b=malloc((size_t)z); if(!b || fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}
    fclose(f); *n=(size_t)z; return b;
}
static int rejected(const uint8_t *bad,size_t n,uint8_t *ram,const uint8_t *saved) {
    nflstreet_dol_header h;
    memcpy(ram,saved,BYTES);
    if(nflstreet_dol_parse_header(bad,n,&h))return 0;
    return !memcmp(ram,saved,BYTES);
}
#define REQUIRE(x) do { if(!(x)){fprintf(stderr,"comparison failed at line %u\n",__LINE__);return 4;} } while(0)
int main(int argc,char **argv) {
    if(argc!=5)return 2;
    size_t n=0,m=0,s=0; uint8_t *dol=NULL;
    uint8_t *ref=read_file(argv[2],&m),*saved=read_file(argv[3],&s);
    if(!ref || !saved || s!=BYTES || !nflstreet_disc_read_dol(argv[1],&dol,&n))return 3;
    REQUIRE(n==m && !memcmp(dol,ref,n));
    REQUIRE(nflstreet_dol_verify_revision(ref,m));
    uint8_t *ram=malloc(BYTES),*expected=malloc(BYTES),*bad=malloc(n);
    if(!ram || !expected || !bad)return 3;
    unsigned edited=(unsigned)strtoul(argv[4],NULL,10),invalid=0;
    uint32_t hash=2166136261u;
    for(unsigned c=0;c<=edited;++c){
        memcpy(ram,saved,BYTES);
        if(c)for(size_t i=0;i<BYTES;++i)ram[i]=(uint8_t)(rnd()>>24);
        memcpy(expected,ram,BYTES);
        /* Reference map uses raw header offsets, independently from parsed production descriptors. */
        uint32_t bss=rd(ref+0xd8), bsize=rd(ref+0xdc);
        REQUIRE(bss>=BASE && (uint64_t)bss-BASE+bsize<=BYTES);
        memset(expected+bss-BASE,0,bsize);
        for(unsigned i=0;i<18;++i){
            uint32_t off=rd(ref+4*i),addr=rd(ref+0x48+4*i),len=rd(ref+0x90+4*i);
            if(!len)continue;
            REQUIRE(off<=m && len<=m-off && addr>=BASE && (uint64_t)addr-BASE+len<=BYTES);
            memcpy(expected+addr-BASE,ref+off,len);
        }
        nflstreet_dol_header h;
        REQUIRE(nflstreet_dol_parse_header(dol,n,&h));
        REQUIRE(h.entry_point==rd(ref+0xe0));
        REQUIRE(nflstreet_dol_load_sections(dol,n,&h,ram,BYTES,BASE));
        REQUIRE(!memcmp(ram,expected,BYTES));
        for(size_t i=0;i<BYTES;++i)hash=(hash^ram[i])*16777619u;
    }
    memcpy(bad,dol,n);bad[n-1]^=1;REQUIRE(!nflstreet_dol_verify_revision(bad,n));++invalid;
    REQUIRE(!nflstreet_dol_verify_revision(dol,n-1));++invalid;
    REQUIRE(rejected(dol,255,ram,saved));++invalid;
    REQUIRE(rejected(dol,n-1,ram,saved));++invalid;
    for(unsigned k=0;k<7;++k){
        memcpy(bad,dol,n);
        if(k==0)wr(bad,255); /* source before header */
        if(k==1)wr(bad,(uint32_t)n); /* source past end */
        if(k==2)wr(bad+0x4c,rd(bad+0x48)); /* guest section overlap */
        if(k==3)wr(bad+4,rd(bad)); /* file section overlap */
        if(k==4)wr(bad+0x48,0xffffff00u); /* guest wrap */
        if(k==5){wr(bad+0xd8,0xfffffff0u);wr(bad+0xdc,32);} /* BSS wrap */
        if(k==6)wr(bad+0x90,0xffffffffu); /* oversized source */
        REQUIRE(rejected(bad,n,ram,saved));++invalid;
    }
    nflstreet_dol_header h;
    REQUIRE(nflstreet_dol_parse_header(dol,n,&h));
    for(unsigned k=0;k<5;++k){
        nflstreet_dol_header forged=h;
        if(k==0)forged.text[0].offset=(uint32_t)n;
        if(k==1)forged.text[0].address=BASE-1;
        if(k==2)forged.data[4].address=BASE+BYTES-1;
        if(k==3)forged.bss_address=BASE+BYTES-1;
        if(k==4)forged.text[1].address=forged.text[0].address;
        memcpy(ram,saved,BYTES);
        REQUIRE(!nflstreet_dol_load_sections(dol,n,&forged,ram,BYTES,BASE));
        REQUIRE(!memcmp(ram,saved,BYTES));++invalid;
    }
    memcpy(ram,saved,BYTES);
    REQUIRE(!nflstreet_dol_load_sections(dol,n-1,&h,ram,BYTES,BASE));
    REQUIRE(!memcmp(ram,saved,BYTES));++invalid;
    printf("DOL constants: %u cases; all 25165824 bytes match; %u invalid inputs rejected; digest %08x.\n",edited+1,invalid,hash);
    free(ref);free(saved);free(dol);free(ram);free(expected);free(bad);return 0;
}
