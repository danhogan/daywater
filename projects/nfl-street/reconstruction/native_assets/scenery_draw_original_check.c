/* Original scenery draw on the same prepared RAM as each native case: the
   venue loops 8015B670 (field passes) and 80165DC4 (main field), or one
   801EC8E4 call. Every game routine executes in the original module except:
   GX SDK entry points (recorded, gx_oracle.h); the matrix-stack updates
   801893C8, 8018941C, 80189BB0, 8018949C and the model-view store 80189ED8,
   which return at once so instances keep their stored matrices as natively;
   and the callees the native loops do not model (800D75EC, 801652BC,
   8015A5CC, 8015A7C4), which return at once. Calls, render-state globals
   and all RAM outside the oracle stack must equal the native results. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#define NFLSTREET_SCENERY_ORACLE 1
#include "scenery_draw_check.c"
#include "gx_oracle.h"
static int initialize_oracle(const char *dll){return gx_oracle_open(dll);}
static int venue_hooks(CPUState *c,void *p){(void)p;
 static const uint32_t skipped[]={0x801893c8u,0x8018941cu,0x80189bb0u,0x8018949cu,0x80189ed8u,0x800d75ecu,0x801652bcu,0x8015a5ccu,0x8015a7c4u};
 for(unsigned i=0;i<sizeof skipped/sizeof *skipped;++i)if(c->pc==skipped[i]){c->pc=c->lr;return 1;}
 return 0;}
/* Ring matrices: also rotation, scale and identity (80189A34, 80189CEC,
   801894B0, 80189944) return at once. */
static int ring_hooks(CPUState *c,void *p){
 static const uint32_t skipped[]={0x80189a34u,0x80189cecu,0x801894b0u,0x80189944u};
 for(unsigned i=0;i<sizeof skipped/sizeof *skipped;++i)if(c->pc==skipped[i]){c->pc=c->lr;return 1;}
 return venue_hooks(c,p);}
static int original_case(const uint8_t *before,const scenery_case *sc,const trace *t,uint8_t *after,const nflstreet_render_state *s){
 memcpy(oracle_ram,before,RAM_BYTES);unsigned n=0,m=0;uint32_t camera=ram32(oracle_ram,R13-0x4f44u);
 if(sc->kind==6){const uint32_t none[6]={0};if(!gx_oracle_run(0x800d75ecu,none,t,NULL,NULL,&n))return 0;}
 else if(sc->kind==5){for(unsigned i=0;i<4;++i){const uint32_t args[6]={venue_lines[i],0,0,0,0,0};unsigned k=0;trace rest={t->calls+n,t->count-n};
   if(!gx_oracle_run(0x800da074u,args,&rest,NULL,NULL,&k))return 0;n+=k;}}
 else if(sc->kind==4){for(unsigned i=0;i<3;++i){const uint32_t args[6]={venue_rings[i],0,0,0,0,0};unsigned k=0;trace rest={t->calls+n,t->count-n};
   if(!gx_oracle_run(0x800e3f7cu,args,&rest,ring_hooks,NULL,&k))return 0;n+=k;}}
 else if(sc->kind==3){const uint32_t none[6]={0};unsigned m2=0;if(!gx_oracle_run(0x801655d8u,none,t,NULL,NULL,&n))return 0;
  trace rest={t->calls+n,t->count-n};if(!gx_oracle_run(0x801643f4u,none,&rest,NULL,NULL,&m2))return 0;n+=m2;}
 else if(sc->kind==2){const uint32_t args[6]={sc->instance,camera,sc->lights,0,0,0};if(!gx_oracle_run(0x801ec8e4u,args,t,venue_hooks,NULL,&n))return 0;}
 else{const uint32_t passes[6]={FIELD_PASSES,0,0,0,0,0},field[6]={MAIN_FIELD,0,0,0,0,0};
  if(!gx_oracle_run(0x8015b670u,passes,t,venue_hooks,NULL,&n))return 0;
  trace rest={t->calls+n,t->count-n};if(!gx_oracle_run(0x80165dc4u,field,&rest,venue_hooks,NULL,&m))return 0;n+=m;}
 if(n!=t->count){fprintf(stderr,"original issued %u GX calls, native %u\n",n,t->count);return 0;}
 if(!gx_oracle_same_ram(after,s))return 0;
 ++oracle_runs;oracle_calls+=n;return 1;}
