/* Fresh isolated actor initialization; allocators are supplied, all channel
   helpers and the original model initializer execute in the preserved DLL. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "actor_model_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define ACTOR 0x817b0000u
#define ARRAYS 0x817d0000u
#define HEAP 0x81600000u
#define HEAP_STRIDE 0x4000u
#define MAX_BLOCKS 7
static uint16_t be16(const uint8_t*p){return(uint16_t)((uint16_t)p[0]<<8|p[1]);}
static uint32_t be(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t*ptr(uint8_t*ram,uint32_t a,size_t n){if(a<BASE||(uint64_t)a-BASE+n>RAM_SIZE)return NULL;return ram+a-BASE;}
typedef struct { uint32_t size,count,kind; uint8_t storage[HEAP_STRIDE]; } block;
typedef struct { block blocks[MAX_BLOCKS];unsigned count; } allocations;
static int allocate(void*opaque,const nflstreet_actor_model_allocation*q,nflstreet_actor_model_memory*out){
    allocations*t=opaque;if(t->count>=MAX_BLOCKS||q->bytes>HEAP_STRIDE)return 0;
    unsigned i=t->count++;t->blocks[i].size=q->bytes;t->blocks[i].count=q->source_count;t->blocks[i].kind=q->kind;
    memset(t->blocks[i].storage,0xa5,HEAP_STRIDE);out->token=HEAP+i*HEAP_STRIDE;out->bytes=t->blocks[i].storage;out->size=HEAP_STRIDE;return 1;
}
static uint8_t*load(const char*path){FILE*f=fopen(path,"rb");if(!f)return NULL;uint8_t*b=malloc(RAM_SIZE);if(!b||fread(b,1,RAM_SIZE,f)!=RAM_SIZE){free(b);fclose(f);return NULL;}fclose(f);return b;}
static nflstreet_actor_model_resource resource(uint8_t*ram,uint32_t token){
    nflstreet_actor_model_resource r={0};if(!token)return r;uint8_t*p=ptr(ram,token,8);if(!p)return r;
    size_t n=0x410u+6u*be16(p+6);if(n<0x410u+19u*6u)n=0x410u+19u*6u;
    if(!ptr(ram,token,n))return r;r.token=token;r.bytes=p;r.size=n;return r;
}
static int original(const ModernGekkoModuleDesc*m,uint8_t*ram,const nflstreet_actor_model_descriptor*d,allocations*t){
    memset(ptr(ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),0,NFLSTREET_ACTOR_MODEL_STORAGE);
    for(unsigned i=0;i<2;++i){put(ptr(ram,ARRAYS+4*i,4),d->primary[i].token);put(ptr(ram,ARRAYS+8+4*i,4),d->secondary[i].token);}
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
    c.gpr[3]=ACTOR;c.gpr[4]=d->skeleton.token;c.gpr[5]=ARRAYS;c.gpr[6]=ARRAYS+8;c.gpr[7]=d->composite.token;c.pc=0x800d4e0cu;c.cycle_budget=200000000;
    uint32_t channel_count=0;int channel_request=0;
    for(unsigned step=0;c.pc&&step<200000;++step){
        if(c.pc==0x8015e910u){channel_count=c.gpr[3]&0xffffu;channel_request=1;}
        if(c.pc==0x8018b7b8u){if(t->count>=MAX_BLOCKS||c.gpr[3]>HEAP_STRIDE||c.gpr[4])return 0;
            unsigned i=t->count++;block*b=t->blocks+i;b->size=c.gpr[3];b->count=channel_request?channel_count:c.gpr[3]/64;b->kind=channel_request?NFLSTREET_ACTOR_CHANNEL:NFLSTREET_ACTOR_TRANSFORMS;
            memset(ptr(ram,HEAP+i*HEAP_STRIDE,HEAP_STRIDE),0xa5,HEAP_STRIDE);c.gpr[3]=HEAP+i*HEAP_STRIDE;c.pc=c.lr;channel_request=0;continue;}
        if(c.pc==0x8017bc54u){uint8_t*p=ptr(ram,c.gpr[3],c.gpr[5]);if(!p)return 0;memset(p,(uint8_t)c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
        c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"original actor exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}
    }
    if(c.pc)return 0;for(unsigned i=0;i<t->count;++i)memcpy(t->blocks[i].storage,ptr(ram,HEAP+i*HEAP_STRIDE,HEAP_STRIDE),HEAP_STRIDE);return 1;
}
static int check(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const nflstreet_actor_model_descriptor*d,unsigned mask,const char*label){
    uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    nflstreet_actor_model_descriptor selected=*d;if(!(mask&1))memset(&selected.skeleton,0,sizeof selected.skeleton);
    if(!(mask&2))memset(&selected.primary[0],0,sizeof selected.primary[0]);if(!(mask&4))memset(&selected.primary[1],0,sizeof selected.primary[1]);
    if(!(mask&8))memset(&selected.secondary[0],0,sizeof selected.secondary[0]);if(!(mask&16))memset(&selected.secondary[1],0,sizeof selected.secondary[1]);
    if(!(mask&32))memset(&selected.composite,0,sizeof selected.composite);
    allocations*o=calloc(1,sizeof *o),*n=calloc(1,sizeof *n);uint8_t native[NFLSTREET_ACTOR_MODEL_STORAGE]={0};
    if(!o||!n||!original(m,ram,&selected,o)||!nflstreet_actor_model_initialize(native,sizeof native,&selected,allocate,n)){fprintf(stderr,"actor mask%u failedexecution\n",mask);free(o);free(n);free(ram);return 0;}
    int ok=!memcmp(native,ptr(ram,ACTOR,sizeof native),sizeof native)&&o->count==n->count;
    if(ok)for(unsigned i=0;i<o->count;++i){if(memcmp(o->blocks+i,n->blocks+i,sizeof(block))){fprintf(stderr,"actor mask%u allocation%u mismatch size%u/%u count%u/%u kind%u/%u\n",mask,i,o->blocks[i].size,n->blocks[i].size,o->blocks[i].count,n->blocks[i].count,o->blocks[i].kind,n->blocks[i].kind);ok=0;break;}}
    if(!ok){for(unsigned i=0;i<sizeof native;++i)if(native[i]!=ptr(ram,ACTOR,sizeof native)[i]){fprintf(stderr,"actor mask%u byte%04x native%02x original%02x\n",mask,i,native[i],ptr(ram,ACTOR,sizeof native)[i]);break;}
    } /* Keep first differing owned byte diagnostic bounded. */
    if(ok)printf("%s mask=%u: full actor1A90 +%u allocated blocks exact\n",label,mask,o->count);
    free(o);free(n);free(ram);return ok;
}
int main(int argc,char**argv){
    if(argc!=3){fprintf(stderr,"Usage: actor-model-init-original-check.exe originalDLL savedRAM\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    uint8_t*baseline=load(argv[2]);if(!baseline){FreeLibrary(h);return 4;}
    uint8_t*slot=ptr(baseline,0x803a9a40u-0x7bbcu,4);uint8_t*list=slot?ptr(baseline,be(slot),4):NULL;uint8_t*a=list?ptr(baseline,be(list),NFLSTREET_ACTOR_MODEL_STORAGE):NULL;
    if(!a){free(baseline);FreeLibrary(h);return 4;}nflstreet_actor_model_descriptor d={0};
    d.skeleton=resource(baseline,be(a+0x64));d.primary[0]=resource(baseline,be(a+0xbc));d.primary[1]=resource(baseline,be(a+0x104));
    d.secondary[0]=resource(baseline,be(a+0x194));d.secondary[1]=resource(baseline,be(a+0x210));d.composite=resource(baseline,be(a+0x70));d.initial_word_bits=be(ptr(baseline,0x8025283cu,4));
    if(!d.skeleton.token||!d.primary[0].token||!d.primary[1].token||!d.secondary[0].token||!d.secondary[1].token||!d.composite.token){free(baseline);FreeLibrary(h);return 4;}
    printf("Source descriptor bones=%u/%u/%u/%u/%u/%u scale=%08x\n",be16(d.skeleton.bytes+6),be16(d.primary[0].bytes+6),be16(d.primary[1].bytes+6),be16(d.secondary[0].bytes+6),be16(d.secondary[1].bytes+6),be16(d.composite.bytes+6),d.initial_word_bits);
    int ok=1;for(unsigned mask=0;mask<64&&ok;++mask)ok=check(m,baseline,&d,mask,"source-descriptor");
    free(baseline);FreeLibrary(h);if(ok)puts("Original800D4E0C + original channel helpers match semantic owned initialization for all64 part masks; fresh isolated objects, no complete player created.");return ok?0:5;
}
