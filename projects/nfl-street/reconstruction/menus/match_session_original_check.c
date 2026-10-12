/* Offline differential: original orchestration only. Every downstream owner is
   explicitly supplied. These checks do not create actors or a playable match. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "match_session.h"
#include "match_start.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define R13 0x803a9a40u
#define STACK 0x81700000u
#define ARRAY_A 0x817e0000u
#define ARRAY_B 0x817e0040u
#define ACCEPT 0x817e0100u
#define PRE_CALLBACK 0x817f1000u
#define POST_CALLBACK 0x817f2000u
#define TRACE_MAX 96
/* Entries use original service addresses and explicit argument words, so a
   native enum mismatch cannot silently normalize away an argument mismatch. */
typedef struct { uint32_t pc, arg[5], output[3]; int32_t status; } entry;
typedef struct { entry rows[TRACE_MAX]; unsigned count; } trace;
typedef struct {
    uint8_t initialized, pre, post, cancel;
    uint32_t fail_pc; int32_t fail_status;
    uint32_t sides[2];
    int joined;
} plan;
typedef struct { const plan *p; trace *t; uint32_t arrays[2][14]; } env;
static uint8_t *ptr(uint8_t *ram,uint32_t a,size_t n){
    if(a<BASE||(uint64_t)a-BASE+n>RAM_SIZE)return NULL;return ram+a-BASE;
}
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *load(const char *path,size_t *size){
    FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
    long n=ftell(f);if(n<1||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t*b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return b;
}
static entry *record(env*e,uint32_t pc,const uint32_t args[5]){
    if(e->t->count>=TRACE_MAX){fprintf(stderr,"trace overflow\n");exit(8);}
    entry*r=&e->t->rows[e->t->count++];memset(r,0,sizeof *r);r->pc=pc;
    memcpy(r->arg,args,sizeof r->arg);r->status=e->p->fail_pc==pc?e->p->fail_status:0;return r;
}
static int session_pc(uint32_t pc){switch(pc){
    case 0x801e51d4u:case PRE_CALLBACK:case POST_CALLBACK:case 0x802675d0u:case 0x802675e4u:
    case 0x801e5628u:case 0x801e5698u:case 0x801e4d4cu:case 0x801e333cu:
    case 0x801e0e7cu:case 0x801e3b2cu:case 0x801e4b6cu:return 1;default:return 0;}}
static int original_service(CPUState*c,uint8_t*ram,env*e){
    uint32_t pc=c->pc,args[5]={0};
    if(pc==0x801b4510u)pc=c->gpr[4];
    if(!session_pc(pc))return 0;
    if(pc==0x801e51d4u)args[0]=c->gpr[3];
    else if(pc==0x802675d0u||pc==0x802675e4u){if(c->gpr[3])return -1;}
    else if(pc==0x801e4d4cu){
        args[0]=c->gpr[3];args[1]=c->gpr[5];args[4]=c->gpr[7];
        for(unsigned side=0;side<2;++side){uint8_t*a=ptr(ram,c->gpr[side?6:4],56);if(!a)return -1;
            for(unsigned i=0;i<14;++i)e->arrays[side][i]=be(a+4*i);}
    }else {args[0]=c->gpr[3];args[1]=c->gpr[4];
        if(pc!=PRE_CALLBACK&&pc!=0x801e5628u&&pc!=0x801e5698u){args[2]=c->gpr[5];args[3]=c->gpr[6];}}
    entry*r=record(e,pc,args);
    if(pc==0x801e51d4u){uint8_t*a=ptr(ram,c->gpr[4],4),*b=ptr(ram,c->gpr[5],4);if(!a||!b)return -1;
        put(a,e->p->sides[0]);put(b,e->p->sides[1]);r->output[0]=e->p->sides[0];r->output[1]=e->p->sides[1];}
    if(pc==PRE_CALLBACK){uint8_t*a=ptr(ram,c->gpr[5],1);if(!a||*a!=1)return -1;*a=e->p->cancel;r->output[2]=*a;}
    c->gpr[3]=(uint32_t)r->status;c->pc=c->lr;return 1;
}
static int outer_original(CPUState*c,uint8_t*ram,env*e){
    uint32_t pc=c->pc,args[5]={0};if(pc==0x801b4510u)pc=c->gpr[4];
    switch(pc){
    case 0x801e071cu:record(e,pc,args);return 0;
    case 0x80268b74u:{entry*r=record(e,pc,args);uint8_t*a=ptr(ram,c->gpr[5],1);if(!a)return -1;*a=0;c->gpr[3]=(uint32_t)r->status;break;}
    case 0x80268b98u:{args[0]=c->gpr[6];entry*r=record(e,pc,args);uint8_t*a=ptr(ram,c->gpr[5],4);if(!a)return -1;
        r->output[0]=0x12340000u+c->gpr[6];put(a,r->output[0]);c->gpr[3]=(uint32_t)r->status;break;}
    case 0x80268bd0u:{args[0]=c->gpr[6];args[1]=c->gpr[7];entry*r=record(e,pc,args);uint8_t*a=ptr(ram,c->gpr[5],4);if(!a)return -1;
        r->output[0]=c->gpr[6]^0x00550000u;put(a,r->output[0]);c->gpr[3]=(uint32_t)r->status;break;}
    case 0x80266c80u:case 0x80266ca0u:case 0x80268b00u:args[0]=c->gpr[5];c->gpr[3]=(uint32_t)record(e,pc,args)->status;break;
    case 0x801e3828u:case 0x801e3720u:args[0]=c->gpr[3];c->gpr[3]=(uint32_t)record(e,pc,args)->status;break;
    case 0x801e05a4u:args[0]=c->gpr[3];args[1]=c->gpr[4];c->gpr[3]=(uint32_t)record(e,pc,args)->status;break;
    case 0x801e5eccu:c->gpr[3]=(uint32_t)record(e,pc,args)->status;break;
    default:return 0;}
    c->pc=c->lr;return 1;
}
static int32_t native_service(void*opaque,const nflstreet_match_session_request*q,nflstreet_match_session_response*out){
    env*e=opaque;static const uint32_t pcs[]={0x801e51d4u,PRE_CALLBACK,0x802675d0u,0x802675e4u,0x801e5628u,0x801e5698u,0x801e4d4cu,POST_CALLBACK,0x801e333cu,0x801e0e7cu,0x801e3b2cu,0x801e4b6cu};
    if((unsigned)q->kind>=sizeof pcs/sizeof pcs[0])exit(8);
    uint32_t pc=pcs[q->kind],args[5]={0};
    if(q->kind==NFLSTREET_SESSION_RESOLVE)args[0]=q->argument;
    else if(q->kind!=NFLSTREET_SESSION_DELETE_A&&q->kind!=NFLSTREET_SESSION_DELETE_B){
        args[0]=q->sides[0];args[1]=q->sides[1];
        if(q->kind==NFLSTREET_SESSION_ASSIGN_ROSTERS){args[4]=q->table_tag;memcpy(e->arrays,q->formations,sizeof e->arrays);}
        else if(q->kind>=NFLSTREET_SESSION_BEGIN_CALLBACK){args[2]=q->mode;args[3]=q->argument;}}
    entry*r=record(e,pc,args);
    if(q->kind==NFLSTREET_SESSION_RESOLVE){out->sides[0]=e->p->sides[0];out->sides[1]=e->p->sides[1];r->output[0]=out->sides[0];r->output[1]=out->sides[1];}
    if(q->kind==NFLSTREET_SESSION_CHECK_CALLBACK){if(out->continuation!=1)exit(8);out->continuation=e->p->cancel;r->output[2]=out->continuation;}
    return r->status;
}
typedef struct { env *e; nflstreet_match_session_owner *owner; nflstreet_match_session_result *session; } joined_env;
static int32_t native_outer(void*opaque,const nflstreet_match_start_request*q,uint32_t*out){
    joined_env*j=opaque;env*e=j->e;uint32_t pc=0,args[5]={0};
    switch(q->kind){
    case NFLSTREET_MATCH_REGISTER:pc=0x801e071cu;break;
    case NFLSTREET_MATCH_QUERY_MODE:pc=0x80268b74u;*out=0;break;
    case NFLSTREET_MATCH_SETUP_A:pc=0x801e3828u;args[0]=q->argument0;break;
    case NFLSTREET_MATCH_SETUP_B:pc=0x801e3720u;args[0]=q->argument0;break;
    case NFLSTREET_MATCH_SETUP_PAIR:pc=0x801e05a4u;args[0]=q->argument0;args[1]=q->argument1;break;
    case NFLSTREET_MATCH_SETUP_FINAL:pc=0x801e5eccu;break;
    case NFLSTREET_MATCH_QUERY_SIDE:pc=0x80268b98u;args[0]=q->side;*out=0x12340000u+q->side;break;
    case NFLSTREET_MATCH_SET_SIDE_A:pc=0x80266c80u;args[0]=q->value;break;
    case NFLSTREET_MATCH_SET_SIDE_B:pc=0x80266ca0u;args[0]=q->value;break;
    case NFLSTREET_MATCH_QUERY_PLAYER:pc=0x80268bd0u;args[0]=q->value;args[1]=q->context;*out=q->value^0x00550000u;break;
    case NFLSTREET_MATCH_UPDATE:pc=0x80268b00u;args[0]=q->value;break;
    case NFLSTREET_MATCH_SESSION:
        if(!nflstreet_match_session_run(q->value,q->argument1,q->formations,j->owner,native_service,e,j->session))exit(8);
        return j->session->status;
    default:exit(8);}
    entry*r=record(e,pc,args);
    if(q->kind==NFLSTREET_MATCH_QUERY_SIDE||q->kind==NFLSTREET_MATCH_QUERY_PLAYER)r->output[0]=*out;
    return r->status;
}
static int original(const ModernGekkoModuleDesc*m,uint8_t*ram,env*e,uint32_t *status,uint8_t *accepted,uint8_t *initialized){
    *ptr(ram,R13-0x62a8u,1)=e->p->initialized;
    put(ptr(ram,R13-0x62b4u,4),e->p->pre?PRE_CALLBACK:0);
    put(ptr(ram,R13-0x62b0u,4),e->p->post?POST_CALLBACK:0);
    *ptr(ram,ACCEPT,1)=0xcd;
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
    c.gpr[3]=0;c.gpr[4]=0x13579bdfu;c.gpr[5]=ACCEPT;c.gpr[6]=ARRAY_A;c.gpr[7]=ARRAY_B;
    c.pc=0x801e577cu;
    if(e->p->joined){c.pc=0x801ea480u;c.gpr[3]=ARRAY_A;c.gpr[4]=ARRAY_B;}
    c.downcount=c.cycle_budget=200000000u;
    uint32_t accepted_address=ACCEPT;
    for(unsigned step=0;c.pc&&step<200000;++step){
        if(c.pc==0x801e577cu&&e->p->joined)accepted_address=c.gpr[5];
        int handled=original_service(&c,ram,e);if(handled<0)return 0;if(handled)continue;
        if(e->p->joined){handled=outer_original(&c,ram,e);if(handled<0)return 0;if(handled)continue;}
        c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}
    }
    if(c.pc){fprintf(stderr,"budget pc=%08x\n",c.pc);return 0;}
    *status=c.gpr[3];*accepted=*ptr(ram,accepted_address,1);*initialized=*ptr(ram,R13-0x62a8u,1);return 1;
}
static int compare(const trace*a,const trace*b){
    if(a->count!=b->count){fprintf(stderr,"count %u/%u\n",a->count,b->count);for(unsigned j=0;j<a->count;++j)fprintf(stderr,"original %u %08x\n",j,a->rows[j].pc);return 0;}
    for(unsigned i=0;i<a->count;++i)if(memcmp(&a->rows[i],&b->rows[i],sizeof(entry))){
        const entry*x=&a->rows[i],*y=&b->rows[i];fprintf(stderr,"row%u pc=%08x/%08x args=%08x,%08x,%08x,%08x,%08x / %08x,%08x,%08x,%08x,%08x\n",i,x->pc,y->pc,x->arg[0],x->arg[1],x->arg[2],x->arg[3],x->arg[4],y->arg[0],y->arg[1],y->arg[2],y->arg[3],y->arg[4]);return 0;}
    return 1;
}
static int run_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const plan*p,const char*label){
    uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    uint32_t formations[2][14];for(unsigned side=0;side<2;++side)for(unsigned i=0;i<14;++i){
        formations[side][i]=0xa1000000u+side*0x10000u+i;put(ptr(ram,side?ARRAY_B:ARRAY_A,56)+4*i,formations[side][i]);}
    trace ot={0},nt={0};env oe={p,&ot,{{0}}},ne={p,&nt,{{0}}};uint32_t os=0;uint8_t oa=0,oi=0;
    nflstreet_match_session_owner owner={p->initialized,p->pre,p->post};nflstreet_match_session_result nr={0};uint32_t ns=0;
    if(!original(m,ram,&oe,&os,&oa,&oi)){fprintf(stderr,"%s original failed\n",label);free(ram);return 0;}
    if(p->joined){joined_env je={&ne,&owner,&nr};nflstreet_match_start_result outer={0};
        if(!nflstreet_match_start_run(formations,native_outer,&je,&outer)){free(ram);return 0;}ns=(uint32_t)outer.status;
        for(unsigned side=0;side<2;++side)for(unsigned i=0;i<14;++i)if(be(ptr(ram,side?ARRAY_B:ARRAY_A,56)+i*4)!=outer.formations[side][i]){free(ram);return 0;}
    }else {if(!nflstreet_match_session_run(0,0x13579bdfu,formations,&owner,native_service,&ne,&nr)){free(ram);return 0;}ns=(uint32_t)nr.status;}
    unsigned assigned=0,completed=0;for(unsigned i=0;i<ot.count;++i){
        if(ot.rows[i].pc==0x801e4d4cu)assigned++;
        if((ot.rows[i].pc==0x801e333cu||ot.rows[i].pc==0x801e0e7cu||ot.rows[i].pc==0x801e3b2cu||ot.rows[i].pc==0x801e4b6cu)&&!ot.rows[i].status)completed++;}
    int ok=compare(&ot,&nt)&&os==ns&&oa==nr.continuation&&oi==owner.initialized&&assigned==nr.rosters_assigned&&completed==nr.stages_completed&&
        !memcmp(oe.arrays,ne.arrays,sizeof oe.arrays)&&!memcmp(nr.sides,p->sides,sizeof nr.sides);
    if(!ok)fprintf(stderr,"%s mismatch status=%08x/%08x continuation=%u/%u initialized=%u/%u assigned=%u/%u stages=%u/%u\n",label,os,ns,oa,nr.continuation,oi,owner.initialized,assigned,nr.rosters_assigned,completed,nr.stages_completed);
    else printf("%s: exact status=%08x continuation=%u initialized=%u trace=%u assigned=%u stages=%u joined=%u\n",label,os,oa,oi,ot.count,assigned,completed,p->joined);
    free(ram);return ok;
}
static int tests(const ModernGekkoModuleDesc*m,const uint8_t*baseline){
    plan p={0};p.sides[0]=0x11223344u;p.sides[1]=0x55667788u;p.cancel=1;
    if(!run_case(m,baseline,&p,"initial-success"))return 0;
    p.pre=p.post=1;
    const uint8_t flags[]={0,1,2,255};for(unsigned i=0;i<sizeof flags;++i){char label[40];snprintf(label,sizeof label,"callback-flag-%u",flags[i]);p.initialized=flags[i];if(!run_case(m,baseline,&p,label))return 0;}
    p.initialized=1;
    const uint8_t cancellations[]={0,2,255};for(unsigned i=0;i<sizeof cancellations;++i){char label[40];snprintf(label,sizeof label,"check-cancel-%u",cancellations[i]);p.cancel=cancellations[i];if(!run_case(m,baseline,&p,label))return 0;}p.cancel=1;
    const uint32_t failures[]={0x801e51d4u,PRE_CALLBACK,0x802675d0u,0x802675e4u,0x801e5628u,0x801e5698u,0x801e4d4cu,POST_CALLBACK,0x801e333cu,0x801e0e7cu,0x801e3b2cu,0x801e4b6cu};
    for(unsigned i=0;i<sizeof failures/sizeof failures[0];++i){char label[40];snprintf(label,sizeof label,"service-error-%08x",failures[i]);p.fail_pc=failures[i];p.fail_status=(int32_t)(0x90u+i);if(!run_case(m,baseline,&p,label))return 0;}
    p.initialized=0;p.pre=0;for(unsigned i=0;i<sizeof failures/sizeof failures[0];++i){if(failures[i]==PRE_CALLBACK)continue;char label[48];snprintf(label,sizeof label,"fresh-service-error-%08x",failures[i]);p.fail_pc=failures[i];p.fail_status=(int32_t)(0xb0u+i);if(!run_case(m,baseline,&p,label))return 0;}
    p.initialized=1;p.fail_pc=0;p.fail_status=0;p.pre=0;if(!run_case(m,baseline,&p,"begin-only"))return 0;
    p.pre=1;p.post=0;if(!run_case(m,baseline,&p,"check-only"))return 0;
    p.post=1;p.joined=1;if(!run_case(m,baseline,&p,"joined-success"))return 0;
    p.cancel=0;if(!run_case(m,baseline,&p,"joined-cancel-update"))return 0;p.cancel=1;
    p.fail_pc=0x801e5628u;p.fail_status=0xa1;if(!run_case(m,baseline,&p,"joined-prepare-error"))return 0;
    p.fail_pc=0x801e0e7cu;p.fail_status=0xa2;if(!run_case(m,baseline,&p,"joined-stage-error"))return 0;
    p.fail_pc=0x80268b00u;p.fail_status=0xa3;if(!run_case(m,baseline,&p,"joined-update-error"))return 0;
    p.joined=0;p.fail_pc=0x801e333cu;p.fail_status=-1;if(!run_case(m,baseline,&p,"negative-stage-status"))return 0;
    return 1;
}
int main(int argc,char**argv){
    if(argc!=3){fprintf(stderr,"Usage: match-session-original-check.exe <originalDLL> <savedRAM>\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    size_t size=0;uint8_t*baseline=load(argv[2],&size);if(!baseline||size!=RAM_SIZE){free(baseline);FreeLibrary(h);return 4;}
    int ok=tests(m,baseline);free(baseline);FreeLibrary(h);
    if(ok)puts("Bounded original801E577C and joined801EA480 differential passed; downstream services supplied, no actors or playable match created.");
    return ok?0:5;
}
