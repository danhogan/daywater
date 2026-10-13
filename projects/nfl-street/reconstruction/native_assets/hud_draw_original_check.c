/* Original HUD draw on the same prepared RAM as each native case: the 14
   name tags through 800DADC0, one 80186B1C(font, text, flags, limit,
   position), one camera view 8017D16C(camera, data), or the uis screens
   8012AB5C(camera, 0). Every game routine executes in the original module, matrix
   stacks, fog switch and error reporting included; only GX SDK entry points
   are recorded (gx_oracle.h) and immediate vertex data is captured at the
   GX pipe. Calls, pipe writes, render-state globals and all RAM outside the
   oracle stack must equal the native results. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#define NFLSTREET_HUD_ORACLE 1
#include "hud_draw_check.c"
#include "gx_oracle.h"
static int initialize_oracle(const char *dll){return gx_oracle_open(dll);}
/* The uis resource lock 801AF944/801AF9CC (an OS mutex around archive
   state queries) is a supplied no-op, as natively. */
static int resource_lock(CPUState *c,void *p){(void)p;if(c->pc==0x801af944u||c->pc==0x801af9ccu){c->pc=c->lr;return 1;}return 0;}
static int original_case(const uint8_t *before,const hud_case *hc,const trace *t,uint8_t *after,const nflstreet_render_state *s){
 memcpy(oracle_ram,before,RAM_BYTES);unsigned n=0;
 if(hc->kind==4){const uint32_t args[6]={ram32(oracle_ram,R13-0x6cd8u),0,0,0,0,0};if(!gx_oracle_run(0x8012ab5cu,args,t,resource_lock,NULL,&n))return 0;}
 else if(hc->kind==3){const uint32_t args[6]={hc->camera,ram32(oracle_ram,hc->camera+0x40),0,0,0,0};if(!gx_oracle_run(0x8017d16cu,args,t,NULL,NULL,&n))return 0;}
 else if(hc->kind==2){const uint32_t args[6]={hc->font,hc->text,hc->flags,hc->position,0,0};gx_oracle_f1=hc->limit;
  int ok=gx_oracle_run(0x80186b1cu,args,t,NULL,NULL,&n);gx_oracle_f1=0.0;if(!ok)return 0;}
 else for(unsigned i=0;i<HUD_NAME_TAG_COUNT;++i){const uint32_t args[6]={hud_name_tag(i),0,0,0,0,0};unsigned k=0;trace rest={t->calls+n,t->count-n};
  if(!gx_oracle_run(0x800dadc0u,args,&rest,NULL,NULL,&k))return 0;n+=k;}
 if(n!=t->count){fprintf(stderr,"original issued %u GX calls, native %u\n",n,t->count);return 0;}
 if(!gx_oracle_same_ram(after,s))return 0;
 ++oracle_runs;oracle_calls+=n;return 1;}
