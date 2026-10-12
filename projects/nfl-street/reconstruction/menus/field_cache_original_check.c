#include <windows.h>
#include "moderngekko/module_abi.h"
#include "field_cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u * 1024u * 1024u)
#define RECORDS 0x817e0000u
#define DEST0 0x817e1000u
#define DEST1 0x817e1100u
#define DEST2 0x817e1200u
#define DEST3 0x817e1300u
#define VALUE 0x817e1400u
#define R13 0x817f0000u
#define STACK 0x817df000u

static void put32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v;
}
static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
}
static uint8_t *at(uint8_t *ram, uint32_t a) {
    if (a < 0x80000000u || (size_t)(a-0x80000000u) >= RAM_SIZE) return NULL;
    return ram + a - 0x80000000u;
}
static int run(const ModernGekkoModuleDesc *m, CPUState *c) {
    c->pc=0x8011f1f0u; c->lr=0; c->exception=0;
    c->downcount=c->cycle_budget=100000000;
    for (unsigned i=0; c->pc && i<10000; ++i) {
        m->dispatch(c,c->pc);
        if (c->exception) return 0;
    }
    return c->pc==0 && !c->exception;
}
static int set_string(uint8_t *r, size_t off, size_t n, unsigned pattern) {
    if (pattern==0) { memset(r+off,'A'+(unsigned)(off%20),n); r[off+2]=0; return 1; }
    if (pattern==1) { memset(r+off,'a'+(unsigned)(off%20),n); return 1; }
    memset(r+off,'z',n); r[off+n-2]=0; r[off+n-1]='!'; return 1;
}
static int compare_case(const ModernGekkoModuleDesc *m, CPUState *c, uint8_t *ram,
                        unsigned index, unsigned pattern, unsigned cache) {
    uint8_t *records=at(ram,RECORDS), *slot=records+index*NFLSTREET_FIELD_CACHE_RECORD_SIZE;
    memset(records,0xa5,NFLSTREET_FIELD_CACHE_RECORD_COUNT*NFLSTREET_FIELD_CACHE_RECORD_SIZE);
    set_string(slot,0,33,pattern); set_string(slot,0x21,33,(pattern+1)%3);
    set_string(slot,0x42,33,(pattern+2)%3); set_string(slot,99,102,pattern);
    slot[0xc9]=(uint8_t)(0x80u+index*7u); slot[0xca]=(uint8_t)cache;
    uint8_t record_before[NFLSTREET_FIELD_CACHE_RECORD_COUNT*NFLSTREET_FIELD_CACHE_RECORD_SIZE];
    memcpy(record_before,records,sizeof record_before);
    uint8_t *global=at(ram,R13-0x6e08u), *selected=at(ram,R13-0x6e04u);
    put32(global,RECORDS); put32(selected,0xdecafbad);
    memset(at(ram,DEST0),0x31,128); memset(at(ram,DEST1),0x42,128);
    memset(at(ram,DEST2),0x53,128); memset(at(ram,DEST3),0x64,128);
    memset(at(ram,VALUE),0x75,8);
    uint8_t before[4][128];
    memcpy(before[0],at(ram,DEST0),128); memcpy(before[1],at(ram,DEST1),128);
    memcpy(before[2],at(ram,DEST2),128); memcpy(before[3],at(ram,DEST3),128);

    nflstreet_field_cache_output native_before; memset(&native_before,0,sizeof native_before);
    memset(native_before.text0,0x31,sizeof native_before.text0);
    memset(native_before.text21,0x42,sizeof native_before.text21);
    memset(native_before.text42,0x53,sizeof native_before.text42);
    memset(native_before.text99,0x64,sizeof native_before.text99);
    native_before.value_c9=0x75; native_before.selected_index=0x76767676u;
    nflstreet_field_cache_output native=native_before;
    int native_ok=nflstreet_field_cache_read(records,NFLSTREET_FIELD_CACHE_RECORD_COUNT*NFLSTREET_FIELD_CACHE_RECORD_SIZE,
                                               NFLSTREET_FIELD_CACHE_RECORD_COUNT,index,&native);
    if (!cache) {
        return !native_ok && !memcmp(&native,&native_before,sizeof native);
    }
    memset(c,0,sizeof *c); c->ram=ram; c->ram_size=RAM_SIZE;
    c->gpr[1]=STACK; c->gpr[2]=0x803b9a40u; c->gpr[13]=R13;
    c->gpr[3]=index; c->gpr[4]=DEST0; c->gpr[5]=32;
    c->gpr[6]=DEST1; c->gpr[7]=32; c->gpr[8]=DEST2; c->gpr[9]=32;
    c->gpr[10]=VALUE; c->gpr[11]=DEST3; c->gpr[12]=101;
    /* PPC ABI parameters 9 and 10 are passed in the caller's parameter save area. */
    put32(at(ram,STACK+8),DEST3); put32(at(ram,STACK+12),101);
    if(!run(m,c)){fprintf(stderr,"dispatch failed pc=%08x exception=%08x\\n",c->pc,c->exception);return 0;}

    if(!native_ok || be32(selected)!=index || be32(at(ram,VALUE))!=slot[0xc9]) {fprintf(stderr,"state mismatch native=%d selected=%08x value=%08x expected=%02x\\n",native_ok,be32(selected),be32(at(ram,VALUE)),slot[0xc9]);return 0;}
    if(memcmp(native.text0,at(ram,DEST0),33)||memcmp(native.text21,at(ram,DEST1),33)||
       memcmp(native.text42,at(ram,DEST2),33)||memcmp(native.text99,at(ram,DEST3),102))return 0;
    if(memcmp(before[0]+33,at(ram,DEST0)+33,95)||memcmp(before[1]+33,at(ram,DEST1)+33,95)||
       memcmp(before[2]+33,at(ram,DEST2)+33,95)||memcmp(before[3]+102,at(ram,DEST3)+102,26))return 0;
    return native.selected_index==index && native.value_c9==slot[0xc9] &&
           memcmp(record_before,records,sizeof record_before)==0;
}

static int native_guard_checks(void) {
    uint8_t records[NFLSTREET_FIELD_CACHE_RECORD_COUNT*NFLSTREET_FIELD_CACHE_RECORD_SIZE];
    memset(records,'Q',sizeof records);
    records[0xca]=1;
    nflstreet_field_cache_output out,before;
    memset(&out,0x8d,sizeof out); before=out;
    if(nflstreet_field_cache_read(records,sizeof records,8,8,&out)||memcmp(&out,&before,sizeof out))return 0;
    if(nflstreet_field_cache_read(records,sizeof records,9,0,&out)||memcmp(&out,&before,sizeof out))return 0;
    if(nflstreet_field_cache_read(records,sizeof records-1,8,0,&out)||memcmp(&out,&before,sizeof out))return 0;
    if(nflstreet_field_cache_read(records,sizeof records,8,0,
                                  (nflstreet_field_cache_output *)(void *)records))return 0;
    return 1;
}

int main(int argc,char **argv) {
    if(argc!=2)return 2;
    HMODULE h=LoadLibraryA(argv[1]); if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get)return 3; const ModernGekkoModuleDesc*m=get();
    if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
    if(!native_guard_checks()){fprintf(stderr,"native bounded-input/overlap guard failed\n");return 4;}
    uint8_t *ram=calloc(1,RAM_SIZE);if(!ram)return 4; CPUState c;
    unsigned cases=0;
    for(unsigned i=0;i<NFLSTREET_FIELD_CACHE_RECORD_COUNT;++i)
      for(unsigned p=0;p<3;++p) for(unsigned enabled=0;enabled<2;++enabled){
        if(!compare_case(m,&c,ram,i,p,enabled)){fprintf(stderr,"cached field case failed index=%u pattern=%u cache=%u\n",i,p,enabled);return 5;} ++cases;
      }
    printf("Untouched original8011F1F0 cached branch matches native for %u enabled cases: eight 205-byte records, early-NUL/exact-bound/near-bound text spans at offsets 0/0x21/0x42/99, raw unsigned C9 byte, selected index, unchanged record bytes, and untouched destination tails. Separate native-only cache-flag rejection leaves output unchanged; original cache-miss construction is not modeled.\n",cases/2);
    free(ram);FreeLibrary(h);return 0;
}
