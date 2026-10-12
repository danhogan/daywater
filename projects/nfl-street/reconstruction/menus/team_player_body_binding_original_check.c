/* Untouched original body-pool code with the plamodel archive decoder as an
   explicit host boundary at 801A75B0. 801C65A0/801C67E4 execute original. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <moderngekko/module_abi.h>
#include "../native_assets/archive.h"
#include "team_player_body_binding.h"

#define RAM_SIZE (24u * 1024u * 1024u)
#define R13 0x803a9a40u
#define STACK 0x817df000u
#define POOL_TOKEN 0x81710000u
#define BUFFER_BASE 0x802ab684u
#define BODY_POINTERS 0x802760b8u
#define TYPE0_TABLE 0x802ab1e8u
#define BUFFER_BASES 0x81000000u
#define BUFFER_STRIDE 0x80000u

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
static void put32(uint8_t *p, uint32_t x) {
    p[0]=(uint8_t)(x>>24); p[1]=(uint8_t)(x>>16); p[2]=(uint8_t)(x>>8); p[3]=(uint8_t)x;
}
static uint8_t *at(uint8_t *ram, uint32_t a) {
    if (a < 0x80000000u || a-0x80000000u >= RAM_SIZE) return NULL;
    return ram+a-0x80000000u;
}
static int read_file(const char *path, uint8_t **out, size_t *size) {
    FILE *f=fopen(path,"rb"); if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;} long n=ftell(f);
    if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return 0;}
    uint8_t *p=malloc((size_t)n);
    if(!p||fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return 0;}
    fclose(f);*out=p;*size=(size_t)n;return 1;
}
static int load_dol(uint8_t *ram,const uint8_t *dol,size_t size) {
    if(size<0xe4)return 0;
    for(unsigned i=0;i<18;++i){
        uint32_t off=be32(dol+(i<7?0:0x1c)+4*(i<7?i:i-7));
        uint32_t addr=be32(dol+(i<7?0x48:0x64)+4*(i<7?i:i-7));
        uint32_t len=be32(dol+(i<7?0x90:0xac)+4*(i<7?i:i-7));
        if(!len) continue;
        uint8_t *dst=at(ram,addr);
        if(!dst||off>size||len>size-off) return 0;
        memcpy(dst,dol+off,len);
    }
    uint32_t bss=be32(dol+0xd8),n=be32(dol+0xdc);
    if(n){uint8_t *p=at(ram,bss);if(!p)return 0;memset(p,0,n);}return 1;
}
static nflstreet_archive archive;
static uint8_t *ram;
typedef struct {uint32_t index,buffer,record;size_t size;char name[128];} event;
static event events[12]; static unsigned event_count,attach_count,lock_count;
static int supplied_loader(CPUState *c) {
    if(event_count>=9||c->gpr[3]!=POOL_TOKEN||c->gpr[4]<1||c->gpr[4]>9){
        fprintf(stderr,"loader args count=%u pool=%08x index=%u out=%08x\n",event_count,c->gpr[3],c->gpr[4],c->gpr[5]);return 0;
    }
    uint8_t *dst=at(ram,c->gpr[5]);if(!dst){fprintf(stderr,"loader output outside MEM1: %08x\n",c->gpr[5]);return 0;}
    nflstreet_archive_entry entry;uint8_t *decoded=NULL;size_t size=0;
    if(!nflstreet_archive_get(&archive,c->gpr[4],&entry)||
       !nflstreet_archive_decode(&entry,&decoded,&size)||size>BUFFER_STRIDE){fprintf(stderr,"archive entry decode failed index=%u\n",c->gpr[4]);free(decoded);return 0;}
    nflstreet_team_body_binding binding={0};
    /* Parse Info through the public native binder using the actual archive. */
    unsigned key=(c->gpr[4]-1)/3,variant=(c->gpr[4]-1)%3;
    if(!nflstreet_team_body_binding_open(archive.bytes,archive.size,key,variant,&binding)||
       binding.archive_index!=c->gpr[4]||binding.size!=size){fprintf(stderr,"binding failed index=%u size=%zu\n",c->gpr[4],size);free(decoded);nflstreet_team_body_binding_close(&binding);return 0;}
    memcpy(dst,decoded,size);
    events[event_count++]=(event){c->gpr[4],c->gpr[5],0,size,{0}};
    strcpy(events[event_count-1].name,binding.info_name);
    free(decoded);nflstreet_team_body_binding_close(&binding);
    c->gpr[3]=0;c->pc=c->lr;return 1;
}
static void dispatch_bound(const ModernGekkoModuleDesc *m,CPUState *c) {
    if(c->pc==0x801a75b0u){if(!supplied_loader(c))c->exception=0xBAD00001u;return;}
    if(c->pc==0x801c5ec8u){lock_count++;c->gpr[3]=0;c->pc=c->lr;return;}
    if(c->pc==0x801c67e4u){
        if(attach_count<9){events[attach_count].record=c->gpr[3];attach_count++;}
    }
    c->downcount=c->cycle_budget=100000000;m->dispatch(c,c->pc);
}
static int run(const ModernGekkoModuleDesc *m,CPUState *c,unsigned max) {
    for(unsigned i=0;c->pc&&i<max;++i){if(c->exception)return 0;dispatch_bound(m,c);}
    return !c->pc&&!c->exception;
}
static int query_variant(const ModernGekkoModuleDesc *m,uint8_t *ram,uint32_t key,uint32_t variant,uint32_t *result) {
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.pc=0x800e0734u;c.lr=0;
    c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
    c.gpr[3]=0;c.gpr[4]=key;c.gpr[5]=variant;c.downcount=c.cycle_budget=100000000;
    if(!run(m,&c,100000)) return 0;
    *result=c.gpr[3];
    return 1;
}
int main(int argc,char **argv) {
    if(argc!=4){fprintf(stderr,"usage: body-binding-check <original-module.so> <original.dol> <plamodel.dat>\n");return 2;}
    void *lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);if(!lib){fprintf(stderr,"dlopen failed\n");return 3;}
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)dlsym(lib,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get) return 3;
    const ModernGekkoModuleDesc *m=get();
    if(!m||m->cpu_state_size!=sizeof(CPUState)) return 3;
    uint8_t *dol=NULL,*archive_bytes=NULL;size_t dol_size=0,archive_size=0;
    if(!read_file(argv[2],&dol,&dol_size)||!read_file(argv[3],&archive_bytes,&archive_size))return 4;
    if(!nflstreet_archive_open(archive_bytes,archive_size,&archive))return 5;
    ram=calloc(1,RAM_SIZE);if(!ram||!load_dol(ram,dol,dol_size))return 6;free(dol);
    /* Feed the nine original descriptor rows with bounded MEM1 output slots. */
    for(unsigned i=0;i<9;++i){uint8_t *p=at(ram,BUFFER_BASE+i*4);if(!p)return 7;put32(p,BUFFER_BASES+i*BUFFER_STRIDE);}
    uint8_t *p=at(ram,R13-0x5ae4u);if(!p)return 7;put32(p,POOL_TOKEN);
    p=at(ram,R13-0x6234u);if(!p)return 7;put32(p,0);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.pc=0x800dfbe4u;c.lr=0;
    c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.downcount=c.cycle_budget=100000000;
    if(!run(m,&c,1000000)){fprintf(stderr,"original constructor stopped pc=%08x exception=%08x\n",c.pc,c.exception);return 8;}
    if(event_count!=9||attach_count!=9||lock_count!=9)return 9;
    const uint32_t descs[3]={0x80271c98u,0x80271dd8u,0x80271f18u};
    for(unsigned i=0;i<9;++i){unsigned key=i/3,variant=i%3;uint32_t desc=be32(at(ram,BODY_POINTERS+key*4));
        uint32_t expected_desc=descs[key],record=expected_desc+0x1cu+variant*0x50u;
        uint32_t expected_index=1+i,buffer=BUFFER_BASES+i*BUFFER_STRIDE;
        nflstreet_team_body_binding binding={0};
        if(desc!=expected_desc||events[i].index!=expected_index||events[i].buffer!=buffer||
           events[i].record!=record||
           !nflstreet_team_body_binding_open(archive.bytes,archive.size,key,variant,&binding))return 10;
        if(events[i].size!=binding.size||strcmp(events[i].name,binding.info_name)||
           memcmp(at(ram,buffer),binding.bytes,binding.size))return 11;
        uint32_t got_name=0, got_info=0;
        if(!query_variant(m,ram,key,variant,&got_name)||got_name!=record){
            fprintf(stderr,"original E0734 record mismatch key=%u variant=%u got=%08x expected=%08x\n",key,variant,got_name,record);return 12;
        }
        /* 801C67E4 ran original on each decoded TMdl. The Info field it
           recorded is the raw section offset supplied by that exact file. */
        got_info=be32(at(ram,record));
        unsigned sections=(unsigned)binding.bytes[8]<<8|binding.bytes[9];
        size_t directory=(size_t)binding.bytes[10]<<8|binding.bytes[11];
        uint32_t expected_info=0;unsigned found=0;
        for(unsigned s=0;s<sections;++s){const uint8_t *row=binding.bytes+directory+(size_t)s*16;
            if(!memcmp(row,"Info",4)){expected_info=be32(row+4);found++;}}
        if(found!=1||got_info!=expected_info){fprintf(stderr,"original attach Info mismatch key=%u variant=%u got=%08x expected=%08x\n",key,variant,got_info,expected_info);return 13;}
        printf("key=%u variant=%u archive=%u Info=%s decoded=%zu original_record=%08x\n",key,variant,expected_index,binding.info_name,binding.size,record);
        nflstreet_team_body_binding_close(&binding);
    }
    /* The verified menu alias selects original pool key 2. */
    for(unsigned v=0;v<3;++v){uint32_t got=0;
        if(!query_variant(m,ram,2,v,&got)||got!=descs[2]+0x1cu+v*0x50u)return 14;
        nflstreet_team_body_binding alias={0};
        if(!nflstreet_team_body_binding_open(archive.bytes,archive.size,7,v,&alias)||
           alias.pool_key!=2||alias.archive_index!=7+v||
           strcmp(alias.info_name,events[6+v].name))return 15;
        nflstreet_team_body_binding_close(&alias);
    }
    nflstreet_team_body_binding unchanged={.tbyp=77,.pool_key=88,.variant=99,.archive_index=111};
    if(nflstreet_team_body_binding_open(archive.bytes,archive.size,3,0,&unchanged)||
       unchanged.tbyp!=77||unchanged.pool_key!=88||unchanged.variant!=99||unchanged.archive_index!=111||
       nflstreet_team_body_binding_open(archive.bytes,archive.size,0,3,&unchanged))return 16;
    printf("Passed: 9 actual plamodel TERF entries decoded and Info attached by original 801C67E4; original 800E0734 resolved each key/variant descriptor; TBYP 7 -> key 2 remains covered by the separate route oracle. Loader 801A75B0 was the supplied archive-decoding boundary; no renderer or runtime resource-host claim.\n");
    free(archive_bytes);free(ram);dlclose(lib);return 0;
}
