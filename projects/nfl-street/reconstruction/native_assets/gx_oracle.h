#ifndef NFLSTREET_GX_ORACLE_H
#define NFLSTREET_GX_ORACLE_H
/* Check-only: runs original routines in the preserved ModernGekko module and
   records their GX SDK calls at the SDK entry points (returning without
   executing them; they would write the GPU FIFO), comparing each with the
   native call at the same position. GXInitLightColor/GXGetLightColor
   (80205140/8020514C) only touch memory and execute. The including check
   defines RAM_BASE, RAM_BYTES, R13, STACK_LO, STACK_HI, ram_at, rd32, wr32,
   fbits and the trace type {calls, count}. */
#include <windows.h>
#include "moderngekko/module_abi.h"
static const ModernGekkoModuleDesc *gx_module;static uint8_t *oracle_ram;
static int gx_oracle_open(const char *dll){
 HMODULE h=LoadLibraryA(dll);if(!h)return 0;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get)return 0;gx_module=get();oracle_ram=malloc(RAM_BYTES);return gx_module&&oracle_ram;}
typedef struct {uint32_t pc;nflstreet_gx_op op;unsigned args;} sdk_entry;
/* args: integer register arguments; pointer and float arguments are read in capture. */
static const sdk_entry sdk[]={
 {0x80201fb8u,NFLSTREET_GX_SET_VTX_DESC,2},{0x802029a8u,NFLSTREET_GX_CLEAR_VTX_DESC,0},{0x802029e0u,NFLSTREET_GX_SET_VTX_ATTR_FMT,5},
 {0x802033ccu,NFLSTREET_GX_SET_ARRAY,3},{0x80203468u,NFLSTREET_GX_SET_TEX_COORD_GEN,6},{0x80203738u,NFLSTREET_GX_SET_NUM_TEX_GENS,1},
 {0x80204224u,NFLSTREET_GX_SET_CULL_MODE,1},{0x80205158u,NFLSTREET_GX_LOAD_LIGHT,0},{0x802051d4u,NFLSTREET_GX_SET_CHAN_AMB_COLOR,1},
 {0x802052c8u,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1},{0x802053bcu,NFLSTREET_GX_SET_NUM_CHANS,1},{0x80205400u,NFLSTREET_GX_SET_CHAN_CTRL,7},
 {0x80205bccu,NFLSTREET_GX_INIT_TEX_OBJ_TLUT,2},{0x80205da0u,NFLSTREET_GX_LOAD_TEX_OBJ,2},{0x80205e5cu,NFLSTREET_GX_LOAD_TLUT,2},
 {0x80206f1cu,NFLSTREET_GX_SET_TEV_COLOR_IN,5},{0x80206f60u,NFLSTREET_GX_SET_TEV_ALPHA_IN,5},{0x80206fa4u,NFLSTREET_GX_SET_TEV_COLOR_OP,6},
 {0x8020700cu,NFLSTREET_GX_SET_TEV_ALPHA_OP,6},{0x80207074u,NFLSTREET_GX_SET_TEV_COLOR,1},{0x80207320u,NFLSTREET_GX_SET_ALPHA_COMPARE,5},
 {0x802073f8u,NFLSTREET_GX_SET_TEV_ORDER,4},{0x802075d0u,NFLSTREET_GX_SET_NUM_TEV_STAGES,1},{0x8020791cu,NFLSTREET_GX_SET_BLEND_MODE,4},
 {0x80207c38u,NFLSTREET_GX_CALL_DISPLAY_LIST,2},{0x80207f1cu,NFLSTREET_GX_SET_CURRENT_MTX,1},{0x80207f54u,NFLSTREET_GX_LOAD_TEX_MTX,0},
 {0x8020810cu,NFLSTREET_GX_SET_VIEWPORT,0},{0x80207e7cu,NFLSTREET_GX_LOAD_POS_MTX,0},{0x80207eccu,NFLSTREET_GX_LOAD_NRM_MTX,0},
 {0x802079c8u,NFLSTREET_GX_SET_Z_MODE,3},{0x802079fcu,NFLSTREET_GX_SET_Z_COMP_LOC,1},{0x80207600u,NFLSTREET_GX_SET_FOG,1}};
static int capture(const CPUState *c,const sdk_entry *e,nflstreet_gx_call *g){
 memset(g,0,sizeof *g);g->op=e->op;for(unsigned i=0;i<e->args;++i)g->arg[i]=c->gpr[3+i];
 const uint8_t *p;
 switch(e->op){
 case NFLSTREET_GX_LOAD_LIGHT:if(!(p=ram_at(oracle_ram,c->gpr[3],64)))return 0;g->arg[0]=c->gpr[4];for(unsigned i=0;i<16;++i)g->data[i]=rd32(p+4*i);break;
 case NFLSTREET_GX_SET_CHAN_AMB_COLOR:case NFLSTREET_GX_SET_CHAN_MAT_COLOR:case NFLSTREET_GX_SET_TEV_COLOR:
  if(!(p=ram_at(oracle_ram,c->gpr[4],4)))return 0;g->data[0]=rd32(p);break;
 case NFLSTREET_GX_LOAD_TEX_MTX:{unsigned n=c->gpr[5]==1?8:12;if(!(p=ram_at(oracle_ram,c->gpr[3],4*n)))return 0;
  g->arg[0]=c->gpr[4];g->arg[1]=c->gpr[5];for(unsigned i=0;i<n;++i)g->data[i]=rd32(p+4*i);break;}
 case NFLSTREET_GX_SET_VIEWPORT:for(unsigned i=0;i<6;++i)g->data[i]=fbits((float)c->fpr[1+i]);break;
 case NFLSTREET_GX_LOAD_POS_MTX:if(!(p=ram_at(oracle_ram,c->gpr[3],48)))return 0;g->arg[0]=c->gpr[4];for(unsigned i=0;i<12;++i)g->data[i]=rd32(p+4*i);break;
 case NFLSTREET_GX_LOAD_NRM_MTX:if(!(p=ram_at(oracle_ram,c->gpr[3],48)))return 0;g->arg[0]=c->gpr[4];
  for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)g->data[3*r+k]=rd32(p+16*r+4*k);break;
 case NFLSTREET_GX_SET_FOG:if(!(p=ram_at(oracle_ram,c->gpr[4],4)))return 0;g->data[0]=rd32(p);for(unsigned i=0;i<4;++i)g->data[1+i]=fbits((float)c->fpr[1+i]);break;
 default:break;}
 return 1;}
static void print_call(const char *who,const nflstreet_gx_call *g){
 fprintf(stderr,"  %s op %d args",who,(int)g->op);for(unsigned i=0;i<7;++i)fprintf(stderr," %08x",g->arg[i]);
 fprintf(stderr," data");for(unsigned i=0;i<16;++i)fprintf(stderr," %08x",g->data[i]);fprintf(stderr,"\n");}
/* Render-state words in original RAM: the native state is written at these
   addresses before the whole-RAM comparison. */
static void put_state(uint8_t *ram,const nflstreet_render_state *s){
 const uint32_t base=0x8039e560u;uint32_t v[6];for(unsigned i=0;i<6;++i)v[i]=fbits(s->viewport[i]);
 const struct {uint32_t at,value;} w[]={{0x00,s->chunks},{0x04,s->palette},{0x08,s->model_10},{0x0c,s->groups},{0x10,s->materials},
  {0x14,s->instance},{0x18,s->shader},{0x1c,s->blend},{0x20,s->overrides},{0x24,v[0]},{0x28,v[1]},{0x2c,v[2]},{0x30,v[3]},{0x34,v[4]},
  {0x38,v[5]},{0x3c,s->effects},{0x60,s->format},{0x64,s->model_04},{0x68,s->model_24},{0x6c,s->model_28},{0x70,s->model_30},
  {0x88,s->model_20},{0x8c,s->override},{0x90,s->lod}};
 for(unsigned i=0;i<sizeof w/sizeof *w;++i)wr32(ram_at(ram,base+w[i].at,4),w[i].value);
 wr32(ram_at(ram,R13-0x6220u,4),s->light_set);wr32(ram_at(ram,R13-0x5630u,4),s->light_mask);
 for(unsigned i=0;i<4;++i)wr32(ram_at(ram,0x80293a54u+4*i,4),s->matrix_cache[i]);}
/* A service hook returns 1 when it handled the entry (the caller's pc is
   set), 0 to execute it, -1 to fail. */
typedef int (*gx_service)(CPUState *,void *);
/* Runs pc(args) from oracle_ram, compares recorded calls with t; *calls is
   the number recorded. */
static int gx_oracle_run(uint32_t pc,const uint32_t args[6],const trace *t,gx_service service,void *context,unsigned *calls){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle_ram;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;for(unsigned i=0;i<6;++i)c.gpr[3+i]=args[i];c.pc=pc;unsigned n=0;
 for(unsigned step=0;c.pc;++step){
  if(step>40000000u){fprintf(stderr,"%08x did not return (pc %08x)\n",pc,c.pc);return 0;}
  int h=service?service(&c,context):0;if(h<0)return 0;if(h)continue;
  uint32_t at=c.pc;
  if(at>=0x80200000u&&at<0x80210000u&&at!=0x80205140u&&at!=0x8020514cu){
   const sdk_entry *e=NULL;for(unsigned i=0;i<sizeof sdk/sizeof *sdk;++i)if(sdk[i].pc==at)e=sdk+i;
   nflstreet_gx_call g;
   if(!e||!capture(&c,e,&g)){fprintf(stderr,"unexpected SDK entry %08x (lr %08x)\n",at,c.lr);return 0;}
   if(n>=t->count||memcmp(&g,t->calls+n,sizeof g)){fprintf(stderr,"GX call %u of %u differs (lr %08x)\n",n,t->count,c.lr);
    print_call("original",&g);if(n<t->count)print_call("native",t->calls+n);return 0;}
   ++n;c.pc=c.lr;continue;}
  c.downcount=-1000000000;gx_module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"%08x: exception at %08x (lr %08x)\n",pc,c.pc,c.lr);return 0;}}
 *calls=n;return 1;}
/* Whole RAM outside the oracle stack, after writing the native state. */
static int gx_oracle_same_ram(uint8_t *after,const nflstreet_render_state *s){
 put_state(after,s);const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 if(!memcmp(oracle_ram,after,lo)&&!memcmp(oracle_ram+hi,after+hi,RAM_BYTES-hi))return 1;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&oracle_ram[i]!=after[i]){
  fprintf(stderr,"RAM differs at %08x: original %02x native %02x\n",RAM_BASE+i,oracle_ram[i],after[i]);break;}
 return 0;}
#endif
