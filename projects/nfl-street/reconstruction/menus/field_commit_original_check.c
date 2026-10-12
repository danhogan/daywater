#include <windows.h>
#include "moderngekko/module_abi.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define R13 0x803a9a40u
#define FIELD_KEYS (R13-0x6e10u)
#define SELECTED_SLOT (R13-0x6e04u)
#define QUICK_MODE (R13-0x6f88u)
#define FIELD_KEY_GLOBAL 0x802a301cu
#define FORMATION_GLOBAL 0x802a4430u
#define SIDE_WORDS 14u
#define SIDE_BYTES (SIDE_WORDS*4u)

typedef struct {
    uint32_t field_key;
    uint32_t sides[2][SIDE_WORDS];
    uint32_t call_order[5];
    unsigned call_count;
    unsigned field_request_count;
    unsigned settings_count;
    unsigned one_time_init_count;
    unsigned cache_free_count;
    uint32_t cache_free_arg;
    uint32_t array_ptrs[2];
} capture;

static uint8_t *ram_ptr(uint8_t *ram,uint32_t address,size_t bytes){
    if(address<BASE)return NULL;
    uint64_t offset=(uint64_t)address-BASE;
    if(offset+bytes>RAM_SIZE)return NULL;
    return ram+(size_t)offset;
}
static uint32_t get32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put32(uint8_t*p,uint32_t x){p[0]=(uint8_t)(x>>24);p[1]=(uint8_t)(x>>16);p[2]=(uint8_t)(x>>8);p[3]=(uint8_t)x;}
static uint8_t *load_file(const char*path,size_t*size){
    FILE*f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
    if(n<1||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t*b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return b;
}
static int parse_unsigned_after(const char*text,const char*key,unsigned long*out){
    const char*p=strstr(text,key);if(!p)return 0;p+=strlen(key);
    while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n'||*p==':')++p;
    char*end=NULL;unsigned long value=strtoul(p,&end,10);if(end==p)return 0;*out=value;return 1;
}
static int load_native_selection(const char*path,uint32_t slots[2][SIDE_WORDS],unsigned*field_slot,unsigned*field_key){
    size_t n=0;char*text=(char*)load_file(path,&n);if(!text)return 0;
    if(n>65536u){free(text);return 0;}text=realloc(text,n+1);if(!text)return 0;text[n]=0;
    unsigned long slot=0,key=0;
    const char*p=strstr(text,"\"formation_players\"");
    if(!p||!parse_unsigned_after(text,"\"field_slot\"",&slot)||
       !parse_unsigned_after(text,"\"field_key\"",&key)||slot>=8||key>=8){free(text);return 0;}
    p=strchr(p,'[');if(!p){free(text);return 0;}++p;
    for(unsigned i=0;i<2*SIDE_WORDS;++i){
        while(*p&&(*p<'0'||*p>'9'))++p;
        if(!*p){free(text);return 0;}
        char*end=NULL;unsigned long value=strtoul(p,&end,10);
        if(end==p||value>UINT32_MAX){free(text);return 0;}
        slots[i/SIDE_WORDS][i%SIDE_WORDS]=(uint32_t)value;p=end;
    }
    *field_slot=(unsigned)slot;*field_key=(unsigned)key;free(text);return 1;
}
static int append(capture*c,uint32_t tag){if(c->call_count>=5)return 0;c->call_order[c->call_count++]=tag;return 1;}

static int run_original(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,capture*out){
    memset(out,0,sizeof *out);
    uint8_t*key_global=ram_ptr(ram,FIELD_KEY_GLOBAL,4);
    uint8_t*formations=ram_ptr(ram,FORMATION_GLOBAL,2*SIDE_BYTES);
    if(!key_global||!formations)return 0;
    memset(c,0,sizeof *c);c->ram=ram;c->ram_size=RAM_SIZE;c->msr=0x2000;
    c->gpr[1]=0x81700000u;c->gpr[2]=0x803b9a40u;c->gpr[13]=R13;
    c->pc=0x8011f170u;c->lr=0;c->downcount=c->cycle_budget=100000000u;
    for(unsigned step=0;c->pc&&step<100000;++step){
        switch(c->pc){
        case 0x801211d4u:
            if(!append(out,1)){return 0;}c->pc=c->lr;break;
        case 0x801ea480u:
            if(!append(out,2)||c->gpr[3]<BASE||c->gpr[4]<BASE)return 0;
            out->array_ptrs[0]=c->gpr[3];out->array_ptrs[1]=c->gpr[4];
            for(unsigned side=0;side<2;++side){
                uint8_t*src=ram_ptr(ram,c->gpr[3+side],SIDE_BYTES);if(!src)return 0;
                for(unsigned i=0;i<SIDE_WORDS;++i)out->sides[side][i]=get32(src+i*4);
            }
            out->field_key=get32(key_global);++out->field_request_count;c->gpr[3]=0;c->pc=c->lr;break;
        case 0x8014c148u:
            if(!append(out,3))return 0;++out->settings_count;c->pc=c->lr;break;
        case 0x800040f0u:
            if(!append(out,4))return 0;++out->one_time_init_count;c->pc=c->lr;break;
        case 0x8018baa8u:
            if(!append(out,5))return 0;++out->cache_free_count;out->cache_free_arg=c->gpr[3];c->pc=c->lr;break;
        default:
            m->dispatch(c,c->pc);if(c->exception){fprintf(stderr,"original commit exception pc=%08x ex=%08x\n",c->pc,c->exception);return 0;}
        }
    }
    if(c->pc){fprintf(stderr,"original commit stopped pc=%08x\n",c->pc);return 0;}
    return 1;
}

int main(int argc,char**argv){
    if(argc!=4){fprintf(stderr,"Usage: field-commit-original-check.exe <original-DLL> <saved-RAM> <native-selection.json>\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();
    if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    size_t ram_size=0;uint8_t*ram=load_file(argv[2],&ram_size);
    if(!ram||ram_size!=RAM_SIZE){free(ram);FreeLibrary(h);return 4;}
    capture got;CPUState*c=calloc(1,sizeof *c);if(!c){free(ram);FreeLibrary(h);return 4;}
    uint32_t expected[2][SIDE_WORDS];unsigned slot=0,native_key=0;
    if(!load_native_selection(argv[3],expected,&slot,&native_key)){fprintf(stderr,"bad native handoff snapshot\n");goto fail;}
    uint8_t*keys=ram_ptr(ram,FIELD_KEYS,8),*selected=ram_ptr(ram,SELECTED_SLOT,4),*mode=ram_ptr(ram,QUICK_MODE,4);
    if(!keys||!selected||!mode||keys[slot]>=8||keys[slot]!=native_key){fprintf(stderr,"saved DOL field order and native selected key disagree\n");goto fail;}
    put32(selected,slot);put32(mode,1);
    uint8_t*formation_memory=ram_ptr(ram,FORMATION_GLOBAL,2*SIDE_BYTES);
    if(!formation_memory)goto fail;
    for(unsigned side=0;side<2;++side)for(unsigned i=0;i<SIDE_WORDS;++i)
        put32(formation_memory+(side*SIDE_WORDS+i)*4u,expected[side][i]);
    if(!run_original(m,c,ram,&got))goto fail;
    if(got.call_count!=5||got.call_order[0]!=1||got.call_order[1]!=2||got.call_order[2]!=3||
       got.call_order[3]!=4||got.call_order[4]!=5||got.field_request_count!=1||
       got.settings_count!=1||got.one_time_init_count!=1||got.cache_free_count!=1||
       got.field_key!=native_key||memcmp(got.sides,expected,sizeof expected)){
        fprintf(stderr,"original commit mismatch key=%u expected=%u calls=%u/%u/%u/%u/%u order=%u,%u,%u,%u,%u arrays=%08x/%08x\n",
            got.field_key,native_key,got.call_count,got.field_request_count,got.settings_count,
            got.one_time_init_count,got.cache_free_count,got.call_order[0],got.call_order[1],
            got.call_order[2],got.call_order[3],got.call_order[4],got.array_ptrs[0],got.array_ptrs[1]);
        for(unsigned side=0;side<2;++side)for(unsigned i=0;i<SIDE_WORDS;++i)
            if(got.sides[side][i]!=expected[side][i])fprintf(stderr,"side%u word%u original=%u native=%u\n",side,i,got.sides[side][i],expected[side][i]);
        goto fail;
    }
    printf("Original8011F170 selected-field commit matched native handoff: slot=%u DOL key=%u; copied formation arrays=2x14 words exact; request, settings-init, one-time-init and cache-free boundaries observed in order. 801EA480 was captured and not executed; field cache/profile population and match creation are outside this comparison.\n",slot,native_key);
    free(c);free(ram);FreeLibrary(h);return 0;
fail:
    free(c);free(ram);FreeLibrary(h);return 5;
}
