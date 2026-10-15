#ifndef NFLSTREET_FIELD_OVERLAY_ORACLE_H
#define NFLSTREET_FIELD_OVERLAY_ORACLE_H
/* Check-only extended runner. SDK GXGetProjectionv and C_MTXOrtho execute
   their original blocks; projection setters are captured, as in gx_oracle.h.
   GXPeekZ is an EFB hardware service, with a supplied deterministic result.
   No game routine is skipped or replaced by native code. */
#include "gx_oracle.h"
typedef struct {nflstreet_field_depth depth;void *user;unsigned peeks,quads;} field_oracle_service;
static int field_compare(const nflstreet_gx_call *g){
 if(pipe.n>=pipe.t->count||memcmp(g,pipe.t->calls+pipe.n,sizeof *g)){fprintf(stderr,"extended GX call %u differs\n",pipe.n);print_call("original",g);
  if(pipe.n<pipe.t->count)print_call("native",pipe.t->calls+pipe.n);return -1;}++pipe.n;return 1;}
static int field_service(CPUState *c,void *context){field_oracle_service *s=context;uint32_t pc=c->pc;
 if(pc==0x80159538u)++s->quads;
 if(pc==0x80207d5cu||pc==0x80207ca8u){nflstreet_gx_call g;memset(&g,0,sizeof g);
  unsigned n=pc==0x80207d5cu?7:16;const uint8_t *p=ram_at(oracle_ram,c->gpr[3],4*n);if(!p)return -1;
  g.op=pc==0x80207d5cu?NFLSTREET_GX_SET_PROJECTION_V:NFLSTREET_FIELD_GX_SET_PROJECTION;if(n==16)g.arg[0]=c->gpr[4];
  for(unsigned i=0;i<n;++i)g.data[i]=rd32(p+4*i);if(field_compare(&g)<0)return -1;c->pc=c->lr;return 1;}
 if(pc==0x80203ce4u){nflstreet_gx_call g;memset(&g,0,sizeof g);g.op=NFLSTREET_FIELD_GX_PEEK_Z;g.arg[0]=c->gpr[3];g.arg[1]=c->gpr[4];
  uint8_t *p=ram_at(oracle_ram,c->gpr[5],4);if(!p||!s->depth(s->user,g.arg[0],g.arg[1],&g.data[0]))return -1;
  if(field_compare(&g)<0)return -1;wr32(p,g.data[0]);++s->peeks;c->pc=c->lr;return 1;}
 if((pc>=0x80207e1cu&&pc<0x80207e7cu)||(pc>=0x80200000u&&pc<0x80200098u)){
  do{c->downcount=-1000000000;gx_module->dispatch(c,c->pc);if(c->exception)return -1;pc=c->pc;}
  while((pc>=0x80207e1cu&&pc<0x80207e7cu)||(pc>=0x80200000u&&pc<0x80200098u));return 1;}
 return 0;}
static int field_oracle_run(const trace *t,field_oracle_service *s,unsigned *n){const uint32_t args[6]={0x80ded1fcu,0,0,0,0,0};
 return gx_oracle_run(0x800e5ca0u,args,t,field_service,s,n);}
#endif
