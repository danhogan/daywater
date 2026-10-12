#include "focus.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 0x81100000u
#define STACK_BASE 0x70010000u
#define CURRENT_SCREEN 7u
#define LINKED_SCREEN 5u

static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint32_t get32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t *at(uint8_t*b,uint32_t a){return a<BASE||a-BASE>=0x600?NULL:b+a-BASE;}
typedef struct {unsigned calls,fail_at;uint32_t kinds[32],properties[32],targets[32],sources[32],links[32],stack_bases[32];} probe;

static int memory_service(void *ctx,uint8_t *screen,size_t size,uint32_t base,
                          const nflstreet_ui_service_request*q,uint32_t*out,
                          uint32_t stack_base,uint32_t words[256]){
    probe *p=ctx;(void)screen;(void)size;(void)base;
    unsigned i=p->calls++;
    if(i>=32)return 0;
    p->kinds[i]=q->kind;p->properties[i]=q->property;p->targets[i]=q->target;
    p->sources[i]=q->source;p->links[i]=q->link;p->stack_bases[i]=stack_base;
    if(p->fail_at&&p->calls==p->fail_at)return 0;
    if(q->kind==9){if(q->source!=CURRENT_SCREEN||q->link!=CURRENT_SCREEN||q->target!=CURRENT_SCREEN)return 0;}
    else if(q->kind==8){if(q->property!=3||q->target!=LINKED_SCREEN)return 0;}
    else if(q->kind==1){
        if(q->argc!=1||q->args[0]!=stack_base+200*4||stack_base!=STACK_BASE)return 0;
        if(q->property==0x111){words[200]=0x40000111u;}
        else if(q->property==0x222){words[200]=0x40000222u;}
        else if(q->property!=0x40000111u&&q->property!=0x40000222u)return 0;
    }else return 0;
    *out=q->property;return 1;
}

static void append_push(uint8_t*b,size_t*off,uint32_t value){b[(*off)++]=0x10;put32(b+*off,value);*off+=4;}
static void make_script(uint8_t*b,size_t off,uint32_t property){
    uint32_t p=BASE+(uint32_t)off;size_t k=off;
    append_push(b,&k,CURRENT_SCREEN<<16);b[k++]=0x02;b[k++]=0;
    b[k++]=0x48;
    append_push(b,&k,property);append_push(b,&k,STACK_BASE+200*4);append_push(b,&k,1);b[k++]=0x0b;
    append_push(b,&k,STACK_BASE+200*4);b[k++]=0x18;
    append_push(b,&k,STACK_BASE+200*4);append_push(b,&k,1);b[k++]=0x0b;
    append_push(b,&k,0);b[k++]=0x44;
    (void)p;
}

static int fixture(uint8_t*b,size_t size){
    if(size<0x500)return 0;
    memset(b,0,size);put32(b,3);put32(b+4,BASE+32);
    uint8_t*nodes=at(b,BASE+32);if(!nodes)return 0;
    const uint32_t roots[3]={BASE+0x200,BASE+0x280,BASE+0x300};
    const uint32_t events[3]={BASE+0x180,BASE+0x1a0,BASE+0x1c0};
    for(unsigned i=0;i<3;++i){uint8_t*r=nodes+i*20;put32(r,roots[i]);put32(r+4,i==0?1:0);put32(r+8,BASE+0xa0);put32(r+12,i?2:0);put32(r+16,events[i]);}
    put32(at(b,BASE+0xa0),BASE+0xc0);put32(at(b,BASE+0xc0)+4,2);put32(at(b,BASE+0xc0)+8,BASE+0x100);
    uint8_t*l=at(b,BASE+0x100);put16(l,0xffff);put32(l+4,1);put16(l+8,0xffff);put32(l+12,2);
    put32(at(b,roots[1])+4,1); /* node 1 is the currently selected child */
    uint8_t*e=at(b,events[1]);put16(e,0x8000);put16(e+2,0xfff9);put32(e+4,BASE+0x400);
    put16(e+8,1);put16(e+10,0xfff9);put32(e+12,BASE+0x400);
    e=at(b,events[2]);put16(e,0x8000);put16(e+2,0xfffa);put32(e+4,BASE+0x440);
    put16(e+8,2);put16(e+10,0xfffa);put32(e+12,BASE+0x440);
    make_script(b,0x400,0x111);make_script(b,0x440,0x222);
    return 1;
}

int main(void){
    const size_t size=0x600;uint8_t*bytes=malloc(size),*before=malloc(size);if(!bytes||!before)return 2;
    if(!fixture(bytes,size))return 3;
    uint32_t flags=0x55aa;probe p={0};nflstreet_ui_focus_effects effects={0};
    if(!nflstreet_ui_focus_indexed_execute_context(bytes,size,BASE,2,1,0xabc,&effects,&flags,
       CURRENT_SCREEN,LINKED_SCREEN,STACK_BASE,NULL,&p,sizeof p,0,memory_service)){fputs("context focus execution failed\n",stderr);return 4;}
    if(effects.count!=4||p.calls!=16||flags!=0x55aa)return 5;
    /* Each focus script must query current, linked, then exercise stack memory
       twice. Prior child deselect scripts precede target select scripts. */
    const uint32_t expected_property[4]={0x111,0x111,0x222,0x222};
    for(unsigned s=0;s<4;++s){unsigned i=s*4;
        if(p.kinds[i]!=9||p.kinds[i+1]!=8||p.kinds[i+2]!=1||p.kinds[i+3]!=1||
           p.targets[i]!=CURRENT_SCREEN||p.targets[i+1]!=LINKED_SCREEN||
           p.properties[i+2]!=expected_property[s]||
           p.properties[i+3]!=(expected_property[s]==0x111?0x40000111u:0x40000222u))return 6;
        for(unsigned j=0;j<4;++j)if(p.stack_bases[i+j]!=STACK_BASE)return 7;
    }
    if(effects.requests[0].root_offset!=0x280||effects.requests[1].root_offset!=0x280||
       effects.requests[2].root_offset!=0x300||effects.requests[3].root_offset!=0x300||
       effects.requests[0].args[0]!=0||effects.requests[1].args[0]!=0||
       effects.requests[2].args[0]!=0xabc||effects.requests[3].args[0]!=0xabc||
       effects.requests[0].args[1]!=UINT32_MAX||effects.requests[2].args[1]!=UINT32_MAX)return 8;
    if(get32(at(bytes,BASE+0x284))!=0||get32(at(bytes,BASE+0x304))!=1)return 9;

    /* A failure in a later service rolls back source bytes, flags, effects,
       and all earlier changes to the copied service context. */
    if(!fixture(bytes,size))return 10;
    memcpy(before,bytes,size);flags=0xdeadbeef;
    probe failing={0};failing.fail_at=7;memset(&effects,0xa5,sizeof effects);
    nflstreet_ui_focus_effects old_effects=effects;probe old_failing=failing;
    if(nflstreet_ui_focus_indexed_execute_context(bytes,size,BASE,2,1,9,&effects,&flags,
       CURRENT_SCREEN,LINKED_SCREEN,STACK_BASE,NULL,&failing,sizeof failing,0,memory_service)||
       memcmp(bytes,before,size)||flags!=0xdeadbeef||memcmp(&effects,&old_effects,sizeof effects)||
       memcmp(&failing,&old_failing,sizeof failing))return 11;
    /* Invalid overlapping VM stack is rejected atomically. */
    probe overlap={0};if(nflstreet_ui_focus_indexed_execute_context(bytes,size,BASE,2,1,9,&effects,&flags,
       CURRENT_SCREEN,LINKED_SCREEN,BASE+0x500,NULL,&overlap,sizeof overlap,0,memory_service)||
       memcmp(bytes,before,size)||memcmp(&overlap,&(probe){0},sizeof overlap))return 12;
    /* Rejection must not depend on a script being found: the owned memory
       regions are invalid independently of the selected node's callbacks. */
    if(!fixture(bytes,size))return 13;
    put32(at(bytes,BASE+32+1*20+12),0);
    put32(at(bytes,BASE+32+2*20+12),0);
    memcpy(before,bytes,size);memset(&effects,0x5a,sizeof effects);old_effects=effects;
    probe no_script={0};probe old_no_script=no_script;
    if(nflstreet_ui_focus_indexed_execute_context(bytes,size,BASE,2,1,9,&effects,&flags,
       CURRENT_SCREEN,LINKED_SCREEN,BASE+0x500,NULL,&no_script,sizeof no_script,0,memory_service)||
       memcmp(bytes,before,size)||memcmp(&effects,&old_effects,sizeof effects)||
       memcmp(&no_script,&old_no_script,sizeof no_script))return 14;
    puts("Context-aware focus API passed: ordered previous-child deselect then target select; current/linked screen IDs and caller stack base reached memory services; service stack writes were read back; later failure and overlapping-stack guards roll back atomically. No team navigation or physical input mapping claimed.");
    free(bytes);free(before);return 0;
}
