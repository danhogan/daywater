#include "match_settings.h"
#include <windows.h>
#include "moderngekko/module_abi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct { unsigned calls; unsigned requests[8]; int fail_kind; int32_t fail_status; } fixture;
static int32_t query(void *opaque, nflstreet_match_settings_query kind,
    uint32_t argument, uint32_t *values, uint32_t count, double float_arg) {
    fixture *f = opaque;
    if (!f || f->calls >= 8) return -1;
    f->requests[f->calls++] = (unsigned)kind;
    if (f->fail_kind == (int)kind) return f->fail_status;
    if (kind == NFLSTREET_SETTING_INITIAL) {
        if (count != NFLSTREET_MATCH_SETTINGS_WORDS) return -2;
        for (uint32_t i=0;i<count;++i) values[i]=0x1000u+i;
        values[NFLSTREET_SETTING_GTYG]=0x4479ff5cu;
        values[NFLSTREET_SETTING_DOTG]=2047;
        values[NFLSTREET_SETTING_HTWG]=15;
        values[NFLSTREET_SETTING_GPWG]=127;
        values[NFLSTREET_SETTING_GTWG]=127;
        values[NFLSTREET_SETTING_GFWG]=127;
        values[NFLSTREET_SETTING_GWWG]=UINT32_MAX;
        values[NFLSTREET_SETTING_OTHG]=4;
        values[NFLSTREET_SETTING_HCHG]=3;
        values[NFLSTREET_SETTING_CSHG]=255;
        values[NFLSTREET_SETTING_RTQG]=1023;
        values[NFLSTREET_SETTING_WODG]=15;
        values[NFLSTREET_SETTING_PNIG]=255;
        values[NFLSTREET_SETTING_IPHG]=63;
        values[NFLSTREET_SETTING_PDHG]=7;
        values[NFLSTREET_SETTING_IPAG]=63;
        values[NFLSTREET_SETTING_PDAG]=8;
        return 0;
    }
    if (kind == NFLSTREET_SETTING_DEPENDENT_DEFAULT) {
        if (count != 1 || argument != 0) return -3;
        values[0]=55;
        return 0;
    }
    if (kind == NFLSTREET_SETTING_SIDE_PAIR) {
        if (count != 2) return -4;
        values[0]=argument+1; values[1]=argument+2;
        return 0;
    }
    if (kind == NFLSTREET_SETTING_SESSION_INSERT) {
        if (count != NFLSTREET_MATCH_SETTINGS_WORDS ||
            (values[NFLSTREET_SETTING_GTYG]==0x41200000u && float_arg!=10.0) ||
            (values[NFLSTREET_SETTING_GTYG]==0x4479ff5cu && float_arg<999.0))
            return -5;
        return 0;
    }
    return -5;
}

static int run(uint32_t mode) {
    fixture f={0,{0},-1,0};
    nflstreet_match_settings_result r;
    if (!nflstreet_match_settings_resolve(mode,query,&f,&r)) return 1;
    if (r.mode != mode || r.status || r.query_values[0] != 0x4479ff5cu ||
        r.query_values[24] != 255u) return 2;
    if (mode == 0) {
        if (f.calls!=5 || r.dependent_default_requests!=1 ||
            r.side_pair_requests!=2 || !r.insert_descriptor_ready ||
            r.normalized_values[NFLSTREET_SETTING_GTYG]!=0x41200000u ||
            r.normalized_values[NFLSTREET_SETTING_DOTG]!=780 ||
            r.normalized_values[NFLSTREET_SETTING_GTWG]!=78 ||
            r.normalized_values[NFLSTREET_SETTING_PDHG]!=7 ||
            r.normalized_values[NFLSTREET_SETTING_PDAG]!=8 ||
            r.side_context[0]!=0x1008u || r.side_context[1]!=0x100eu ||
            memcmp(r.session_insert_values,r.normalized_values,
                sizeof r.session_insert_values)) return 3;
    } else if (f.calls!=4 || r.dependent_default_requests ||
        r.side_pair_requests!=2 || !r.insert_descriptor_ready ||
        r.normalized_values[NFLSTREET_SETTING_GTYG]!=0x4479ff5cu ||
        r.normalized_values[NFLSTREET_SETTING_PDHG]!=7 ||
        r.normalized_values[NFLSTREET_SETTING_PDAG]!=8) return 4;
    return 0;
}

static int run_owned_fixtures(void) {
    /* These fixtures validate the owned descriptor with explicit callback
       results. The DLL differential below executes the untouched original. */
    for (uint32_t mode=0;mode<3;++mode) {
        int rc=run(mode);
        if(rc){fprintf(stderr,"settings fixture mode %u failed: %d\n",mode,rc);return rc;}
    }
    const nflstreet_match_settings_query failures[]={NFLSTREET_SETTING_INITIAL,
        NFLSTREET_SETTING_DEPENDENT_DEFAULT,NFLSTREET_SETTING_SIDE_PAIR,
        NFLSTREET_SETTING_SESSION_INSERT};
    for(unsigned i=0;i<sizeof failures/sizeof failures[0];++i){
        fixture f={0,{0},(int)failures[i],0x31+(int32_t)i};
        nflstreet_match_settings_result r;
        if(!nflstreet_match_settings_resolve(0,query,&f,&r)||
           r.status!=f.fail_status||r.side_context[0]!=1023||r.side_context[1]!=1023||
           r.insert_descriptor_ready){
            fprintf(stderr,"settings error fixture %u did not retain sentinel outputs\n",i);return 5;
        }
    }
    puts("settings descriptor fixtures passed for explicit query outputs in modes 0, 1, 2");
    return 0;
}

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define STACK_TOP 0x81700000u
#define OUT_A 0x817e0000u
#define OUT_B 0x817e0004u
typedef struct { uint32_t values[NFLSTREET_MATCH_SETTINGS_WORDS]; } explicit_plan;
typedef struct {
    unsigned calls, kinds[8];
    uint32_t insert_words[NFLSTREET_MATCH_SETTINGS_WORDS];
    double insert_float;
    int fail_kind;
    int32_t fail_status;
} native_trace;
typedef struct {
    unsigned query_count;
    uint32_t query_addresses[8];
    uint32_t insert_words[NFLSTREET_MATCH_SETTINGS_WORDS-1];
    double insert_float;
    uint32_t outputs[2];
    uint32_t status;
    uint32_t fail_address;
    int32_t fail_status;
} original_trace;

static void fill_values(uint32_t out[NFLSTREET_MATCH_SETTINGS_WORDS]) {
    for (unsigned i=0;i<NFLSTREET_MATCH_SETTINGS_WORDS;++i) out[i]=0x100u+i;
    out[NFLSTREET_SETTING_GTYG]=0x4479ff5cu;
    out[NFLSTREET_SETTING_ISFG]=111;
    out[NFLSTREET_SETTING_DOTG]=2047;
    out[NFLSTREET_SETTING_HTWG]=15;
    out[NFLSTREET_SETTING_GPWG]=127;
    out[NFLSTREET_SETTING_GTWG]=127;
    out[NFLSTREET_SETTING_GFWG]=127;
    out[NFLSTREET_SETTING_GWWG]=UINT32_MAX;
    out[NFLSTREET_SETTING_GTHG]=0x10;
    out[NFLSTREET_SETTING_IPHG]=63;
    out[NFLSTREET_SETTING_PDHG]=7;
    out[NFLSTREET_SETTING_OTHG]=4;
    out[NFLSTREET_SETTING_HCHG]=3;
    out[NFLSTREET_SETTING_CSHG]=255;
    out[NFLSTREET_SETTING_GTAG]=0x20;
    out[NFLSTREET_SETTING_IPAG]=63;
    out[NFLSTREET_SETTING_PDAG]=8;
    out[NFLSTREET_SETTING_OTAG]=4;
    out[NFLSTREET_SETTING_HCAG]=3;
    out[NFLSTREET_SETTING_CSAG]=255;
    out[NFLSTREET_SETTING_MITG]=UINT32_MAX;
    out[NFLSTREET_SETTING_RTQG]=1023;
    out[NFLSTREET_SETTING_ELQG]=99;
    out[NFLSTREET_SETTING_WODG]=15;
    out[NFLSTREET_SETTING_PNIG]=255;
}
static int32_t native_query(void *opaque,nflstreet_match_settings_query kind,
    uint32_t argument,uint32_t *values,uint32_t count,double farg) {
    native_trace*t=opaque;if(!t||t->calls>=8)return -1;
    t->kinds[t->calls++]=(unsigned)kind;
    if(t->fail_kind==(int)kind)return t->fail_status;
    if(kind==NFLSTREET_SETTING_INITIAL){if(count!=25)return -2;fill_values(values);return 0;}
    if(kind==NFLSTREET_SETTING_DEPENDENT_DEFAULT){if(count!=1)return -3;values[0]=55;return 0;}
    if(kind==NFLSTREET_SETTING_SIDE_PAIR){if(count!=2)return -4;values[0]=argument+1;values[1]=argument+2;return 0;}
    if(kind==NFLSTREET_SETTING_SESSION_INSERT){if(count!=25)return -5;memcpy(t->insert_words,values,sizeof t->insert_words);t->insert_float=farg;return 0;}
    return -6;
}
static uint8_t *ram_ptr(uint8_t*ram,uint32_t address,size_t n){
    if(address<BASE)return NULL;uint64_t off=(uint64_t)address-BASE;
    return off+n<=RAM_SIZE?ram+(size_t)off:NULL;
}
static uint32_t be32(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static int read_file(const char*path,uint8_t*out,size_t n){FILE*f=fopen(path,"rb");if(!f)return 0;int ok=fread(out,1,n,f)==n;fclose(f);return ok;}
static int original_query(uint8_t*ram,CPUState*c,original_trace*t){
    const uint32_t fmt=c->gpr[4];if(t->query_count>=8)return 0;
    t->query_addresses[t->query_count++]=fmt;
    if(fmt==t->fail_address){c->gpr[3]=(uint32_t)t->fail_status;return 1;}
    if(fmt==0x80266f44u){
        uint32_t ptrs[25];for(unsigned i=0;i<6;++i)ptrs[i]=c->gpr[5+i];
        for(unsigned i=0;i<19;++i){uint8_t*p=ram_ptr(ram,c->gpr[1]+8+i*4,4);if(!p)return 0;ptrs[6+i]=be32(p);}
        uint32_t values[25];fill_values(values);
        for(unsigned i=0;i<25;++i){uint8_t*p=ram_ptr(ram,ptrs[i],4);if(!p)return 0;put32(p,values[i]);}
    }else if(fmt==0x80267118u){
        uint8_t*p=ram_ptr(ram,c->gpr[1]+172,4);if(!p)return 0;put32(p,55);
    }else if(fmt==0x8026713cu){
        uint8_t*a=ram_ptr(ram,c->gpr[5],4),*b=ram_ptr(ram,c->gpr[6],4);if(!a||!b)return 0;
        put32(a,c->gpr[7]+1);put32(b,c->gpr[7]+2);
    }else if(fmt==0x802671a0u){
        for(unsigned i=0;i<6;++i)t->insert_words[i]=c->gpr[5+i];
        for(unsigned i=0;i<18;++i){uint8_t*p=ram_ptr(ram,c->gpr[1]+8+i*4,4);if(!p)return 0;t->insert_words[6+i]=be32(p);}
        t->insert_float=c->fpr[1];
    }else return 0;
    c->gpr[3]=0;return 1;
}
static int run_original(const ModernGekkoModuleDesc*m,const uint8_t*baseline,
    uint32_t mode,original_trace*t,uint32_t fail_address,int32_t fail_status){
    t->fail_address=fail_address;t->fail_status=fail_status;
    uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK_TOP;
    c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=mode;
    c.gpr[4]=OUT_A;c.gpr[5]=OUT_B;c.pc=0x801e51d4u;c.lr=0;
    for(unsigned i=0;i<200000&&c.pc;++i){
        if(c.pc==0x801b4510u){if(!original_query(ram,&c,t)){free(ram);return 0;}c.pc=c.lr;continue;}
        c.downcount=200000000;m->dispatch(&c,c.pc);
        if(c.exception){fprintf(stderr,"original exception %08x at %08x\n",c.exception,c.pc);free(ram);return 0;}
    }
    if(c.pc){free(ram);return 0;}
    uint8_t*a=ram_ptr(ram,OUT_A,4),*b=ram_ptr(ram,OUT_B,4);if(!a||!b){free(ram);return 0;}
    t->outputs[0]=be32(a);t->outputs[1]=be32(b);t->status=c.gpr[3];
    free(ram);return 1;
}
static int check_one(const ModernGekkoModuleDesc*m,const uint8_t*baseline,
    const char*label,uint32_t mode){
    original_trace o={0};native_trace nt={0};nt.fail_kind=-1;
    nflstreet_match_settings_result n;
    if(!run_original(m,baseline,mode,&o,0,0)||
       !nflstreet_match_settings_resolve(mode,native_query,&nt,&n))return 0;
    const uint32_t expected_mode0[5]={0x80266f44u,0x80267118u,0x8026713cu,0x8026713cu,0x802671a0u};
    const uint32_t expected_other[4]={0x80266f44u,0x8026713cu,0x8026713cu,0x802671a0u};
    const unsigned want=mode==0?5:4;
    if(o.query_count!=want||nt.calls!=want)return 0;
    const uint32_t*expected=mode==0?expected_mode0:expected_other;
    for(unsigned i=0;i<o.query_count;++i)if(o.query_addresses[i]!=expected[i])return 0;
    /* 25 original selected fields flow in fixed SQL order; the update uses
       the 24 integer fields and GTYG arrives through the PPC floating ABI. */
    for(unsigned i=0;i<24;++i)if(o.insert_words[i]!=n.session_insert_values[i+1])return 0;
    if(o.insert_float!=n.session_insert_float_argument)return 0;
    if(mode==0){
        if(n.normalized_values[NFLSTREET_SETTING_RTQG]!=55||
           n.normalized_values[NFLSTREET_SETTING_IPHG]!=0x11||
           n.normalized_values[NFLSTREET_SETTING_IPAG]!=0x21)return 0;
    }
    if(o.status!=(uint32_t)n.status||o.outputs[0]!=n.side_context[0]||
       o.outputs[1]!=n.side_context[1])return 0;
    printf("%s mode %u: untouched 801E51D4 queries=%u insert-float=%.8g outputs=%08x/%08x\n",
        label,mode,o.query_count,o.insert_float,o.outputs[0],o.outputs[1]);
    return 1;
}
static int check_error(const ModernGekkoModuleDesc*m,const uint8_t*baseline,
    uint32_t address,nflstreet_match_settings_query kind,unsigned expected_calls,
    int32_t status){
    original_trace o={0};native_trace nt={0};nflstreet_match_settings_result n;
    nt.fail_kind=(int)kind;nt.fail_status=status;
    if(!run_original(m,baseline,0,&o,address,status)||
       !nflstreet_match_settings_resolve(0,native_query,&nt,&n))return 0;
    if(o.status!=(uint32_t)status||n.status!=status||
       o.outputs[0]!=1023||o.outputs[1]!=1023||
       n.side_context[0]!=1023||n.side_context[1]!=1023||
       o.query_count!=expected_calls||nt.calls!=expected_calls||
       n.insert_descriptor_ready)return 0;
    printf("error query %08x: original/native status=%08x outputs=000003ff/000003ff\n",address,(uint32_t)status);
    return 1;
}
int main(int argc,char**argv){
    if(argc!=5){fprintf(stderr,"usage: %s original.dll context14.mem1 context15.mem1 context16.mem1\n",argv[0]);return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();
    if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    if(run_owned_fixtures()){FreeLibrary(h);return 5;}
    const char*labels[3]={"context14","context15","context16"};int ok=1;
    for(unsigned i=0;i<3&&ok;++i){uint8_t*ram=malloc(RAM_SIZE);if(!ram||!read_file(argv[i+2],ram,RAM_SIZE)){free(ram);ok=0;break;}
        for(uint32_t mode=0;mode<3&&ok;++mode)ok=check_one(m,ram,labels[i],mode);
        if(i==0&&ok){
            ok=check_error(m,ram,0x80266f44u,NFLSTREET_SETTING_INITIAL,1,0x39)&&
               check_error(m,ram,0x80267118u,NFLSTREET_SETTING_DEPENDENT_DEFAULT,2,0x39)&&
               check_error(m,ram,0x8026713cu,NFLSTREET_SETTING_SIDE_PAIR,3,0x39)&&
               check_error(m,ram,0x802671a0u,NFLSTREET_SETTING_SESSION_INSERT,5,0x39)&&
               check_error(m,ram,0x80266f44u,NFLSTREET_SETTING_INITIAL,1,-3)&&
               check_error(m,ram,0x80267118u,NFLSTREET_SETTING_DEPENDENT_DEFAULT,2,-3)&&
               check_error(m,ram,0x8026713cu,NFLSTREET_SETTING_SIDE_PAIR,3,-3)&&
               check_error(m,ram,0x802671a0u,NFLSTREET_SETTING_SESSION_INSERT,5,-3);
        }
        free(ram);}
    FreeLibrary(h);
    if(ok)puts("Original 801E51D4 settings differential passed in contexts 14, 15 and 16 with explicit query outputs.");
    return ok?0:6;
}
