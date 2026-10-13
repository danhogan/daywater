/* Original player draw 80161E10 on the same prepared RAM as each native case.
   The original module executes every game routine of the draw; only the GX
   SDK entry points are intercepted and recorded (gx_oracle.h). The head
   display-list compile behind 801626E0 is the supplied service: 801626E0
   runs (the module enters it directly within its chunk), and its cross-chunk
   helpers return the case's choice. Every recorded call must equal the native
   call at the same position; afterwards the render-state globals must equal
   the native state and all other RAM outside the oracle stack must equal the
   native RAM. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#define NFLSTREET_DRAW_ORACLE 1
#include "player_draw_check.c"
#include "gx_oracle.h"
static int initialize_oracle(const char *dll){return gx_oracle_open(dll);}
typedef struct {int compiled;unsigned requests;} face_service;
/* Buffer room (801CB8AC, threshold 0xCE3), a nonzero face weight (801CBC94)
   and the compile itself, which is not modeled (the saved compiled model stays). */
static int face_hooks(CPUState *c,void *p){face_service *f=p;uint32_t pc=c->pc;
 if(pc==0x801cb8acu){++f->requests;c->gpr[3]=f->compiled?0xce4u:0xce3u;c->pc=c->lr;return 1;}
 if(pc==0x801cbc94u){c->gpr[3]=1;c->pc=c->lr;return 1;}
 if(pc==0x801cbd28u||pc==0x801cbe18u||pc==0x801cbf60u||pc==0x801cbdccu||pc==0x801cbd74u){c->pc=c->lr;return 1;}
 return 0;}
static int original_case(const uint8_t *before,const draw_case *dc,const trace *t,uint8_t *after,const nflstreet_render_state *s){
 memcpy(oracle_ram,before,RAM_BYTES);face_service f={dc->compiled,0};unsigned n=0;
 const uint32_t args[6]={dc->actor,0,0,0,0,0};
 if(!gx_oracle_run(0x80161e10u,args,t,face_hooks,&f,&n))return 0;
 if(n!=t->count){fprintf(stderr,"original issued %u GX calls, native %u\n",n,t->count);return 0;}
 if(f.requests!=((dc->features&F_LOD0)&&(dc->features&F_FACE)?1u:0u)){fprintf(stderr,"801626E0 requested %u times\n",f.requests);return 0;}
 if(!gx_oracle_same_ram(after,s))return 0;
 ++oracle_runs;oracle_calls+=n;return 1;}
