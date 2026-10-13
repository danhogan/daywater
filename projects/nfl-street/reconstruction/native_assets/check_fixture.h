#ifndef NFLSTREET_CHECK_FIXTURE_H
#define NFLSTREET_CHECK_FIXTURE_H
/* Check-only fixture shared by the scenery and HUD checks: saved RAM
   access, the recording draw context, the deterministic generator and the
   native render state from saved globals. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define CALLS_MAX 32768u
#define STACK_LO 0x80f38000u
#define STACK_HI 0x80f47000u
#define STATE 0x8039e560u
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *ram_at(uint8_t *ram,uint32_t a,uint32_t n){return a>=RAM_BASE&&(uint64_t)a-RAM_BASE+n<=RAM_BYTES?ram+(a-RAM_BASE):NULL;}
static uint32_t ram32(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,4);return p?rd32(p):0;}
static float ramf(uint8_t *ram,uint32_t a){uint32_t v=ram32(ram,a);float f;memcpy(&f,&v,4);return f;}
#include "venue_fixture.h"
static uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint8_t *load_ram(const char *path){FILE *f=fopen(path,"rb");if(!f)return NULL;uint8_t *b=malloc(RAM_BYTES);
 if(!b||fread(b,1,RAM_BYTES,f)!=RAM_BYTES){free(b);fclose(f);return NULL;}fclose(f);return b;}
static uint8_t *span(void *m,uint32_t token,uint32_t size){
 if((uint64_t)token+size>STACK_LO&&token<STACK_HI)return NULL;return ram_at(m,token,size);}
typedef struct {nflstreet_gx_call *calls;unsigned count;} trace;
static int record(void *t,const nflstreet_gx_call *c){trace *r=t;if(r->count>=CALLS_MAX)return 0;r->calls[r->count++]=*c;return 1;}
static uint32_t seed=0x13579bdu;
static uint32_t rnd(void){seed=seed*1103515245u+12345u;return seed>>8;}
static float rndf(float lo,float hi){return lo+(hi-lo)*(float)(rnd()&0xffffu)/65535.0f;}
static uint32_t fbits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
static nflstreet_scenery_context scenery_context(uint8_t *ram,nflstreet_player_draw_context *x,trace *t){
 memset(x,0,sizeof *x);x->span=span;x->memory=ram;x->emit=record;x->sink=t;
 x->default_texture=ram32(ram,R13-0x640cu);x->null_shader=ram32(ram,R13-0x5684u);
 x->setups[0]=0x801c9d0cu;x->setups[1]=0x801c9dacu;x->setups[2]=0x801c9dfcu;x->setups[3]=0x801c9d5cu;
 return venue_context(ram,x);}
/* Native render state from the saved globals (8039E560.., r13-6220/-5630, 80293A54..). */
static void saved_state(uint8_t *ram,nflstreet_render_state *s){
 uint32_t *w[]={&s->chunks,&s->palette,&s->model_10,&s->groups,&s->materials,&s->instance,&s->shader,&s->blend,&s->overrides};
 for(unsigned i=0;i<9;++i)*w[i]=ram32(ram,STATE+4*i);
 for(unsigned i=0;i<6;++i)s->viewport[i]=ramf(ram,STATE+0x24+4*i);
 s->effects=ram32(ram,STATE+0x3c);s->format=ram32(ram,STATE+0x60);s->model_04=ram32(ram,STATE+0x64);s->model_24=ram32(ram,STATE+0x68);
 s->model_28=ram32(ram,STATE+0x6c);s->model_30=ram32(ram,STATE+0x70);s->model_20=ram32(ram,STATE+0x88);s->override=ram32(ram,STATE+0x8c);
 s->lod=ram32(ram,STATE+0x90);s->light_set=ram32(ram,R13-0x6220u);s->light_mask=ram32(ram,R13-0x5630u);
 for(unsigned i=0;i<4;++i)s->matrix_cache[i]=ram32(ram,0x80293a54u+4*i);}
#endif
