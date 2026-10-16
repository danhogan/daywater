#include "play_selection.h"
/* Independent native closure, emitted from source disassembly at construction
   time. No runtime instruction decoder and no original-module dependency. */
#include "play_selection_native.inc"
static uint32_t ps_word(nflstreet_play_selection *c,uint32_t a){
 const uint8_t *p=c->ram.span(c->ram.memory,a,4);if(!p){c->ok=0;c->failure_address=a;return 0;}
 return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t ps_team(nflstreet_play_selection *c,uint32_t team){return ps_word(c,c->ram.r13-0x5a98u)+team*0x7878u;}
void nflstreet_play_selection_init(nflstreet_play_selection *c,nflstreet_actor_ai ram){nflstreet_play_native_init(c,ram);}
int nflstreet_play_selection_call(nflstreet_play_selection *c,uint32_t pc,const uint32_t args[6],uint32_t *result){
 if(!c||!args||!result)return 0;nflstreet_ai_call call={pc,{0}};memcpy(call.args,args,sizeof call.args);
 return nflstreet_play_native_host_call(c,&call,0,result);}
static int ps_call(nflstreet_play_selection *c,uint32_t pc,uint32_t a,uint32_t b,uint32_t d,uint32_t *v){
 const uint32_t args[6]={a,b,d,0,0,0};return nflstreet_play_selection_call(c,pc,args,v);}
int nflstreet_play_select(nflstreet_play_selection *c,uint32_t team,nflstreet_selected_play *out){
 if(!c||!out||team>1||!c->ram.span)return 0;c->ok=1;
 const uint32_t field=ps_word(c,c->ram.r13-0x7078u);const uint8_t *p=c->ram.span(c->ram.memory,field+0x38u,1);
 if(!c->ok||!p||*p>1)return 0;
 const uint32_t pc=team==*p?0x800f4e54u:0x800f4ee0u;uint32_t result,ignored;
 /* Same flags used around selection/expansion by 8003729C. Keep scheduling
    scopes host-owned; the game script, database queries, weights and RNG run
    unchanged. Choice 0 intentionally does not reposition actors. */
 if(!ps_call(c,0x800f3264u,3,0,0,&ignored)||!ps_call(c,pc,0,0,0,&result))return 0;
 if(!ps_call(c,0x800f2718u,team,0,0,&ignored)||!ps_call(c,0x800f3278u,3,0,0,&ignored))return 0;
 out->team=team;const uint32_t play=ps_team(c,team)+0xd1cu;out->id=ps_word(c,play);
 const uint8_t *name=c->ram.span(c->ram.memory,play+0x1f0u,28);if(!name){c->ok=0;c->failure_address=play+0x1f0u;return 0;}
 memcpy(out->name,name,28);out->name[28]=0;out->script_result=(int32_t)result;return c->ok;}
int nflstreet_play_install(nflstreet_play_selection *c,uint32_t team){
 if(!c||team>1||!c->ram.span)return 0;c->ok=1;const uint32_t base=ps_team(c,team);uint32_t ignored;
 if(!c->ok)return 0;return ps_call(c,0x800ef234u,base+0x38u,base+0xd1cu,team,&ignored);}
