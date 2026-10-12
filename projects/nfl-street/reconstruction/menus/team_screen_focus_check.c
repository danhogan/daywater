#include "team_screen.h"
#include "events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 0x81100000u
#define SIZE 0x800u
static void put16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static void put32(uint8_t *p, uint32_t v) { p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v; }
static uint32_t get32(const uint8_t *p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static uint8_t *at(uint8_t *b, uint32_t address) { return address<BASE||address-BASE>=SIZE?NULL:b+(address-BASE); }

static void push(uint8_t *b, size_t *pc, uint32_t value) { b[(*pc)++]=0x10; put32(b+*pc,value); *pc+=4; }
static void ret(uint8_t *b, size_t *pc) { push(b,pc,0); b[(*pc)++]=0x44; }

static int focus_script(uint8_t *b, size_t offset) {
    size_t pc=offset; b[pc++]=0x48; /* supplied linked-screen context query */
    push(b,&pc,0); push(b,&pc,0x135); push(b,&pc,0); b[pc++]=0x0b;
    ret(b,&pc); return pc<SIZE;
}

static int fixture(uint8_t *b) {
    memset(b,0,SIZE); put32(b,3); put32(b+4,BASE+0x20);
    uint8_t *nodes=at(b,BASE+0x20); if(!nodes)return 0;
    put32(nodes+0,BASE+0x200); put32(nodes+4,1); put32(nodes+8,BASE+0xa0); put32(nodes+12,3); put32(nodes+16,BASE+0x180);
    put32(nodes+20,BASE+0x280); put32(nodes+24,0); put32(nodes+28,BASE+0xb0); put32(nodes+32,1); put32(nodes+36,BASE+0x1b0);
    put32(nodes+40,BASE+0x300); put32(nodes+44,0); put32(nodes+48,BASE+0xb0); put32(nodes+52,1); put32(nodes+56,BASE+0x1d0);
    /* node0 owns children1/2 and initially has both event-dispatch words set. */
    put32(at(b,BASE+0xa0),BASE+0xc0); put32(at(b,BASE+0xc0)+4,2); put32(at(b,BASE+0xc0)+8,BASE+0xe0);
    put16(at(b,BASE+0xe0),0xffff); put32(at(b,BASE+0xe4),1);
    put16(at(b,BASE+0xe8),0xffff); put32(at(b,BASE+0xec),2);
    put32(at(b,BASE+0x204),1); put32(at(b,BASE+0x260),1);
    put32(at(b,BASE+0x284),1); put32(at(b,BASE+0x304),0);
    /* kind0 event2 on node0 queues target node2 through source opcode47. */
    put16(at(b,BASE+0x180),0x8000); put16(at(b,BASE+0x182),2); put32(at(b,BASE+0x184),BASE+0x400);
    /* owner rows select callbacks for child1 deselect and child2 select. */
    put16(at(b,BASE+0x188),1); put16(at(b,BASE+0x18a),0xfff9); put32(at(b,BASE+0x18c),BASE+0x480);
    put16(at(b,BASE+0x190),2); put16(at(b,BASE+0x192),0xfffa); put32(at(b,BASE+0x194),BASE+0x4c0);
    put16(at(b,BASE+0x1b0),0x8000); put16(at(b,BASE+0x1b2),0xfff9); put32(at(b,BASE+0x1b4),BASE+0x500);
    put16(at(b,BASE+0x1d0),0x8000); put16(at(b,BASE+0x1d2),0xfffa); put32(at(b,BASE+0x1d4),BASE+0x540);
    /* op47 reads a relative list pointer from this owned cell; list entries are
       file-relative root addresses exactly as the indexed focus API expects. */
    put32(at(b,BASE+0x150),0x160); put32(at(b,BASE+0x160),1); put32(at(b,BASE+0x168),0x300);
    size_t pc=0x400; push(b,&pc,0); push(b,&pc,BASE+0x150); push(b,&pc,2); b[pc++]=0x47; ret(b,&pc);
    if(!focus_script(b,0x480)||!focus_script(b,0x4c0)||!focus_script(b,0x500)||!focus_script(b,0x540))return 0;
    return 1;
}

static int run_success(void) {
    uint8_t *b=malloc(SIZE); nflstreet_team_screen *s=calloc(1,sizeof *s); if(!b||!s)return 0;
    if(!fixture(b))return 0;s->classes.shared.link_enabled=1;s->classes.shared.linked_screen=5;
    unsigned event_found=0;uint32_t event_pc=0;if(!nflstreet_ui_event(b,SIZE,BASE,0,0,2,0,&event_pc,&event_found)||!event_found||event_pc!=0x400){fprintf(stderr,"event lookup failed found=%u pc=%x\n",event_found,event_pc);return 0;}
    /* An older request is retained but must not be replayed or resolve. */
    s->classes.shared.pending_count=1;s->classes.shared.pending[0].kind=7;s->classes.shared.pending[0].target=0xdeadbeef;
    int input_ok=nflstreet_team_screen_input(b,SIZE,BASE,s,0,2,1.0f);
    uint8_t *n1=at(b,BASE+0x280),*n2=at(b,BASE+0x300);
    if(!input_ok||!n1||!n2||get32(n1+4)!=0||get32(n2+4)!=1||s->classes.shared.pending_count!=2||
       s->classes.shared.pending[0].target!=0xdeadbeef||s->classes.shared.pending[1].kind!=7||
       s->classes.shared.pending[1].target!=BASE+0x300||s->classes.shared.pending[1].property!=2||
       s->classes.shared.pending[1].args[0]!=BASE+0x160||s->classes.shared.pending[1].args[1]!=0||
       s->classes.shared.link_count!=4){
        fprintf(stderr,"success fixture mismatch input=%d selected=%u/%u pending=%u links=%u target=%08x property=%x list=%08x index=%u\n",input_ok,n1?get32(n1+4):99,n2?get32(n2+4):99,s->classes.shared.pending_count,s->classes.shared.link_count,s->classes.shared.pending[1].target,s->classes.shared.pending[1].property,s->classes.shared.pending[1].args[0],s->classes.shared.pending[1].args[1]);return 0;}
    free(s);free(b);return 1;
}

static int run_rollback(void) {
    uint8_t *b=malloc(SIZE),*old=malloc(SIZE); nflstreet_team_screen *s=calloc(1,sizeof *s),*prior=malloc(sizeof *prior);
    if(!b||!old||!s||!prior)return 0;if(!fixture(b))return 0;
    s->classes.shared.link_enabled=0;s->classes.shared.linked_screen=5;memcpy(old,b,SIZE);*prior=*s;
    if(nflstreet_team_screen_input_context(b,SIZE,BASE,s,0,2,1.0f,7,UINT32_MAX,0x817c0000u)||memcmp(b,old,SIZE)||memcmp(s,prior,sizeof *s))return 0;
    free(prior);free(s);free(old);free(b);return 1;
}

static int run_explicit_manager_context(void) {
    uint8_t *b=malloc(SIZE);nflstreet_team_screen *s=calloc(1,sizeof *s);
    if(!b||!s||!fixture(b))return 0;
    s->classes.shared.link_enabled=1;s->classes.shared.linked_screen=UINT32_MAX;
    nflstreet_ui_focus_effects effects={0};
    int first=nflstreet_team_screen_focus_context(b,SIZE,BASE,s,1,1,0,7,UINT32_MAX,
          0x817f2380u,BASE+0x160,&effects);
    if(!first||effects.count!=2){fprintf(stderr,"manager first failed ok=%d effects=%u\n",first,effects.count);return 0;}
    int second=nflstreet_team_screen_focus_context(b,SIZE,BASE,s,2,1,0,7,UINT32_MAX,
          0x817f2380u,BASE+0x160,&effects);
    if(!second||effects.count!=4){fprintf(stderr,"manager second failed ok=%d effects=%u selected=%u/%u\n",second,effects.count,get32(at(b,BASE+0x280)+4),get32(at(b,BASE+0x300)+4));return 0;}
    uint8_t *n1=at(b,BASE+0x280),*n2=at(b,BASE+0x300);
    int ok=n1&&n2&&get32(n1+4)==0&&get32(n2+4)==1;
    free(s);free(b);return ok;
}

static int run_context_overlap(void) {
    uint8_t *b=malloc(SIZE),*old=malloc(SIZE);
    nflstreet_team_screen *s=calloc(1,sizeof *s),*prior=malloc(sizeof *prior);
    if(!b||!old||!s||!prior||!fixture(b))return 0;
    memcpy(old,b,SIZE);*prior=*s;
    /* Even a no-script event must reject a VM stack intersecting the source. */
    int ok=nflstreet_team_screen_input_context(b,SIZE,BASE,s,0,4,1.0f,
        7,UINT32_MAX,BASE+0x400);
    int unchanged=!ok&&!memcmp(b,old,SIZE)&&!memcmp(s,prior,sizeof *s);
    free(prior);free(s);free(old);free(b);return unchanged;
}

static int run_supplied_input_context(void) {
    uint8_t *b=malloc(SIZE);nflstreet_team_screen *s=calloc(1,sizeof *s);
    if(!b||!s||!fixture(b))return 0;
    s->classes.shared.link_enabled=1;s->classes.shared.linked_screen=UINT32_MAX;
    if(!nflstreet_team_screen_input_context(b,SIZE,BASE,s,0,2,1.0f,
          7,UINT32_MAX,0x817f2380u))return 0;
    uint8_t *n1=at(b,BASE+0x280),*n2=at(b,BASE+0x300);
    int ok=n1&&n2&&get32(n1+4)==0&&get32(n2+4)==1&&
       s->classes.shared.pending_count==1&&s->classes.shared.pending[0].target==BASE+0x300;
    free(s);free(b);return ok;
}

int main(void) {
    if(!run_success()){fputs("Team focus drain source integration or pending-prefix check failed\n",stderr);return 2;}
    if(!run_rollback()){fputs("Team focus drain failure did not roll back source and state\n",stderr);return 3;}
    if(!run_explicit_manager_context()){fputs("Explicit manager focus context failed\n",stderr);return 4;}
    if(!run_context_overlap()){fputs("Source and VM stack overlap was not rejected transactionally\n",stderr);return 5;}
    if(!run_supplied_input_context()){fputs("Supplied source input VM context was not propagated\n",stderr);return 6;}
    puts("Team focus checks passed: source op47 kind7 drain preserves the pending prefix; explicit manager focus uses supplied context7/FFFFFFFF stack0x817f2380 and selects child1 then child2; supplied input context reaches source and focus scripts; late service failure and even no-script source/stack overlap reject transactionally.");
    return 0;
}
