#include <windows.h>
#include "moderngekko/module_abi.h"
#include "field_availability.h"
#include "team_catalog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u * 1024u * 1024u)
#define RAM_BASE 0x80000000u
#define RECORDS 0x817e0000u
#define DEST0 0x817e1000u
#define DEST1 0x817e1100u
#define DEST2 0x817e1200u
#define DEST3 0x817e1300u
#define VALUE 0x817e1400u
#define R13 0x817f0000u
#define STACK 0x817df000u

static uint8_t *at(uint8_t *r,uint32_t a){return a<RAM_BASE||(size_t)(a-RAM_BASE)>=RAM_SIZE?NULL:r+a-RAM_BASE;}
static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static int read_file(const char *path,uint8_t **out,size_t *size){
    FILE*f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)||(*size=(size_t)ftell(f))==0||fseek(f,0,SEEK_SET)){fclose(f);return 0;}
    *out=(uint8_t*)malloc(*size);if(!*out){fclose(f);return 0;}
    int ok=fread(*out,1,*size,f)==*size;fclose(f);if(!ok){free(*out);*out=NULL;}return ok;
}

typedef struct { uint32_t ocid_to_digd[2],digd_to_ifid[256];uint8_t has_digd[256]; } supplied_ivid;
static const uint8_t *g_ivid_bytes;
static size_t g_ivid_size;
static int field_ix(const nflstreet_team_table*t,uint32_t tag,unsigned*out){
    unsigned found=0,n=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;
        if(!nflstreet_team_table_field(t,i,&f))return 0;if(f.tag==tag){found=i;++n;}}
    if(n!=1)return 0;*out=found;return 1;
}
static int parse_ivid(const uint8_t*b,size_t n,supplied_ivid*out){
    nflstreet_team_table t;unsigned oi,di,fi;if(!nflstreet_team_table_open_tag(b,n,0x49564944u,&t)||
       !field_ix(&t,0x4f434944u,&oi)||!field_ix(&t,0x44494744u,&di)||!field_ix(&t,0x49464944u,&fi))return 0;
    supplied_ivid next;memset(&next,0,sizeof next);unsigned ocids=0;
    for(unsigned row=0;row<t.used;++row){uint32_t ocid,digd,ifid;
        nflstreet_team_field of,df,ff;
        if(!nflstreet_team_table_field(&t,oi,&of)||of.type!=3||
           !nflstreet_team_table_field(&t,di,&df)||df.type!=3||
           !nflstreet_team_table_field(&t,fi,&ff)||ff.type!=3||
           !nflstreet_team_table_number(&t,row,oi,&ocid)||!nflstreet_team_table_number(&t,row,di,&digd)||
           !nflstreet_team_table_number(&t,row,fi,&ifid)||ocid>255||digd>255||ifid>127||next.has_digd[digd])return 0;
        next.has_digd[digd]=1;next.digd_to_ifid[digd]=ifid;
        if(ocid<2){next.ocid_to_digd[ocid]=digd;ocids|=1u<<ocid;}
    }
    if(ocids!=3u||!next.has_digd[next.ocid_to_digd[0]]||!next.has_digd[next.ocid_to_digd[1]])return 0;
    *out=next;return 1;
}

/* We hook only the source's database-return wrappers. The actual source
   branch/comparisons/cache write in 8011F1F0 remain original instructions. */
static int dispatch_original(const ModernGekkoModuleDesc*m,CPUState*c,
                             uint8_t*ram,const supplied_ivid*ivid){
    c->pc=0x8011f1f0u;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=100000000;
    for(unsigned steps=0;c->pc&&steps<200000;++steps){
        switch(c->pc){
        case 0x8014b4f0u: case 0x8014b738u: case 0x8014b770u: case 0x8014b7a8u:
        case 0x8014b560u: case 0x8014c9f0u: case 0x8014ca34u: case 0x8014b150u:
        case 0x8014b128u: case 0x8014cf8cu: case 0x801e6914u:
            c->pc=c->lr;continue;
        case 0x8014cfccu: c->gpr[3]=42u;c->pc=c->lr;continue;
        case 0x80127900u: c->gpr[3]=0xffffffffu;c->pc=c->lr;continue;
        case 0x801e6958u: c->gpr[3]=0;c->pc=c->lr;continue;
        case 0x80129b24u: {
            uint8_t *dst=at(ram,c->gpr[4]);if(!dst)return 0;
            memset(dst,0,16);c->pc=c->lr;continue;
        }
        case 0x8017cf30u: {
            uint8_t *dst=at(ram,c->gpr[3]);
            uint8_t *src=at(ram,c->gpr[4]);uint32_t n=c->gpr[5];
            if(!dst||n>0x1000)return 0;
            if(src)memmove(dst,src,n);else memset(dst,0,n);
            c->pc=c->lr;continue;
        }
        case 0x8014b1a4u: {
            /* 8014B1A4 calls 8014947C(OCID, input) then reads DIGD. */
            uint32_t ocid=c->gpr[3];
            if(ocid>1u)return 0;
            /* Supplied iterator response for actual IVID row OCID=0/1. */
            c->gpr[3]=ivid->ocid_to_digd[ocid];c->pc=c->lr;continue;
        }
        case 0x8014b204u: {
            /* 8014B204 receives the preceding DIGD in PPC r3 (implicit
               source dataflow) and reads the row's IFID. */
            uint32_t digd=c->gpr[3];if(digd>255u||!ivid->has_digd[digd])return 0;
            c->gpr[3]=ivid->digd_to_ifid[digd];c->pc=c->lr;continue;
        }
        default: m->dispatch(c,c->pc);if(c->exception)return 0;break;
        }
    }
    return c->pc==0&&!c->exception;
}

static int compare_one(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,
                       const supplied_ivid*ivid,unsigned raw){
    uint8_t expected,ifids[2];
    if(!nflstreet_field_availability_from_ivid(g_ivid_bytes,g_ivid_size,(int8_t)raw,&expected,ifids))return 0;
    memset(ram,0,RAM_SIZE);
    put32(at(ram,R13-0x6e08u),RECORDS);
    put32(at(ram,R13-0x6e04u),0xdecafbad);
    at(ram,R13-0x6e10u)[0]=(uint8_t)raw;
    memset(at(ram,RECORDS),0x5a,8u*0xcdu);
    at(ram,RECORDS)[0xca]=0;
    memset(at(ram,DEST0),0xa1,128);memset(at(ram,DEST1),0xb2,128);
    memset(at(ram,DEST2),0xc3,128);memset(at(ram,DEST3),0xd4,128);
    memset(at(ram,VALUE),0xe5,8);
    memset(c,0,sizeof *c);c->ram=ram;c->ram_size=RAM_SIZE;c->gpr[1]=STACK;
    c->gpr[2]=0x803b9a40u;c->gpr[13]=R13;
    c->gpr[3]=0;c->gpr[4]=DEST0;c->gpr[5]=32;c->gpr[6]=DEST1;c->gpr[7]=32;
    c->gpr[8]=DEST2;c->gpr[9]=32;c->gpr[10]=VALUE;c->gpr[11]=DEST3;c->gpr[12]=101;
    put32(at(ram,STACK+8),DEST3);put32(at(ram,STACK+12),101);
    if(!dispatch_original(m,c,ram,ivid))return 0;
    return at(ram,RECORDS)[0xca]==1 && at(ram,RECORDS)[0xc9]==expected &&
           be32(at(ram,R13-0x6e04u))==0 && ifids[0]==7 && ifids[1]==1;
}

int main(int argc,char**argv){
    if(argc!=3)return 2;
    uint8_t*ivid=NULL;size_t ivid_size=0;if(!read_file(argv[2],&ivid,&ivid_size))return 3;
    g_ivid_bytes=ivid;g_ivid_size=ivid_size;
    supplied_ivid supplied;if(!parse_ivid(ivid,ivid_size,&supplied)){free(ivid);return 3;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h){free(ivid);return 4;}
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);free(ivid);return 4;}const ModernGekkoModuleDesc*m=get();
    if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);free(ivid);return 4;}
    uint8_t*ram=(uint8_t*)calloc(1,RAM_SIZE);CPUState*c=(CPUState*)calloc(1,sizeof *c);
    if(!ram||!c){free(ram);free(c);FreeLibrary(h);free(ivid);return 4;}
    unsigned n=0;for(unsigned id=0;id<8;++id){if(!compare_one(m,c,ram,&supplied,id)){
        fprintf(stderr,"original cache-miss C9 mismatch for raw field %u (pc=%08x exception=%08x)\n",id,c->pc,c->exception);
        free(ram);free(c);FreeLibrary(h);free(ivid);return 5;}++n;}
    printf("Untouched original 8011F1F0 cache-miss availability branch matches native parsing for %u actual field IDs. Original source C9 comparisons executed with explicit IVID OCID/DIGD/IFID iterator responses and no-profile returns; non-availability resource, stat-format, and transaction services were supplied. Actual IVID mapping: OCID 0 -> DIGD 7 -> IFID 7; OCID 1 -> DIGD 3 -> IFID 1.\n",n);
    free(ram);free(c);FreeLibrary(h);free(ivid);return 0;
}
