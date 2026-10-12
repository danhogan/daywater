#include <windows.h>
#include "moderngekko/module_abi.h"
#include "match_start.h"
#include "match_player_query.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define R13 0x803a9a40u
#define INPUT_A 0x817e0000u
#define INPUT_B 0x817e0040u
#define STACK_TOP 0x81700000u
#define ARRAY_WORDS 14u
#define SIDE_BYTES (ARRAY_WORDS*4u)
#define TRACE_MAX 80u

typedef struct {
    nflstreet_match_start_request request;
    uint32_t output;
    int32_t status;
} trace_entry;
typedef struct {
    trace_entry entries[TRACE_MAX];
    unsigned count;
} trace_log;
typedef struct {
    const uint8_t *database;
    size_t database_size;
    nflstreet_match_start_kind fail_kind;
    uint32_t fail_side, fail_index;
    int32_t fail_status;
    uint8_t initial_mode;
    uint32_t side_context[2];
    int fail_mode_query;
    int32_t mode_status;
    int32_t setup_status[4];
    int32_t side_status[2];
    int32_t set_side_status[2];
    int32_t session_status, update_status, registration_status;
} plan;
typedef struct {
    const plan *config;
    trace_log *trace;
} service_state;
typedef struct {
    uint32_t field_strings[4];
    uint32_t arrays[2][ARRAY_WORDS];
    uint32_t terminal_arrays[2][ARRAY_WORDS];
    uint32_t terminal_mode;
    unsigned terminal_calls, update_calls;
    uint32_t final_status;
    unsigned query_calls[8];
} original_capture;

static uint8_t *ram_ptr(uint8_t*ram,uint32_t address,size_t size){
    if(address<BASE)return NULL;uint64_t off=(uint64_t)address-BASE;
    return off+size<=RAM_SIZE?ram+(size_t)off:NULL;
}
static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *load_file(const char*path,size_t*size){
    FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
    long n=ftell(f);if(n<1||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t*b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return b;
}
static int parse_unsigned_after(const char*text,const char*key,unsigned long*out){
    const char*p=strstr(text,key);if(!p)return 0;p+=strlen(key);
    while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n'||*p==':')++p;
    char*end=NULL;unsigned long value=strtoul(p,&end,10);if(end==p)return 0;*out=value;return 1;
}
static int load_formations(const char*path,uint32_t out[2][ARRAY_WORDS]){
    size_t n=0;char*text=(char*)load_file(path,&n);if(!text||n>65536u){free(text);return 0;}
    char*grown=realloc(text,n+1);if(!grown){free(text);return 0;}text=grown;text[n]=0;
    unsigned long slot,key;const char*p=strstr(text,"\"formation_players\"");
    if(!p||!parse_unsigned_after(text,"\"field_slot\"",&slot)||
       !parse_unsigned_after(text,"\"field_key\"",&key)||slot>=8||key>=8){free(text);return 0;}
    p=strchr(p,'[');if(!p){free(text);return 0;}++p;
    for(unsigned i=0;i<2*ARRAY_WORDS;++i){while(*p&&(*p<'0'||*p>'9'))++p;if(!*p){free(text);return 0;}
        char*end=NULL;unsigned long v=strtoul(p,&end,10);if(end==p||v>UINT32_MAX){free(text);return 0;}
        out[i/ARRAY_WORDS][i%ARRAY_WORDS]=(uint32_t)v;p=end;
    }
    free(text);return 1;
}
static int32_t transform_word(uint32_t side,uint32_t index,uint32_t value,uint32_t*out){
    /* Controlled test mapping: proves the original per-player output pointer
       reaches each slot. This is not a claim about production database values. */
    *out=value^(0x5a000000u|((side&1u)<<16)|index);return 0;
}
static int32_t service(void*opaque,const nflstreet_match_start_request*r,uint32_t*out){
    service_state*s=opaque;if(!s||!s->config||!s->trace||!r||!out||s->trace->count>=TRACE_MAX)return 0x7fffffff;
    trace_entry*e=&s->trace->entries[s->trace->count++];e->request=*r;e->request.formations=NULL;
    int32_t status=0;const plan*p=s->config;
    switch(r->kind){
    case NFLSTREET_MATCH_REGISTER:status=p->registration_status;break;
    case NFLSTREET_MATCH_QUERY_MODE:
        *out=p->initial_mode;status=p->mode_status;break;
    case NFLSTREET_MATCH_SETUP_A:status=p->setup_status[0];break;
    case NFLSTREET_MATCH_SETUP_B:status=p->setup_status[1];break;
    case NFLSTREET_MATCH_SETUP_PAIR:status=p->setup_status[2];break;
    case NFLSTREET_MATCH_SETUP_FINAL:status=p->setup_status[3];break;
    case NFLSTREET_MATCH_QUERY_SIDE:
        if(r->side>1){status=0x7fffffff;break;}*out=p->side_context[r->side];status=p->side_status[r->side];break;
    case NFLSTREET_MATCH_SET_SIDE_A:case NFLSTREET_MATCH_SET_SIDE_B:
        if(r->side>1){status=0x7fffffff;break;}status=p->set_side_status[r->side];break;
    case NFLSTREET_MATCH_QUERY_PLAYER:
        if(r->side>1||r->index>=ARRAY_WORDS){status=0x7fffffff;break;}
        if(p->database){
            if(!nflstreet_match_player_query(p->database,p->database_size,r->context,r->value,out)){fprintf(stderr,"actual database lookup rejected\n");exit(6);}
        }else status=transform_word(r->side,r->index,r->value,out);
        if(p->fail_kind==r->kind&&p->fail_side==r->side&&p->fail_index==r->index)status=p->fail_status;
        break;
    case NFLSTREET_MATCH_SESSION:status=p->session_status;break;
    case NFLSTREET_MATCH_UPDATE:status=p->update_status;break;
    default:status=0x7fffffff;break;
    }
    e->output=(r->kind==NFLSTREET_MATCH_QUERY_MODE||r->kind==NFLSTREET_MATCH_QUERY_SIDE||r->kind==NFLSTREET_MATCH_QUERY_PLAYER)?*out:0;e->status=status;return status;
}
static int trace_equal(const trace_log*a,const trace_log*b){
    if(a->count!=b->count)return 0;
    for(unsigned i=0;i<a->count;++i){const trace_entry*x=&a->entries[i],*y=&b->entries[i];
        const nflstreet_match_start_request*u=&x->request,*v=&y->request;
        if(u->kind!=v->kind||u->side!=v->side||u->index!=v->index||u->value!=v->value||
           u->context!=v->context||u->argument0!=v->argument0||u->argument1!=v->argument1||
           x->output!=y->output||x->status!=y->status){
            fprintf(stderr,"trace mismatch[%u] kind%u/%u side%u/%u index%u/%u value%08x/%08x context%08x/%08x args%08x,%08x/%08x,%08x out%08x/%08x status%08x/%08x\n",
              i,u->kind,v->kind,u->side,v->side,u->index,v->index,u->value,v->value,u->context,v->context,
              u->argument0,u->argument1,v->argument0,v->argument1,x->output,y->output,(uint32_t)x->status,(uint32_t)y->status);return 0;}}
    return 1;
}
static int original_query(CPUState*c,uint8_t*ram,const plan*p,service_state*s,original_capture*cap){
    (void)p;
    uint32_t fmt=c->gpr[4];nflstreet_match_start_request r={0};uint32_t out=0;int32_t status=0;
    if(fmt==0x80268b74u){
        r.kind=NFLSTREET_MATCH_QUERY_MODE;++cap->query_calls[0];
        uint8_t*dst=ram_ptr(ram,c->gpr[5],1);if(!dst)return 0;out=*dst;
        status=service(s,&r,&out);*dst=(uint8_t)out;
    }else if(fmt==0x80268b98u){
        if(c->gpr[6]>1)return 0;r.kind=NFLSTREET_MATCH_QUERY_SIDE;r.side=c->gpr[6];r.argument0=c->gpr[6];
        ++cap->query_calls[1];uint8_t*dst=ram_ptr(ram,c->gpr[5],4);if(!dst)return 0;out=be32(dst);
        status=service(s,&r,&out);put32(dst,out);
    }else if(fmt==0x80266c80u||fmt==0x80266ca0u){
        r.kind=fmt==0x80266c80u?NFLSTREET_MATCH_SET_SIDE_A:NFLSTREET_MATCH_SET_SIDE_B;
        r.side=fmt==0x80266c80u?0:1;r.value=c->gpr[5];r.argument0=c->gpr[5];
        ++cap->query_calls[2];status=service(s,&r,&out);
    }else if(fmt==0x80268bd0u){
        uint32_t side,idx;if(c->gpr[5]>=INPUT_A&&c->gpr[5]<INPUT_A+SIDE_BYTES){side=0;idx=(c->gpr[5]-INPUT_A)/4;}
        else if(c->gpr[5]>=INPUT_B&&c->gpr[5]<INPUT_B+SIDE_BYTES){side=1;idx=(c->gpr[5]-INPUT_B)/4;}
        else return 0;
        uint8_t*dst=ram_ptr(ram,c->gpr[5],4);if(!dst||idx>=ARRAY_WORDS||be32(dst)!=c->gpr[6])return 0;
        r.kind=NFLSTREET_MATCH_QUERY_PLAYER;r.side=side;r.index=idx;r.value=c->gpr[6];r.context=c->gpr[7];
        ++cap->query_calls[3];out=be32(dst);status=service(s,&r,&out);put32(dst,out);
    }else if(fmt==0x80268b00u){
        r.kind=NFLSTREET_MATCH_UPDATE;r.value=c->gpr[5];r.argument0=c->gpr[5];
        ++cap->query_calls[4];status=service(s,&r,&out);++cap->update_calls;
    }else if(fmt==0x80266c80u||fmt==0x80266ca0u){
        return 0;
    }else{
        fprintf(stderr,"unsupported B4510 query format %08x\n",fmt);return 0;
    }
    c->gpr[3]=(uint32_t)status;return 1;
}
static int run_original(const ModernGekkoModuleDesc*m,uint8_t*ram,const uint32_t input[2][ARRAY_WORDS],
    const plan*p,trace_log*trace,original_capture*cap,uint32_t*result){
    service_state ss={p,trace};memset(cap,0,sizeof *cap);
    uint8_t*a=ram_ptr(ram,INPUT_A,2*SIDE_BYTES);uint8_t*b=ram_ptr(ram,INPUT_B,2*SIDE_BYTES);(void)b;
    if(!a)return 0;for(unsigned side=0;side<2;++side)for(unsigned i=0;i<ARRAY_WORDS;++i)
        put32(ram_ptr(ram,side?INPUT_B:INPUT_A,SIDE_BYTES)+i*4,input[side][i]);
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
    c.gpr[1]=STACK_TOP;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
    c.gpr[3]=INPUT_A;c.gpr[4]=INPUT_B;c.pc=0x801ea480u;c.lr=0;c.downcount=c.cycle_budget=200000000u;
    for(unsigned step=0;c.pc&&step<200000;++step){
        switch(c.pc){
        case 0x801e071cu:{
            nflstreet_match_start_request r={0};r.kind=NFLSTREET_MATCH_REGISTER;uint32_t o=0;
            if(service(&ss,&r,&o)){}m->dispatch(&c,c.pc);break;}
        case 0x801b4510u:
            if(!original_query(&c,ram,p,&ss,cap))return 0;c.pc=c.lr;break;
        case 0x801e3828u:{nflstreet_match_start_request r={0};r.kind=NFLSTREET_MATCH_SETUP_A;r.argument0=c.gpr[3];uint32_t o=0;c.gpr[3]=(uint32_t)service(&ss,&r,&o);c.pc=c.lr;break;}
        case 0x801e3720u:{nflstreet_match_start_request r={0};r.kind=NFLSTREET_MATCH_SETUP_B;r.argument0=c.gpr[3];uint32_t o=0;c.gpr[3]=(uint32_t)service(&ss,&r,&o);c.pc=c.lr;break;}
        case 0x801e05a4u:{nflstreet_match_start_request r={0};r.kind=NFLSTREET_MATCH_SETUP_PAIR;r.argument0=c.gpr[3];r.argument1=c.gpr[4];uint32_t o=0;c.gpr[3]=(uint32_t)service(&ss,&r,&o);c.pc=c.lr;break;}
        case 0x801e5eccu:{nflstreet_match_start_request r={0};r.kind=NFLSTREET_MATCH_SETUP_FINAL;uint32_t o=0;c.gpr[3]=(uint32_t)service(&ss,&r,&o);c.pc=c.lr;break;}
        case 0x801e577cu:{
            nflstreet_match_start_request r={0};r.kind=NFLSTREET_MATCH_SESSION;r.value=c.gpr[3];r.argument0=c.gpr[3];r.argument1=c.gpr[4];
            r.formations=NULL;uint32_t o=0;cap->terminal_mode=c.gpr[3];
            for(unsigned side=0;side<2;++side){uint8_t*src=ram_ptr(ram,c.gpr[6+side],SIDE_BYTES);if(!src)return 0;
                for(unsigned i=0;i<ARRAY_WORDS;++i)cap->terminal_arrays[side][i]=be32(src+i*4);}
            ++cap->terminal_calls;c.gpr[3]=(uint32_t)service(&ss,&r,&o);c.pc=c.lr;break;}
        default:m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"original match-start exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}
        }
    }
    if(c.pc){fprintf(stderr,"original match-start stopped pc=%08x\n",c.pc);return 0;}
    *result=c.gpr[3];
    for(unsigned side=0;side<2;++side)for(unsigned i=0;i<ARRAY_WORDS;++i)
        cap->arrays[side][i]=be32(ram_ptr(ram,side?INPUT_B:INPUT_A,SIDE_BYTES)+i*4);
    cap->final_status=*result;return 1;
}

static int run_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,
    const uint32_t input[2][ARRAY_WORDS],const plan*p,const char*label){
    uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    trace_log original_trace={0},native_trace={0};original_capture cap={0};uint32_t original_status=0;
    service_state ns={p,&native_trace};nflstreet_match_start_result native={0};
    if(!run_original(m,ram,input,p,&original_trace,&cap,&original_status)||
       !nflstreet_match_start_run(input,service,&ns,&native)){fprintf(stderr,"%s execution failed\n",label);free(ram);return 0;}
    int ok=trace_equal(&original_trace,&native_trace)&&original_status==(uint32_t)native.status&&
       !memcmp(cap.arrays,native.formations,sizeof cap.arrays)&&
       cap.terminal_calls==native.session_requested&&cap.update_calls==native.update_requested;
    if(cap.terminal_calls&&memcmp(cap.terminal_arrays,native.formations,sizeof cap.terminal_arrays))ok=0;
    uint32_t expected_context[2]={0};
    for(unsigned i=0;i<original_trace.count;++i)
        if(original_trace.entries[i].request.kind==NFLSTREET_MATCH_QUERY_SIDE)
            expected_context[original_trace.entries[i].request.side]=original_trace.entries[i].output;
    if(memcmp(native.side_context,expected_context,sizeof expected_context))ok=0;
    if(!ok){fprintf(stderr,"%s mismatch original=%08x native=%08x calls=%u/%u terminal=%u/%u updates=%u/%u\n",
        label,original_status,(uint32_t)native.status,original_trace.count,native_trace.count,
        cap.terminal_calls,native.session_requested,cap.update_calls,native.update_requested);
        free(ram);return 0;}
    printf("%s: original/native status=%08x trace=%u player-queries=%u session=%u update=%u; two output arrays exact.\n",
        label,original_status,original_trace.count,cap.query_calls[3],cap.terminal_calls,cap.update_calls);
    free(ram);return 1;
}

static int run_tests(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const uint32_t input[2][ARRAY_WORDS]){
    plan p={0};p.side_context[0]=0x11223344u;p.side_context[1]=0x55667788u;
    if(!run_case(m,baseline,input,&p,"success"))return 0;
    p.initial_mode=1;if(!run_case(m,baseline,input,&p,"nonzero-mode-bypass"))return 0;p.initial_mode=0;
    p.initial_mode=255;if(!run_case(m,baseline,input,&p,"mode255-bypass"))return 0;p.initial_mode=0;
    p.mode_status=0x31;if(!run_case(m,baseline,input,&p,"initial-query-error"))return 0;p.mode_status=0;
    for(unsigned i=0;i<4;++i){p.setup_status[i]=0x40+(int32_t)i;
        if(!run_case(m,baseline,input,&p,"setup-error"))return 0;p.setup_status[i]=0;}
    for(unsigned side=0;side<2;++side){p.side_status[side]=0x50+(int32_t)side;
        if(!run_case(m,baseline,input,&p,"side-query-error"))return 0;p.side_status[side]=0;}
    for(unsigned side=0;side<2;++side){p.set_side_status[side]=0x60+(int32_t)side;
        if(!run_case(m,baseline,input,&p,"side-set-error"))return 0;p.set_side_status[side]=0;}
    p.session_status=0x71;if(!run_case(m,baseline,input,&p,"session-boundary-error"))return 0;p.session_status=0;
    p.update_status=0x72;if(!run_case(m,baseline,input,&p,"final-update-error"))return 0;p.update_status=0;
    p.fail_kind=NFLSTREET_MATCH_QUERY_PLAYER;p.fail_side=0;p.fail_index=0;p.fail_status=0x81;
    if(!run_case(m,baseline,input,&p,"ignored-player-errors-side0"))return 0;
    p.fail_side=1;p.fail_index=13;
    if(!run_case(m,baseline,input,&p,"ignored-player-errors-side1"))return 0;
    return 1;
}

int main(int argc,char**argv){
    if(argc!=4&&argc!=5){fprintf(stderr,"Usage: match-start-original-check.exe <original-DLL> <saved-RAM> <native-selection.json> [team-DB]\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();
    if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    size_t n=0;uint8_t*baseline=load_file(argv[2],&n);if(!baseline||n!=RAM_SIZE){free(baseline);FreeLibrary(h);return 4;}
    uint32_t input[2][ARRAY_WORDS];if(!load_formations(argv[3],input)){fprintf(stderr,"invalid native selection snapshot\n");free(baseline);FreeLibrary(h);return 4;}
    int ok=run_tests(m,baseline,input);
    if(ok&&argc==5){
        size_t db_size=0;uint8_t*db=load_file(argv[4],&db_size);
        if(!db)ok=0;
        else{plan actual={0};actual.database=db;actual.database_size=db_size;
            actual.side_context[0]=actual.side_context[1]=1;
            ok=run_case(m,baseline,input,&actual,"actual-DB-player-resolution");free(db);}
    }
    free(baseline);FreeLibrary(h);
    if(ok)puts("Bounded original801EA480 differential passed. Query, setup and terminal services were supplied/captured; session creation was not executed.");
    return ok?0:5;
}
